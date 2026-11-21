#include "matcomp/server.hpp"
#include "matcomp/backend.hpp"
#include "matcomp/window.hpp"

#include <cerrno>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>

namespace matcomp {
namespace {

void start_client(void* data) {
    const char* command = static_cast<const char*>(data);
    if (fork() == 0) {
        // App output must not depend on the compositor's stdout pipe (a tee
        // that can die). Write straight to the serial console so failures are
        // visible in VM acceptance logs.
        const int tty = ::open("/dev/ttyS0", O_WRONLY | O_NOCTTY);
        if (tty >= 0) {
            ::dup2(tty, 1);
            ::dup2(tty, 2);
            ::close(tty);
        }
        execl("/bin/sh", "/bin/sh", "-c", command, static_cast<char*>(nullptr));
        _exit(127);
    }
}

void parse_output_mode(const std::string& text, int& width, int& height, int& refresh) {
    if (text == "auto") return;
    const auto x = text.find('x');
    const auto hz = text.find('@');
    if (x == std::string::npos) throw std::runtime_error("output mode must be WIDTHxHEIGHT[@HZ]");
    width = std::stoi(text.substr(0, x));
    height = std::stoi(text.substr(x + 1, hz == std::string::npos ? std::string::npos : hz - x - 1));
    if (hz != std::string::npos) refresh = static_cast<int>(std::stof(text.substr(hz + 1)) * 1000);
}

} // namespace

Server::Server(Config config) : config_(std::move(config)) {}
Server::~Server() {
    // Outputs must be destroyed while their signal listeners and Server-owned
    // storage are still valid. wlr_backend_destroy emits output.destroy.
    if (backend_) wlr_backend_destroy(backend_);
    listeners_.clear();
    if (display_) wl_display_destroy(display_);
}

void Server::initialize() {
    wlr_log_init(WLR_INFO, nullptr);
    display_ = wl_display_create();
    if (!display_) throw std::runtime_error("cannot create Wayland display");
    create_backend();
    scene_ = wlr_scene_create();
    layout_ = wlr_output_layout_create();
    if (!scene_ || !layout_) throw std::runtime_error("cannot create compositor scene/output layout");
    background_tree_ = wlr_scene_tree_create(&scene_->node);
    bottom_tree_ = wlr_scene_tree_create(&scene_->node);
    for (auto*& tree : workspace_trees_) tree = wlr_scene_tree_create(&scene_->node);
    fullscreen_tree_ = wlr_scene_tree_create(&scene_->node);
    const float backdrop_color[] = {0.075f, 0.067f, 0.09f, 1.0f};
    // The backdrop must live in the background layer. Creating it on the scene
    // root after the workspace trees would paint it OVER every client window.
    backdrop_ = wlr_scene_rect_create(&background_tree_->node, 0, 0, backdrop_color);
    if (!backdrop_) throw std::runtime_error("cannot create scene backdrop");
    top_tree_ = wlr_scene_tree_create(&scene_->node);
    overlay_tree_ = wlr_scene_tree_create(&scene_->node);
    popups_ = wlr_scene_tree_create(&scene_->node);
    for (std::size_t i = 0; i < workspace_trees_.size(); ++i)
        wlr_scene_node_set_enabled(&workspace_trees_[i]->node, i == 0);
    wlr_scene_node_set_enabled(&fullscreen_tree_->node, false);
    const float color[] = {0.075f, 0.067f, 0.09f, 1.0f};
    wlr_scene_rect_set_color(backdrop_, color);
    create_protocols();

    cursor_ = wlr_cursor_create();
    wlr_cursor_attach_output_layout(cursor_, layout_);
    cursor_manager_ = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(cursor_manager_, 1);
    seat_ = wlr_seat_create(display_, "seat0");
    if (!seat_) throw std::runtime_error("cannot create seat");
    xkb_context_ = {xkb_context_new(XKB_CONTEXT_NO_FLAGS), xkb_context_unref};

    auto bind = [this](auto& signal, auto callback) {
        listeners_.push_back(std::make_unique<Listener>());
        listeners_.back()->connect(signal, std::move(callback));
    };
    bind(backend_->events.new_output, [this](void* data) { add_output(static_cast<wlr_output*>(data)); });
    bind(backend_->events.new_input, [this](void* data) { add_input(static_cast<wlr_input_device*>(data)); });
    bind(cursor_->events.motion, [this](void* data) {
        auto* event = static_cast<wlr_event_pointer_motion*>(data);
        wlr_cursor_move(cursor_, event->device, event->delta_x, event->delta_y);
        process_cursor_motion(event->time_msec);
    });
    bind(cursor_->events.motion_absolute, [this](void* data) {
        auto* event = static_cast<wlr_event_pointer_motion_absolute*>(data);
        wlr_cursor_warp_absolute(cursor_, event->device, event->x, event->y);
        process_cursor_motion(event->time_msec);
    });
    bind(cursor_->events.button, [this](void* data) { on_cursor_button(static_cast<wlr_event_pointer_button*>(data)); });
    bind(cursor_->events.axis, [this](void* data) {
        auto* event = static_cast<wlr_event_pointer_axis*>(data);
        wlr_seat_pointer_notify_axis(seat_, event->time_msec, event->orientation,
            event->delta, event->delta_discrete, event->source);
    });
    bind(cursor_->events.frame, [this](void*) { pointer_frame(); });
    bind(xdg_shell_->events.new_surface, [this](void* data) { add_view(static_cast<wlr_xdg_surface*>(data)); });
    bind(layer_shell_->events.new_surface, [this](void* data) { add_layer(static_cast<wlr_layer_surface_v1*>(data)); });
    bind(decorations_->events.new_toplevel_decoration, [this](void* data) {
        auto* decoration = static_cast<wlr_xdg_toplevel_decoration_v1*>(data);
        wlr_xdg_toplevel_decoration_v1_set_mode(decoration,
            config_.server_decorations ? WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
                                       : WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
    });
    bind(seat_->events.request_set_cursor, [this](void* data) {
        auto* event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
        if (seat_->pointer_state.focused_client == event->seat_client)
            wlr_cursor_set_surface(cursor_, event->surface, event->hotspot_x, event->hotspot_y);
    });
    event_loop_ = wl_display_get_event_loop(display_);
}

void Server::create_backend() {
    if (config_.backend == BackendPreference::Headless) {
        backend_ = wlr_headless_backend_create(display_);
    } else {
        if (config_.backend == BackendPreference::Drm) setenv("WLR_BACKENDS", "drm", 1);
        backend_ = wlr_backend_autocreate(display_);
    }
    if (!backend_) throw std::runtime_error("cannot create wlroots backend (try --headless)");
    renderer_ = wlr_renderer_autocreate(backend_);
    if (!renderer_) throw std::runtime_error("cannot create renderer");
    wlr_renderer_init_wl_display(renderer_, display_);
    allocator_ = wlr_allocator_autocreate(backend_, renderer_);
    if (!allocator_) wlr_log(WLR_INFO, "headless backend uses its own software buffers; no allocator");
    std::cerr << "matcomp: backend initialized (headless=" << (config_.backend == BackendPreference::Headless) << ")\n";
}

void Server::create_protocols() {
    wlr_compositor_create(display_, renderer_);
    wlr_data_device_manager_create(display_);
    wlr_primary_selection_v1_device_manager_create(display_);
    xdg_shell_ = wlr_xdg_shell_create(display_);
    layer_shell_ = wlr_layer_shell_v1_create(display_);
    decorations_ = wlr_xdg_decoration_manager_v1_create(display_);
    wlr_presentation_create(display_, backend_);
    // Real screen capture for the screenshot tool (wlr-screencopy-unstable-v1).
    wlr_screencopy_manager_v1_create(display_);
}

void Server::run(const std::string& startup_command) {
    if (config_.backend == BackendPreference::Headless && outputs_.empty()) {
        auto* output = wlr_headless_add_output(backend_, config_.headless_width, config_.headless_height);
        if (!output) throw std::runtime_error("cannot create headless output");
        add_output(output);
    }
    if (!wlr_backend_start(backend_)) throw std::runtime_error("cannot start wlroots backend");
    const char* requested_socket = std::getenv("WAYLAND_DISPLAY");
    const char* socket = requested_socket && *requested_socket
            ? requested_socket : wl_display_add_socket_auto(display_);
    if (requested_socket && *requested_socket && wl_display_add_socket(display_, requested_socket) != 0)
        socket = nullptr;
    if (!socket) throw std::runtime_error("cannot allocate Wayland socket");
    setenv("WAYLAND_DISPLAY", socket, 1);
    std::cout << "matcomp: WAYLAND_DISPLAY=" << socket << '\n';
    if (!startup_command.empty()) {
        wl_event_loop_add_idle(event_loop_, start_client, const_cast<char*>(startup_command.c_str()));
        // The command is short-lived and ownership intentionally remains with
        // the compositor process until shutdown.
    }
    wl_display_run(display_);
}

bool Server::self_test() {
    if (config_.backend != BackendPreference::Headless) return false;
    if (outputs_.empty()) {
        auto* output = wlr_headless_add_output(backend_, 800, 600);
        if (!output) return false;
        add_output(output);
    }
    if (!wlr_backend_start(backend_)) return false;
    if (outputs_.empty()) return false;
    process_cursor_motion(0);
    switch_workspace(1);
    switch_workspace(0);
    return true;
}

void Server::add_output(wlr_output* raw) {
    if (!raw) return;
    if (!wlr_output_is_headless(raw)) {
        if (config_.output_mode != "auto") {
            int width = raw->width, height = raw->height, refresh = raw->refresh;
            parse_output_mode(config_.output_mode, width, height, refresh);
            wlr_output_set_custom_mode(raw, width, height, refresh);
        } else {
            wlr_output_set_mode(raw, wlr_output_preferred_mode(raw));
        }
    }
    auto output = std::make_unique<Output>(*this, raw);
    outputs_.push_back(std::move(output));
    wlr_output_layout_add_auto(layout_, raw);
    // Every real output must be initialized with the allocator/renderer before
    // enabling it, as required by wlroots' DRM backend. This also schedules the
    // first frame after a modeset; skipping it leaves clients unexposed.
    if (!wlr_output_is_headless(raw)) {
        if (!wlr_output_init_render(raw, allocator_, renderer_))
            throw std::runtime_error("cannot initialize output rendering");
        wlr_output_enable(raw, true);
        if (!wlr_output_commit(raw))
            wlr_log(WLR_ERROR, "initial output commit failed for %s", raw->name ? raw->name : "unknown");
    }
    wlr_scene_output_create(scene_, raw);
    apply_output_layout();
    // A new DRM output must request its first frame explicitly. Otherwise a
    // freshly mapped shell can remain blank until an unrelated input event.
    if (!wlr_output_is_headless(raw)) {
        wlr_output_schedule_frame(raw);
        std::cout << "matcomp: scheduled initial frame frame_pending=" << raw->frame_pending << '\n';
    }
}

void Server::add_input(wlr_input_device* device) {
    if (device->type == WLR_INPUT_DEVICE_KEYBOARD) {
        keyboards_.push_back(std::make_unique<Keyboard>(*this, device));
        wlr_seat_set_capabilities(seat_, WL_SEAT_CAPABILITY_KEYBOARD | WL_SEAT_CAPABILITY_POINTER);
    } else if (device->type == WLR_INPUT_DEVICE_POINTER) {
        wlr_cursor_attach_input_device(cursor_, device);
        wlr_seat_set_capabilities(seat_, WL_SEAT_CAPABILITY_POINTER |
            (keyboards_.empty() ? 0 : WL_SEAT_CAPABILITY_KEYBOARD));
        wlr_xcursor_manager_set_cursor_image(cursor_manager_, "left_ptr", cursor_);
    }
}

void Server::add_view(wlr_xdg_surface* surface) {
    if (surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        if (surface->popup->parent) new Popup(*this, popup_parent(), surface);
        return;
    }
    if (surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) return;
    views_.push_back(std::make_unique<View>(*this, surface));
}

void Server::add_layer(wlr_layer_surface_v1* surface) {
    layers_.push_back(std::make_unique<LayerSurface>(*this, surface, surface->output));
}

Output* Server::primary_output() const { return outputs_.empty() ? nullptr : outputs_.front().get(); }

Output* Server::output_at(double x, double y) const {
    auto* raw = wlr_output_layout_output_at(layout_, x, y);
    if (!raw) return primary_output();
    return raw->data ? static_cast<Output*>(raw->data) : nullptr;
}

Output* Server::output_for(const View& view) const {
    if (view.output && view.output->data) return static_cast<Output*>(view.output->data);
    return output_at(view.x, view.y);
}

wlr_box Server::output_box(Output& output, bool usable) const {
    auto* box = wlr_output_layout_get_box(layout_, output.raw);
    if (!box) return {0, 0, 800, 600};
    wlr_box result = *box;
    if (usable) {
        result.x += output.reserved_left;
        result.y += output.reserved_top;
        result.width -= output.reserved_left + output.reserved_right;
        result.height -= output.reserved_top + output.reserved_bottom;
    }
    return result;
}

void Server::apply_output_layout() {
    for (auto& output : outputs_) {
        auto box = output_box(*output, false);
        wlr_scene_rect_set_size(backdrop_, box.width, box.height);
        for (auto& view : views_) {
            if (view->maximized || view->fullscreen) view->place(output_box(*output, true));
        }
        for (auto& layer : layers_) if (layer->mapped) arrange_layer(*layer);
    }
    update_reserved_areas();
}

void Server::update_workspaces() {
    for (std::size_t i = 0; i < workspace_trees_.size(); ++i)
        wlr_scene_node_set_enabled(&workspace_trees_[i]->node, static_cast<int>(i) == current_workspace_);
    if (focused_ && focused_->workspace != current_workspace_) focus_best();
}

void Server::update_reserved_areas() {
    for (auto& output : outputs_) {
        output->reserved_top = output->reserved_right = output->reserved_bottom = output->reserved_left = 0;
        auto* manager = layer_shell_;
        (void)manager;
        for (auto& layer : layers_) {
            if (layer->output != output->raw || !layer->mapped || layer->surface->current.exclusive_zone <= 0) continue;
            const auto& state = layer->surface->current;
            if (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) output->reserved_top += state.exclusive_zone;
            if (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) output->reserved_bottom += state.exclusive_zone;
            if (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) output->reserved_left += state.exclusive_zone;
            if (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) output->reserved_right += state.exclusive_zone;
        }
    }
}

void Server::arrange_layer(LayerSurface& layer) {
    auto* output = layer.output && layer.output->data ? static_cast<Output*>(layer.output->data) : primary_output();
    if (!output) return;
    auto box = output_box(*output, true);
    const auto& state = layer.surface->current;
    int width = state.desired_width;
    int height = state.desired_height;
    const bool horizontal = (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) && (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    const bool vertical = (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) && (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
    if (horizontal) width = box.width;
    if (vertical) height = box.height;
    int x = box.x;
    int y = box.y;
    if (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) x = box.x + box.width - width;
    else if (!(state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) x = box.x + (box.width - width) / 2;
    if (state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) y = box.y + box.height - height;
    else if (!(state.anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) y = box.y + (box.height - height) / 2;
    if (!layer.tree) {
        auto* node = wlr_scene_surface_create(&top_tree_->node, layer.surface->surface);
        if (!node) return;
        // A wlroots scene-surface node has no public tree accessor in 0.15.
        layer.tree = reinterpret_cast<wlr_scene_tree*>(node);
    }
    wlr_scene_node_set_position(&layer.tree->node, x, y);
    wlr_layer_surface_v1_configure(layer.surface, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    layer.owner = {SurfaceOwner::Kind::LayerSurface, &layer.tree->node};
    layer.tree->node.data = &layer.owner;
}

void Server::on_layer_mapped(LayerSurface& layer) {
    arrange_layer(layer);
    if (layer.surface->current.keyboard_interactive) focus_layer(layer);
    update_reserved_areas();
    apply_output_layout();
}

void Server::on_layer_unmapped(LayerSurface& layer) {
    if (focused_layer_ == &layer) focused_layer_ = nullptr;
    update_reserved_areas();
    apply_output_layout();
    focus_best();
}

void Server::remove_layer(LayerSurface& layer) {
    if (focused_layer_ == &layer) focused_layer_ = nullptr;
    std::erase_if(layers_, [&layer](const auto& item) { return item.get() == &layer; });
}

void Server::switch_workspace(int workspace) {
    if (workspace < 0 || workspace >= static_cast<int>(workspace_trees_.size())) return;
    current_workspace_ = workspace;
    update_workspaces();
}

void Server::move_to_workspace(View& view, int workspace) {
    if (workspace < 0 || workspace >= static_cast<int>(workspace_trees_.size())) return;
    if (view.tree) wlr_scene_node_reparent(&view.tree->node, &workspace_trees_[workspace]->node);
    view.workspace = workspace;
    wlr_scene_node_set_enabled(&view.tree->node, workspace == current_workspace_ && view.mapped);
}

void Server::focus_view(View& view, wlr_surface* surface) {
    if (!view.mapped || view.workspace != current_workspace_) return;
    focused_ = &view;
    focused_layer_ = nullptr;
    wlr_scene_node_raise_to_top(&view.tree->node);
    wlr_xdg_toplevel_set_activated(view.surface, true);
    wlr_seat_keyboard_notify_enter(seat_, surface ? surface : view.surface->surface,
        nullptr, 0, nullptr);
}

void Server::focus_layer(LayerSurface& layer) {
    focused_layer_ = &layer;
    focused_ = nullptr;
    wlr_seat_keyboard_notify_enter(seat_, layer.surface->surface, nullptr, 0, nullptr);
}

void Server::focus_best() {
    focused_ = nullptr;
    focused_layer_ = nullptr;
    for (auto& view : views_) if (view->mapped && view->workspace == current_workspace_) { focus_view(*view); return; }
    wlr_seat_keyboard_notify_clear_focus(seat_);
}

void Server::remove_view(View& view) {
    if (focused_ == &view) focused_ = nullptr;
    std::erase_if(views_, [&view](const auto& item) { return item.get() == &view; });
    focus_best();
}

void Server::request_close() { if (focused_) wlr_xdg_toplevel_send_close(focused_->surface); }

void Server::launch(const std::string &command) {
    std::cout << "matcomp: launch " << command << '\n';
    // Spawn on the next idle so key handling never blocks on fork/exec.
    // Ownership of the string stays with the caller until the idle runs; the
    // command literals used by keybindings have static storage duration.
    wl_event_loop_add_idle(event_loop_, start_client, const_cast<char *>(command.c_str()));
}

wlr_box Server::view_box(const View& view) const {
    return {view.x, view.y, static_cast<int>(view.surface->current.geometry.width), static_cast<int>(view.surface->current.geometry.height)};
}

void Server::snap_view(View& view, Snap snap) {
    auto* output = output_for(view);
    if (!output) return;
    const auto box = output_box(*output, true);
    if (!view.snapped && !view.maximized) view.restore_box = view_box(view);
    wlr_box target = box;
    const int gap = config_.window_gap;
    if (snap == Snap::Left) { target.width /= 2; }
    else if (snap == Snap::Right) { target.x += box.width / 2; target.width /= 2; }
    else if (snap == Snap::Up) { target.height /= 2; }
    else if (snap == Snap::Down) { target.y += box.height / 2; target.height /= 2; }
    else if (snap == Snap::Restore && view.restore_box.width > 0) target = view.restore_box;
    target.x += gap; target.y += gap;
    target.width -= gap * 2; target.height -= gap * 2;
    view.maximized = snap == Snap::Maximize || snap == Snap::Up;
    view.snapped = snap != Snap::Restore;
    view.place(target);
}

void Server::toggle_maximize(View& view) { maximize(view, !view.maximized); }
void Server::maximize(View& view, bool enabled) {
    if (!enabled) { view.place(view.restore_box); view.maximized = false; view.snapped = false; }
    else snap_view(view, Snap::Maximize);
}
void Server::toggle_fullscreen(View& view, wlr_output* output) { fullscreen(view, !view.fullscreen, output); }
void Server::fullscreen(View& view, bool enabled, wlr_output* requested) {
    view.fullscreen = enabled;
    if (requested) view.output = requested;
    if (enabled) {
        if (!view.maximized && !view.snapped) view.restore_box = view_box(view);
        if (view.tree) wlr_scene_node_reparent(&view.tree->node, &fullscreen_tree_->node);
        wlr_scene_node_set_enabled(&fullscreen_tree_->node, true);
        wlr_xdg_toplevel_set_fullscreen(view.surface, true);
        auto* output = output_for(view);
        if (output) view.place(output_box(*output, false));
    } else {
        wlr_xdg_toplevel_set_fullscreen(view.surface, false);
        if (view.tree) wlr_scene_node_reparent(&view.tree->node, &workspace_trees_[view.workspace]->node);
        wlr_scene_node_set_enabled(&fullscreen_tree_->node, false);
        view.place(view.restore_box);
    }
}

void Server::begin_interactive(View& view, bool resize, uint32_t edges) {
    if (view.fullscreen) return;
    grabbed_ = &view;
    cursor_mode_ = resize ? CursorMode::Resize : CursorMode::Move;
    grab_x_ = cursor_->x - view.x;
    grab_y_ = cursor_->y - view.y;
    grab_box_ = view_box(view);
    resize_edges_ = edges;
    focus_view(view);
}

void Server::finish_interactive() {
    grabbed_ = nullptr;
    cursor_mode_ = CursorMode::Passthrough;
}

HitResult Server::hit_test(double x, double y) const {
    double sx = 0, sy = 0;
    wlr_scene_node* node = wlr_scene_node_at(&scene_->node, x, y, &sx, &sy);
    if (!node || node->type != WLR_SCENE_NODE_SURFACE) return {};
    auto* surface = wlr_scene_surface_from_node(node);
    if (!surface) return {};
    wlr_scene_node* ancestor = node->parent;
    SurfaceOwner* owner = nullptr;
    while (ancestor) {
        if (ancestor->data) { owner = static_cast<SurfaceOwner*>(ancestor->data); break; }
        ancestor = ancestor->parent;
    }
    return {surface->surface, owner, sx, sy};
}

void Server::on_cursor_button(wlr_event_pointer_button* event) {
    if (event->state == WLR_BUTTON_RELEASED) { finish_interactive(); pointer_frame(); return; }
    const HitResult hit = hit_test(cursor_->x, cursor_->y);
    if (hit.owner && hit.owner->kind == SurfaceOwner::Kind::View) {
        for (auto& view : views_) if (&view->owner == hit.owner) { focus_view(*view, hit.surface); break; }
        if (event->button == BTN_LEFT && (wlr_keyboard_get_modifiers(seat_->keyboard_state.keyboard) & WLR_MODIFIER_LOGO) && focused_)
            begin_interactive(*focused_, false, WLR_EDGE_NONE);
    } else if (hit.owner && hit.owner->kind == SurfaceOwner::Kind::LayerSurface) {
        for (auto& layer : layers_) if (&layer->owner == hit.owner) { focus_layer(*layer); break; }
    } else focus_best();
    wlr_seat_pointer_notify_button(seat_, event->time_msec, event->button, event->state);
}

void Server::process_cursor_motion(uint32_t time) {
    if (grabbed_) {
        if (cursor_mode_ == CursorMode::Move) {
            grabbed_->x = cursor_->x - grab_x_;
            grabbed_->y = cursor_->y - grab_y_;
            if (grabbed_->tree) wlr_scene_node_set_position(&grabbed_->tree->node, grabbed_->x, grabbed_->y);
        } else {
            int width = grab_box_.width + static_cast<int>(cursor_->x - grab_box_.x - grab_x_);
            int height = grab_box_.height + static_cast<int>(cursor_->y - grab_box_.y - grab_y_);
            grabbed_->place({grab_box_.x, grab_box_.y, std::max(32, width), std::max(32, height)});
        }
        return;
    }
    const HitResult hit = hit_test(cursor_->x, cursor_->y);
    if (hit.surface) {
        wlr_seat_pointer_notify_enter(seat_, hit.surface, hit.sx, hit.sy);
        if (config_.focus_follows_mouse && hit.owner && hit.owner->kind == SurfaceOwner::Kind::View)
            for (auto& view : views_) if (&view->owner == hit.owner) focus_view(*view, hit.surface);
    } else {
        wlr_seat_pointer_clear_focus(seat_);
        wlr_xcursor_manager_set_cursor_image(cursor_manager_, "left_ptr", cursor_);
    }
    if (time) wlr_seat_pointer_notify_motion(seat_, time, hit.sx, hit.sy);
}

void Server::pointer_frame() { wlr_seat_pointer_notify_frame(seat_); }

} // namespace matcomp

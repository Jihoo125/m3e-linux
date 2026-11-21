#include "matcomp/window.hpp"
#include "matcomp/server.hpp"
#include "matcomp/backend.hpp"

#include <iostream>
#include <stdexcept>

namespace matcomp {
namespace {
wlr_scene_tree* layer_tree(Server& server, zwlr_layer_shell_v1_layer layer) {
    switch (layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND: return server.background_tree_;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM: return server.bottom_tree_;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP: return server.top_tree_;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY: return server.overlay_tree_;
    }
    throw std::runtime_error("invalid layer-shell layer");
}
}

View::View(Server& owner, wlr_xdg_surface* xdg) : server(owner), surface(xdg) {
    surface->data = this;
    workspace = server.current_workspace();
    if (auto* box = wlr_output_layout_get_box(server.layout_, nullptr)) {
        x = box->x + 32;
        y = box->y + 32;
    }
    map.connect(surface->events.map, [this](void*) { on_map(); });
    unmap.connect(surface->events.unmap, [this](void*) { on_unmap(); });
    destroy.connect(surface->events.destroy, [this](void*) { server.remove_view(*this); });
    commit.connect(surface->surface->events.commit, [this](void*) {
        if (mapped && surface->configured) wlr_scene_node_set_position(&tree->node, x, y);
    });
    request_move.connect(surface->toplevel->events.request_move, [this](void*) {
        server.begin_interactive(*this, false, WLR_EDGE_NONE);
    });
    request_resize.connect(surface->toplevel->events.request_resize, [this](void* data) {
        auto* event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
        server.begin_interactive(*this, true, event->edges);
    });
    request_maximize.connect(surface->toplevel->events.request_maximize, [this](void*) {
        if (mapped) server.toggle_maximize(*this);
    });
    request_fullscreen.connect(surface->toplevel->events.request_fullscreen, [this](void* data) {
        if (!mapped) return;
        auto* event = static_cast<wlr_xdg_toplevel_set_fullscreen_event*>(data);
        server.fullscreen(*this, event->fullscreen, event->output);
    });
    new_popup.connect(surface->events.new_popup, [this](void* data) {
        new Popup(server, server.popup_parent(), static_cast<wlr_xdg_surface*>(data));
    });
}

View::~View() {
    if (surface) surface->data = nullptr;
    if (tree) wlr_scene_node_destroy(&tree->node);
}

void View::on_map() {
    if (mapped) return;
    if (!tree) {
        auto* node = wlr_scene_xdg_surface_create(&server.workspace_trees_[workspace]->node, surface);
        if (!node) throw std::runtime_error("cannot create xdg scene tree");
        tree = reinterpret_cast<wlr_scene_tree*>(node);
        owner = {SurfaceOwner::Kind::View, &tree->node};
        tree->node.data = &owner;
    }
    std::cout << "matcomp: mapped xdg surface with " << surface->surface->current.width << "x" << surface->surface->current.height << " buffer\n";
    mapped = true;
    wlr_scene_node_set_enabled(&tree->node, true);
    place({x, y, 0, 0});
    restore_box = server.view_box(*this);
    server.update_workspaces();
    if (auto* output = server.primary_output(); output && !wlr_output_is_headless(output->raw))
        wlr_output_schedule_frame(output->raw);
}

void View::on_unmap() {
    mapped = false;
    if (tree) wlr_scene_node_set_enabled(&tree->node, false);
    if (server.focused_view() == this) server.focus_best();
    server.update_workspaces();
}

void View::place(const wlr_box& geometry) {
    wlr_box client_geometry;
    wlr_xdg_surface_get_geometry(surface, &client_geometry);
    x = geometry.x - client_geometry.x;
    y = geometry.y - client_geometry.y;
    if (tree) wlr_scene_node_set_position(&tree->node, x, y);
    if (geometry.width > 0 && geometry.height > 0)
        wlr_xdg_toplevel_set_size(surface, static_cast<uint32_t>(geometry.width), static_cast<uint32_t>(geometry.height));
}

Popup::Popup(Server& server, wlr_scene_node* parent, wlr_xdg_surface* xdg) : surface(xdg), tree(nullptr) {
    auto* node = wlr_scene_xdg_surface_create(parent, surface);
    if (!node) throw std::runtime_error("cannot create popup scene tree");
    tree = reinterpret_cast<wlr_scene_tree*>(node);
    int anchor_x = 0, anchor_y = 0;
    wlr_xdg_popup_get_anchor_point(surface->popup, &anchor_x, &anchor_y);
    wlr_scene_node_set_position(&tree->node, anchor_x, anchor_y);
    destroy.connect(surface->events.destroy, [this](void*) { delete this; });
    new_popup.connect(surface->events.new_popup, [&server, this](void* data) {
        new Popup(server, &tree->node, static_cast<wlr_xdg_surface*>(data));
    });
}

Popup::~Popup() {
    if (tree) wlr_scene_node_destroy(&tree->node);
}

LayerSurface::LayerSurface(Server& owner, wlr_layer_surface_v1* layer, wlr_output* selected)
        : server(owner), surface(layer), output(selected), scene_layer(layer->pending.layer) {
    if (!output) {
        auto* primary = server.primary_output();
        output = primary ? primary->raw : nullptr;
    }
    if (!output) throw std::runtime_error("layer surface created without an output");
    surface->output = output;
    surface->data = this;
    tree = layer_tree(server, surface->pending.layer);
    output_destroy.connect(output->events.destroy, [this](void*) {
        output_destroy.reset();
        output = nullptr;
        if (surface && surface->mapped) wlr_layer_surface_v1_destroy(surface);
    });
    map.connect(surface->events.map, [this](void*) { mapped = true; server.on_layer_mapped(*this); });
    unmap.connect(surface->events.unmap, [this](void*) { mapped = false; server.on_layer_unmapped(*this); });
    destroy.connect(surface->events.destroy, [this](void*) { server.remove_layer(*this); });
    commit.connect(surface->surface->events.commit, [this](void*) { server.arrange_layer(*this); });
    new_popup.connect(surface->events.new_popup, [this](void* data) {
        new Popup(server, server.popup_parent(), static_cast<wlr_xdg_surface*>(data));
    });
}

LayerSurface::~LayerSurface() {
    if (surface) surface->data = nullptr;
}

} // namespace matcomp

#include "matcomp/backend.hpp"
#include "matcomp/server.hpp"
#include "matcomp/window.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>

namespace matcomp {

Output::Output(Server& owner, wlr_output* output) : server(owner), raw(output) {
    raw->data = this;
    frame->connect(raw->events.frame, [this](void*) { render(); });
    destroy->connect(raw->events.destroy, [this](void*) {
        frame->reset();
        destroy->reset();
        auto* raw_output = raw;
        raw->data = nullptr;
        for (auto& layer : server.layers_) {
            if (layer->output == raw_output) {
                layer->output_destroy.reset();
                layer->output = nullptr;
                if (layer->surface->mapped) wlr_layer_surface_v1_destroy(layer->surface);
            }
        }
        for (auto& view : server.views_) {
            if (view->output == raw_output) {
                view->output = nullptr;
                if (view->fullscreen) server.fullscreen(*view, false);
            }
        }
        // The output wrapper may be deleted while wlroots is still emitting the
        // same destroy signal. Release it only after its own destroy listeners
        // are detached and server collections have forgotten it.
        auto found = std::find_if(server.outputs_.begin(), server.outputs_.end(),
            [this](const auto& output) { return output.get() == this; });
        if (found != server.outputs_.end()) {
            (void)found->release();
            server.outputs_.erase(found);
        }
        server.apply_output_layout();
        delete this;
    });
}

void Output::render() {
    ++frame_count;
    if (frame_count <= 5)
        std::cout << "matcomp: render enter renderer=" << (raw->renderer != nullptr)
                  << " frame_pending=" << raw->frame_pending
                  << " enabled=" << raw->enabled << '\n';
    // Headless smoke-test outputs need no presentation. Real DRM outputs are
    // initialized with the renderer in Server::add_output before this callback.
    if (!raw->renderer) return;
    // A modeset/page-flip may still be in flight; committing now would fail
    // wlroots' basic output test and leak the attached back buffer.
    if (raw->frame_pending) return;
    auto* scene_output = wlr_scene_get_scene_output(server.scene(), raw);
    if (!scene_output) return;
    const bool committed = wlr_scene_output_commit(scene_output);
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
    if (!committed) {
        wlr_output_rollback(raw);
        wlr_output_schedule_frame(raw);
    }
    if (frame_count <= 3 || !committed)
        std::cout << "matcomp: output frame " << (raw->name ? raw->name : "unknown") << " committed=" << committed << '\n';
}

Keyboard::Keyboard(Server& owner, wlr_input_device* input) : server(owner), device(input) {
    xkb_rule_names names{};
    names.rules = nullptr;
    names.model = server.config().xkb_model.empty() ? nullptr : server.config().xkb_model.c_str();
    names.layout = server.config().xkb_layout.empty() ? nullptr : server.config().xkb_layout.c_str();
    names.variant = server.config().xkb_variant.empty() ? nullptr : server.config().xkb_variant.c_str();
    names.options = server.config().xkb_options.empty() ? nullptr : server.config().xkb_options.c_str();
    auto* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!context) throw std::runtime_error("failed to create XKB context");
    auto* keymap = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap || !wlr_keyboard_set_keymap(device->keyboard, keymap)) {
        if (keymap) xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        throw std::runtime_error("failed to configure XKB keymap");
    }
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, server.config().repeat_rate, server.config().repeat_delay);

    key.connect(device->keyboard->events.key, [this](void* data) {
        auto* event = static_cast<wlr_event_keyboard_key*>(data);
        const uint32_t keycode = event->keycode + 8;
        const xkb_keysym_t* keysyms = nullptr;
        const int count = xkb_state_key_get_syms(device->keyboard->xkb_state, keycode, &keysyms);
        const uint32_t modifiers = wlr_keyboard_get_modifiers(device->keyboard);
        bool handled = false;
        if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
            for (int i = 0; i < count && !handled; ++i)
                handled = handle_binding(keycode, keysyms[i], modifiers);
        }
        if (!handled) {
            wlr_seat_set_keyboard(server.seat_, device);
            wlr_seat_keyboard_notify_key(server.seat_, event->time_msec, event->keycode, event->state);
        }
    });
    modifiers.connect(device->keyboard->events.modifiers, [this](void*) {
        wlr_seat_set_keyboard(server.seat_, device);
        wlr_seat_keyboard_notify_modifiers(server.seat_, &device->keyboard->modifiers);
    });
    destroy.connect(device->events.destroy, [this](void*) {
        std::erase_if(server.keyboards_, [this](const auto& keyboard) { return keyboard.get() == this; });
        uint32_t capabilities = WL_SEAT_CAPABILITY_POINTER;
        if (!server.keyboards_.empty()) capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
        wlr_seat_set_capabilities(server.seat_, capabilities);
    });
    wlr_seat_set_keyboard(server.seat_, device);
}

bool Keyboard::handle_binding(uint32_t keycode, xkb_keysym_t keysym, uint32_t modifiers) {
    const bool super = (modifiers & WLR_MODIFIER_LOGO) != 0;
    const bool alt = (modifiers & WLR_MODIFIER_ALT) != 0;
    const bool ctrl = (modifiers & WLR_MODIFIER_CTRL) != 0;
    if (ctrl && alt && (keysym == XKB_KEY_t || keysym == XKB_KEY_T)) {
        server.launch("matterminal");
        return true;
    }
    if (ctrl && alt && (keysym == XKB_KEY_b || keysym == XKB_KEY_B)) {
        server.launch("matbrowser");
        return true;
    }
    if (alt && (keysym == XKB_KEY_Escape || keycode == XKB_KEY_F4)) {
        wl_display_terminate(server.display_);
        return true;
    }
    if (super && !server.focused_view()) return false;
    if (super) {
        switch (keysym) {
        case XKB_KEY_Left: server.snap_view(*server.focused_view(), Snap::Left); return true;
        case XKB_KEY_Right: server.snap_view(*server.focused_view(), Snap::Right); return true;
        case XKB_KEY_Up: server.snap_view(*server.focused_view(), Snap::Up); return true;
        case XKB_KEY_Down: server.snap_view(*server.focused_view(), Snap::Down); return true;
        case XKB_KEY_m: case XKB_KEY_M: server.toggle_maximize(*server.focused_view()); return true;
        case XKB_KEY_f: case XKB_KEY_F: server.toggle_fullscreen(*server.focused_view()); return true;
        case XKB_KEY_q: case XKB_KEY_Q: server.request_close(); return true;
        case XKB_KEY_1: case XKB_KEY_2: case XKB_KEY_3: case XKB_KEY_4:
            server.switch_workspace(static_cast<int>(keysym - XKB_KEY_1)); return true;
        case XKB_KEY_exclam: server.move_to_workspace(*server.focused_view(), 0); return true;
        case XKB_KEY_at: server.move_to_workspace(*server.focused_view(), 1); return true;
        case XKB_KEY_numbersign: server.move_to_workspace(*server.focused_view(), 2); return true;
        case XKB_KEY_dollar: server.move_to_workspace(*server.focused_view(), 3); return true;
        default: break;
        }
    }
    return false;
}

} // namespace matcomp

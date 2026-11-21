#pragma once

#include "matcomp/listener.hpp"
#include <memory>
#include <utility>

namespace matcomp { class Server; }

namespace matcomp {

struct Output {
    Server& server;
    wlr_output* raw;
    std::unique_ptr<Listener> frame{std::make_unique<Listener>()};
    std::unique_ptr<Listener> destroy{std::make_unique<Listener>()};
    int reserved_top{0};
    int reserved_right{0};
    int reserved_bottom{0};
    int reserved_left{0};
    uint64_t frame_count{0};

    Output(Server& server, wlr_output* raw);
    void render();
};

struct Keyboard {
    Server& server;
    wlr_input_device* device;
    Listener key;
    Listener modifiers;
    Listener destroy;

    Keyboard(Server& server, wlr_input_device* device);
    bool handle_binding(uint32_t keycode, xkb_keysym_t keysym, uint32_t modifiers);
};

} // namespace matcomp

#pragma once

#include "matcomp/types.hpp"
#include "matcomp/listener.hpp"
#include <memory>
#include <utility>

namespace matcomp { class Server; struct Output; enum class Snap; }

namespace matcomp {

struct View {
    Server& server;
    wlr_xdg_surface* surface;
    SurfaceOwner owner{};
    wlr_scene_tree* tree{nullptr};
    bool mapped{false};
    bool maximized{false};
    bool fullscreen{false};
    bool snapped{false};
    int workspace{0};
    int x{0};
    int y{0};
    wlr_output* output{nullptr};
    wlr_box restore_box{};

    Listener map;
    Listener unmap;
    Listener destroy;
    Listener commit;
    Listener request_move;
    Listener request_resize;
    Listener request_maximize;
    Listener request_fullscreen;
    Listener new_popup;

    View(Server& server, wlr_xdg_surface* surface);
    ~View();
    void on_map();
    void on_unmap();
    void place(const wlr_box& geometry);
};

struct Popup {
    wlr_xdg_surface* surface;
    wlr_scene_tree* tree;
    Listener destroy;
    Listener new_popup;
    Popup(Server& server, wlr_scene_node* parent, wlr_xdg_surface* surface);
    ~Popup();
};

struct LayerSurface {
    Server& server;
    wlr_layer_surface_v1* surface;
    wlr_output* output;
    SurfaceOwner owner{};
    wlr_scene_tree* tree{nullptr};
    bool mapped{false};
    zwlr_layer_shell_v1_layer scene_layer;

    Listener map;
    Listener unmap;
    Listener destroy;
    Listener commit;
    Listener new_popup;
    Listener output_destroy;

    LayerSurface(Server& server, wlr_layer_surface_v1* surface, wlr_output* output);
    ~LayerSurface();
};

} // namespace matcomp

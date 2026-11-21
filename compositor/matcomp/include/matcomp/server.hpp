#pragma once

#include "matcomp/config.hpp"
#include "matcomp/listener.hpp"
#include "matcomp/types.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace matcomp {

struct Keyboard;
enum class Snap { Restore, Left, Right, Up, Down, Maximize };

class Server {
public:
    explicit Server(Config config);
    ~Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void initialize();
    void run(const std::string& startup_command);
    bool self_test();

    const Config& config() const { return config_; }
    wlr_scene* scene() const { return scene_; }
    wlr_scene_node* popup_parent() const { return &popups_->node; }
    wlr_box output_box(Output& output, bool usable) const;
    Output* primary_output() const;
    Output* output_at(double x, double y) const;
    Output* output_for(const View& view) const;
    const std::vector<std::unique_ptr<Output>>& outputs() const { return outputs_; }

    void apply_output_layout();
    void update_workspaces();
    void update_reserved_areas();
    void arrange_layer(LayerSurface& layer);
    void switch_workspace(int workspace);
    int current_workspace() const { return current_workspace_; }
    void move_to_workspace(View& view, int workspace);
    void request_close();
    void launch(const std::string &command);
    void focus_view(View& view, wlr_surface* surface = nullptr);
    void focus_layer(LayerSurface& layer);
    void focus_best();
    View* focused_view() const { return focused_; }
    void snap_view(View& view, Snap snap);
    void toggle_maximize(View& view);
    void toggle_fullscreen(View& view, wlr_output* requested = nullptr);
    void maximize(View& view, bool enabled);
    void fullscreen(View& view, bool enabled, wlr_output* requested = nullptr);
    wlr_box view_box(const View& view) const;

    void begin_interactive(View& view, bool resize, uint32_t edges);
    void process_cursor_motion(uint32_t time);
    HitResult hit_test(double x, double y) const;
    void pointer_frame();
    void on_layer_mapped(LayerSurface& layer);
    void on_layer_unmapped(LayerSurface& layer);
    void remove_view(View& view);
    void remove_layer(LayerSurface& layer);

    wl_display* display_{nullptr};
    wlr_backend* backend_{nullptr};
    wlr_renderer* renderer_{nullptr};
    wlr_allocator* allocator_{nullptr};
    wlr_scene* scene_{nullptr};
    wlr_output_layout* layout_{nullptr};
    wlr_seat* seat_{nullptr};
    wlr_cursor* cursor_{nullptr};
    wlr_xcursor_manager* cursor_manager_{nullptr};
    wlr_xdg_shell* xdg_shell_{nullptr};
    wlr_layer_shell_v1* layer_shell_{nullptr};
    wlr_xdg_decoration_manager_v1* decorations_{nullptr};

    wlr_scene_tree* background_tree_{nullptr};
    wlr_scene_tree* bottom_tree_{nullptr};
    std::array<wlr_scene_tree*, 4> workspace_trees_{};
    wlr_scene_tree* fullscreen_tree_{nullptr};
    wlr_scene_rect* backdrop_{nullptr};
    wlr_scene_tree* top_tree_{nullptr};
    wlr_scene_tree* overlay_tree_{nullptr};
    wlr_scene_tree* popups_{nullptr};

    std::vector<std::unique_ptr<Output>> outputs_;
    std::vector<std::unique_ptr<View>> views_;
    std::vector<std::unique_ptr<LayerSurface>> layers_;
    std::vector<std::unique_ptr<Keyboard>> keyboards_;

private:
    enum class CursorMode { Passthrough, Move, Resize };
    struct Workspace {
        wlr_scene_tree* tree{nullptr};
        LayerSurface* exclusive_layer{nullptr};
        View* last_focus{nullptr};
    };

    void create_backend();
    void create_protocols();
    void add_output(wlr_output* raw);
    void add_input(wlr_input_device* device);
    void add_view(wlr_xdg_surface* surface);
    void add_layer(wlr_layer_surface_v1* surface);
    void on_cursor_button(wlr_event_pointer_button* event);
    void update_exclusive_focus();
    void finish_interactive();

    Config config_;
    std::array<Workspace, 4> workspaces_{};
    int current_workspace_{0};
    View* focused_{nullptr};
    LayerSurface* focused_layer_{nullptr};

    CursorMode cursor_mode_{CursorMode::Passthrough};
    View* grabbed_{nullptr};
    double grab_x_{0.0};
    double grab_y_{0.0};
    wlr_box grab_box_{};
    uint32_t resize_edges_{0};

    std::vector<std::unique_ptr<Listener>> listeners_;
    std::unique_ptr<xkb_context, decltype(&xkb_context_unref)> xkb_context_{nullptr, xkb_context_unref};
    wl_event_loop* event_loop_{nullptr};
    wl_event_source* test_timeout_{nullptr};
    bool test_passed_{false};
};

} // namespace matcomp

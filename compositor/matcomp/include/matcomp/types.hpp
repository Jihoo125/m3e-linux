#pragma once

#include "matcomp/wlr_compat.hpp"

namespace matcomp {

struct Output;
struct View;
struct LayerSurface;
struct Server;

// This tag is attached only to the root node of a managed surface tree. Scene
// hit-testing walks up to it to recover the owning view/layer and any popups.
struct SurfaceOwner {
    enum class Kind { View, LayerSurface } kind;
    wlr_scene_node* tree{nullptr};
};

struct HitResult {
    wlr_surface* surface{nullptr};
    SurfaceOwner* owner{nullptr};
    double sx{0.0};
    double sy{0.0};
};

} // namespace matcomp

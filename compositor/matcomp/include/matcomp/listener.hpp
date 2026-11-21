#pragma once

#include "matcomp/wlr_compat.hpp"

#include <functional>

namespace matcomp {

// Owns a wl_signal link, but never owns the emitter. Sources with a longer
// lifetime than their users must call reset() before the callback's target dies.
class Listener {
public:
    Listener() = default;
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;
    ~Listener() { reset(); }

    template<typename F>
    void connect(wl_signal& signal, F&& callback) {
        reset();
        callback_ = std::forward<F>(callback);
        listener_.notify = &dispatch;
        wl_signal_add(&signal, &listener_);
        connected_ = true;
    }

    void reset() {
        if (connected_) {
            wl_list_remove(&listener_.link);
            connected_ = false;
        }
        callback_ = {};
    }

private:
    static void dispatch(wl_listener* listener, void* data) {
        Listener* self = wl_container_of(listener, self, listener_);
        if (self->callback_) {
            self->callback_(data);
        }
    }

    wl_listener listener_{};
    std::function<void(void*)> callback_;
    bool connected_{false};
};

} // namespace matcomp

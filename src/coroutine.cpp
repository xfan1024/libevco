#include <evco/coroutine.h>

#include <thread>

#include "evco_internal.h"

namespace evco {

void init(struct ev_loop *loop) {
    current_loop(loop);
}

void deinit() {
    current_loop_clear();
}

class CoroutineHelper {
public:
    static void check_running_state(Coroutine *co, bool expected, const char *func, int line) {
        if (co->is_running() != expected) {
            fprintf(stderr, "[%s:%d] coroutine is %s, should report to developer to fix\n", func, line, expected ? "not running" : "running");
            abort();
        }
    };

    static void check_pending_state(Coroutine *co, bool expected, const char *func, int line) {
        if (co->pending_ != expected) {
            fprintf(stderr, "[%s:%d] coroutine is %s, should report to developer to fix\n", func, line, expected ? "not pending" : "pending");
            abort();
        }
    };
};

Coroutine::Coroutine() {
    char buf[32];
    snprintf(buf, sizeof(buf), "Coroutine<%p>", (void *)this);
    name_ = buf;
}

Coroutine ::~Coroutine() {
    CoroutineHelper::check_running_state(this, false, __func__, __LINE__);
    assert(finish_callback_ == nullptr);
}

void Coroutine::start() {
    CoroutineHelper::check_running_state(this, false, __func__, __LINE__);
    source_hoder_.emplace([&](SinkType &sink) {
        sink_ptr_ = &sink;
        // cannot call yield() directly, the source_holder_ is not initialized yet
        // so, is_running() will return false. yield requires is_running() to be true
        pending_ = true;
        sink();
        if (interrupted_) {
            return;
        }
        entry();
    });
    resume();
}

void Coroutine::resume() {
    SourceType &source = this->source();
    CoroutineHelper::check_running_state(this, true, __func__, __LINE__);
    CoroutineHelper::check_pending_state(this, true, __func__, __LINE__);
    pending_ = false;
    Coroutine *old = current();
    if (old == this) {
        fprintf(stderr, "[%s:%d] coroutine is already running, should report to developer to fix\n", __func__, __LINE__);
        abort();
    }
    if (old) {
        old->pending_ = true;
    }
    current(this);
    assert(source);
    source();
    if (old) {
        old->pending_ = false;
    }
    current(old);
    if (source) {
        CoroutineHelper::check_pending_state(this, true, __func__, __LINE__);
    } else {
        source_hoder_.reset();
        sink_ptr_ = nullptr;
        if (finish_callback_) {
            auto finish = std::move(finish_callback_);
            finish_callback_ = nullptr;
            finish(this);
            // cannot operate on this after finish is called
            // because finish may delete this
        }
    }
}

void Coroutine::yield() {
    SinkType &sink = this->sink();
    CoroutineHelper::check_running_state(this, true, __func__, __LINE__);
    CoroutineHelper::check_pending_state(this, false, __func__, __LINE__);
    pending_ = true;
    current(nullptr);
    sink();
    CoroutineHelper::check_pending_state(this, false, __func__, __LINE__);
}

void Coroutine::interrupt() {
    interrupted_ = true;
    if (current() == this) {
        // just set interrupted_ to true
        return;
    }
    if (is_running()) {
        resume();
        if (is_running()) {
            fprintf(stderr, "[%s:%d] coroutine is still running, should report to developer to fix\n", __func__, __LINE__);
            abort();
        }
    }
}

bool Coroutine::is_running() {
    return source_hoder_.has_value();
}

bool Coroutine::is_interrupted() {
    return interrupted_;
}

void Coroutine::set_finish_callback(std::function<void(Coroutine *)> fn) {
    finish_callback_ = std::move(fn);
}

Coroutine::SourceType &Coroutine::source() {
    CoroutineHelper::check_running_state(this, true, __func__, __LINE__);
    assert(source_hoder_.has_value());
    return source_hoder_.value();
}

Coroutine::SinkType &Coroutine::sink() {
    CoroutineHelper::check_running_state(this, true, __func__, __LINE__);
    assert(sink_ptr_);
    return *sink_ptr_;
}

}  // namespace evco

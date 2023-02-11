#include <evco/coroutine.h>

namespace evco {

Core::Core(struct ev_loop *loop) {
    loop_ = loop;
}

struct ev_loop *Core::get_loop() {
    return loop_;
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
            fprintf(stderr, "[%s:%d] coroutine is %s, should report to developer to fix\n", func, line, expected ? "not running" : "running");
            abort();
        }
    };
};

Coroutine::Coroutine() {
}

Coroutine ::~Coroutine() {
    CoroutineHelper::check_running_state(this, false, __func__, __LINE__);
    assert(finish_callback_ == nullptr);
}

Core *Coroutine::core() {
    CoroutineHelper::check_running_state(this, true, __func__, __LINE__);
    assert(core_ptr_);
    return core_ptr_;
}

void Coroutine::start(Core *core) {
    CoroutineHelper::check_running_state(this, false, __func__, __LINE__);
    source_hoder_.emplace([&](SinkType &sink) {
        sink_ptr_ = &sink;
        core_ptr_ = core;
        pending_ = false;
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
    source();
    if (source) {
        CoroutineHelper::check_pending_state(this, true, __func__, __LINE__);
    } else {
        source_hoder_.reset();
        sink_ptr_ = nullptr;
        core_ptr_ = nullptr;
        if (finish_callback_) {
            auto finish = std::move(finish_callback_);
            finish_callback_ = nullptr;
            finish();
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
    sink();
    CoroutineHelper::check_pending_state(this, false, __func__, __LINE__);
}

void Coroutine::interrupt() {
    interrupted_ = true;

    if (is_running()) {
        resume();
    }
}

bool Coroutine::is_running() {
    return source_hoder_.has_value();
}

bool Coroutine::is_interrupted() {
    return interrupted_;
}

void Coroutine::set_finish_callback(std::function<void(void)> fn) {
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
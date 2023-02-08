#ifndef __evco_evco_h__
#define __evco_evco_h__

#include <assert.h>
#include <ev.h>

#include <boost/coroutine2/all.hpp>
#include <functional>
#include <optional>

namespace evco {

struct CoroutineContext {
    struct ev_loop *loop;
    std::function<void()> yield;
    std::function<void()> resume;
    bool interrupted{false};
    bool pending{false};
};

template <class CoroutineImpl>
class Coroutine : public CoroutineImpl {
public:
    template <typename... Args>
    Coroutine(Args &&...args) : CoroutineImpl(std::forward<Args>(args)...) {
    }

    ~Coroutine() {
        if (running_) {
            fprintf(stderr, "%s: coroutine is running, should report to developer to fix\n", __func__);
            abort();
        }
        assert(!finish_);
    }

    template <typename Fn>
    void set_finish_callback(Fn &&finish_callback) {
        finish_ = std::forward<Fn>(finish_callback);
    }

    void start(struct ev_loop *loop) {
        source_hoder_.emplace([&](boost::coroutines2::coroutine<void>::push_type &sink) {
            assert(!running_);
            running_ = true;
            context_.loop = loop;
            context_.yield = [&]() {
                if (context_.pending) {
                    fprintf(stderr, "%s: coroutine is pending, should report to developer to fix\n", __func__);
                    abort();
                }
                context_.pending = true;
                sink();
            };
            context_.resume = [&]() {
                auto &source = *source_hoder_;
                if (!context_.pending) {
                    fprintf(stderr, "%s: coroutine is not pending, should report to developer to fix\n", __func__);
                    abort();
                }
                context_.pending = false;
                source();
                if (!source) {
                    running_ = false;
                    if (finish_) {
                        auto finish = std::move(finish_);
                        finish_ = nullptr;
                        finish(this);
                        // cannot operate on this after finish is called
                        // because finish_ may delete this
                    }
                }
            };
            CoroutineImpl::entry(&context_);
        });
    }

    // do not call this inside this coroutine
    void interrupt() {
        context_.interrupted = true;
        context_.resume();
    }

private:
    bool running_{false};
    std::function<void(Coroutine<CoroutineImpl> *self)> finish_;
    CoroutineContext context_;
    std::optional<boost::coroutines2::coroutine<void>::pull_type> source_hoder_;
};

}  // namespace evco

#endif  // __evco_evco_h__

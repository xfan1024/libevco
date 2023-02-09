#ifndef __evco_evco_h__
#define __evco_evco_h__

#include <assert.h>
#include <ev.h>

#include <boost/coroutine2/all.hpp>
#include <functional>
#include <optional>

namespace evco {

struct Context {
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
        if (source_hoder_) {
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
        assert(!source_hoder_);
        source_hoder_.emplace([&](boost::coroutines2::coroutine<void>::push_type &sink) {
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
                    source_hoder_.reset();
                    if (finish_) {
                        auto finish = std::move(finish_);
                        finish_ = nullptr;
                        finish();
                        // cannot operate on this after finish is called
                        // because finish_ may delete this
                    }
                }
            };
            context_.interrupted = false;
            context_.pending = false;
            CoroutineImpl::entry(&context_);
        });
    }

    bool is_running() {
        return source_hoder_.has_value();
    }

    // do not call this inside this coroutine
    void interrupt() {
        if (source_hoder_) {
            context_.interrupted = true;
            context_.resume();
        }
    }

private:
    std::function<void()> finish_;
    Context context_;
    std::optional<boost::coroutines2::coroutine<void>::pull_type> source_hoder_;
};

}  // namespace evco

#endif  // __evco_evco_h__

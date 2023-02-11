#ifndef __evco_coroutine_h__
#define __evco_coroutine_h__

#include <ev.h>

#include <boost/coroutine2/all.hpp>
#include <functional>
#include <optional>

namespace evco {

class Core {
public:
    Core(struct ev_loop *loop);
    Core(const Core &) = delete;
    Core &operator=(const Core &) = delete;

    struct ev_loop *get_loop();

private:
    struct ev_loop *loop_;
};

class Coroutine {
public:
    Coroutine();

    // I don't think the `vritual` is necessary here.
    // application shouldn't delete this base class pointer intead of derived class pointer.
    // add `virtual` here just to pass g++'s almost perverted code checks [-Werror=delete-non-virtual-dtor]
    virtual ~Coroutine();
    Coroutine(const Coroutine &) = delete;
    Coroutine &operator=(const Coroutine &) = delete;

    Core *core();

    void start(Core *core);
    void resume();
    void yield();
    void interrupt();
    bool is_running();
    bool is_interrupted();
    void set_finish_callback(std::function<void(void)> fn);

protected:
    virtual void entry() = 0;

private:
    typedef boost::coroutines2::coroutine<void>::pull_type SourceType;
    typedef boost::coroutines2::coroutine<void>::push_type SinkType;

    SourceType &source();
    SinkType &sink();

    std::optional<SourceType> source_hoder_;
    SinkType *sink_ptr_{nullptr};
    Core *core_ptr_{nullptr};
    std::function<void(void)> finish_callback_;
    bool interrupted_{false};
    bool pending_{false};

    friend class CoroutineHelper;
};

}  // namespace evco

#endif  // __evco_coroutine_h__

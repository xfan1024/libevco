#ifndef __evco_coroutine_h__
#define __evco_coroutine_h__

#include <ev.h>

#include <boost/coroutine2/all.hpp>
#include <functional>
#include <optional>

namespace evco {

void init(struct ev_loop *loop);
void deinit();

class Coroutine {
public:
    Coroutine();

    // I don't think the `virtual` is necessary here.
    // application shouldn't delete this base class pointer intead of derived class pointer.
    // add `virtual` here just to pass g++'s almost perverted code checks [-Werror=delete-non-virtual-dtor]
    virtual ~Coroutine();
    Coroutine(const Coroutine &) = delete;
    Coroutine &operator=(const Coroutine &) = delete;

    void start();
    void resume();
    void yield();
    void interrupt();
    bool is_running();
    bool is_interrupted();
    void set_finish_callback(std::function<void(Coroutine *)> fn);
    void set_name(const std::string &name) {
        name_ = name;
    }
    const std::string &get_name() {
        return name_;
    }

protected:
    virtual void entry() = 0;

private:
    typedef boost::coroutines2::coroutine<void>::pull_type SourceType;
    typedef boost::coroutines2::coroutine<void>::push_type SinkType;

    SourceType &source();
    SinkType &sink();

    std::string name_;
    std::optional<SourceType> source_hoder_;
    SinkType *sink_ptr_{nullptr};
    std::function<void(Coroutine *)> finish_callback_;
    bool interrupted_{false};
    bool pending_{false};

    friend class CoroutineHelper;
};

}  // namespace evco

#endif  // __evco_coroutine_h__

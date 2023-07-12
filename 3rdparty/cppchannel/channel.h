#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

enum ChannelStatus
{
    CHANNEL_OK,
    CHANNEL_CLOSED,
    CHANNEL_NOT_READY,
};

class ChannelImpl;

template <typename T> class ChannelIter;

template <typename T> class Channel
{
  public:
    typedef T value_type;
    typedef ChannelIter<T> iterator;

  public:
    explicit Channel(size_t max_buffer_size = 1);

    void close();

    bool operator<<(const T &object);

    bool input(const T &object);

    bool input_and_close(const T &object);

    ChannelStatus try_input(const T &object);

    bool operator>>(T &object);

    bool operator>>(T &&object);

    bool output(T &object);

    bool output(T &&object);

    ChannelStatus try_output(T &object);

    ChannelStatus try_output(T &&object);

    size_t size() const;

    size_t capacity() const;

    iterator begin();

    iterator end();

  private:
    friend class ChannelSelect;
    std::shared_ptr<ChannelImpl> m;
};

template <typename T> class IChannel
{
  public:
    typedef T value_type;

  public:
    explicit IChannel(size_t max_buffer_size = 0) : m(max_buffer_size)
    {
    }

    explicit IChannel(const Channel<T> &chan) : m(chan)
    {
    }

    IChannel &operator=(const Channel<T> &chan)
    {
        m = chan;
        return *this;
    }

    bool operator<<(const T &object)
    {
        return m << object;
    }

    bool input(const T &object)
    {
        return m.input(object);
    }

    ChannelStatus try_input(const T &object)
    {
        return m.try_input(object);
    }

    void close()
    {
        m.close();
    }

    size_t size() const
    {
        return m.size();
    }

    size_t capacity() const
    {
        return m.capacity();
    }

  private:
    friend class ChannelSelect;
    Channel<T> m;
};

template <typename T> class OChannel
{
  public:
    typedef T value_type;
    typedef ChannelIter<T> iterator;

  public:
    explicit OChannel(size_t max_buffer_size = 0) : m(max_buffer_size)
    {
    }

    explicit OChannel(const Channel<T> &chan) : m(chan)
    {
    }

    OChannel &operator=(const Channel<T> &chan)
    {
        m = chan;
        return *this;
    }

    bool operator>>(T &object)
    {
        return m >> object;
    }

    bool operator>>(T &&object)
    {
        return m >> object;
    }

    bool output(T &object)
    {
        return m.output(object);
    }

    bool output(T &&object)
    {
        return m.output(object);
    }

    ChannelStatus try_output(T &object)
    {
        return try_output(object);
    }

    ChannelStatus try_output(T &&object)
    {
        return try_output(object);
    }

    void close()
    {
        m.close();
    }

    size_t size() const
    {
        return m.size();
    }

    size_t capacity() const
    {
        return m.capacity();
    }

    iterator begin()
    {
        return m.begin();
    }

    iterator end()
    {
        return m.end();
    }

  private:
    friend class ChannelSelect;
    friend class ChannelTimer;
    Channel<T> m;
};

class ChannelTimer
{
  public:
    explicit ChannelTimer(int64_t timeout_ms = 0) : sleep_timeout_ms(timeout_ms)
    {
        reset(sleep_timeout_ms);
    }

    ~ChannelTimer()
    {
        event.notify_all();
        if (thread.joinable())
        {
            thread.join();
        }
    }

    void reset(int64_t timeout_ms = 0)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (status == SLEEPING)
        {
            event.notify_all();
        }
        else if (status == DEFAULT)
        {
            status = RESET;
        }
        lock.unlock();
        if (thread.joinable())
        {
            thread.join();
        }
        status = DEFAULT;
        c = Channel<bool>(1);
        sleep_timeout_ms = timeout_ms;
        if (sleep_timeout_ms <= 0)
        {
            awake();
        }
        else
        {
            thread = std::thread(&ChannelTimer::sleep, this);
        }
    }

    void wait()
    {
        c >> true;
    }

    OChannel<bool> c;

  private:
    void awake()
    {
        status = DEFAULT;
        c.m << true;
        c.close();
    }

    void sleep()
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (sleep_timeout_ms <= 0 || status == RESET)
        {
            awake();
            return;
        }
        status = SLEEPING;
        event.wait_for(lock, std::chrono::milliseconds(sleep_timeout_ms));
        awake();
    }

    enum
    {
        DEFAULT,
        SLEEPING,
        RESET,
    };

  private:
    std::mutex mutex;
    std::thread thread;
    int status = DEFAULT;
    int64_t sleep_timeout_ms = 0;
    std::condition_variable event;
};

struct ChannelItemQueue;

class ChannelImpl
{
  public:
    explicit ChannelImpl(size_t max_buffer_size,
                         size_t object_size,
                         const std::function<void(void *)> &constructor,
                         const std::function<void(void *)> &destructor,
                         const std::function<void(void *, const void *)> &assignment);

    bool input(const void *object);

    bool input_and_close(const void *object);

    ChannelStatus try_input(const void *object);

    bool output(void *object);

    ChannelStatus try_output(void *object);

    void close();

    size_t size() const;

    size_t capacity() const;

  private:
    friend class ChannelSelect;
    std::shared_ptr<ChannelItemQueue> m;
};

template <typename T> class ChannelIter
{
  public:
    typedef T value_type;

  public:
    explicit ChannelIter(Channel<T> &chan) : chan(chan)
    {
    }

    bool operator!=(const ChannelIter &other) const
    {
        return status != other.status;
    }

    T operator*() const
    {
        return value;
    }

    ChannelIter &operator++()
    {
        status = chan >> value;
        return *this;
    }

    const ChannelIter operator++(int)
    {
        auto this_object = *this;
        status = chan >> value;
        return this_object;
    }

  private:
    friend class Channel<T>;

    Channel<T> &chan;
    value_type value;
    bool status = true;
};

template <typename T> Channel<T>::Channel(size_t max_buffer_size)
{
    m = std::make_shared<ChannelImpl>(
        max_buffer_size,
        sizeof(T),
        [](void *address) { new (address) T(); },
        [](void *object) { ((T *)object)->~T(); },
        [](void *dst, const void *src) { *((T *)dst) = *((T *)src); });
}

template <typename T> bool Channel<T>::operator<<(const T &object)
{
    return m->input(&object);
}

template <typename T> bool Channel<T>::input(const T &object)
{
    return m->input(&object);
}

template <typename T> bool Channel<T>::input_and_close(const T &object)
{
    return m->input_and_close(&object);
}

template <typename T> ChannelStatus Channel<T>::try_input(const T &object)
{
    return m->try_input(&object);
}

template <typename T> bool Channel<T>::operator>>(T &object)
{
    return m->output(&object);
}

template <typename T> bool Channel<T>::operator>>(T &&object)
{
    return m->output(&object);
}

template <typename T> bool Channel<T>::output(T &object)
{
    return m->output(&object);
}

template <typename T> bool Channel<T>::output(T &&object)
{
    return m->output(&object);
}

template <typename T> ChannelStatus Channel<T>::try_output(T &object)
{
    return m->try_output(&object);
}

template <typename T> ChannelStatus Channel<T>::try_output(T &&object)
{
    return m->try_output(&object);
}

template <typename T> void Channel<T>::close()
{
    m->close();
}

template <typename T> size_t Channel<T>::size() const
{
    return m->size();
}

template <typename T> size_t Channel<T>::capacity() const
{
    return m->capacity();
}

template <typename T> ChannelIter<T> Channel<T>::begin()
{
    auto iter = ChannelIter<T>(*this);
    iter.status = operator>>(iter.value);
    return iter;
}

template <typename T> ChannelIter<T> Channel<T>::end()
{
    auto iter = ChannelIter<T>(*this);
    iter.status = false;
    return iter;
}

class ChannelCase
{
  public:
    explicit ChannelCase(ChannelImpl &chan) : chan(chan)
    {
    }

    template <typename T> ChannelCase &operator<<(const T &t)
    {
        object = const_cast<T *>(&t);
        is_input = true;
        return *this;
    }

    template <typename T> ChannelCase &operator>>(T &t)
    {
        object = &t;
        is_input = false;
        return *this;
    }

    template <typename T> ChannelCase &operator>>(T &&t)
    {
        object = &t;
        is_input = false;
        return *this;
    }

  private:
    friend class ChannelSelect;

    ChannelImpl chan;
    bool is_input = true;
    void *object = nullptr;

  public:
    std::function<void()> callback;
    std::function<bool(bool)> inner_callback;
};

#define MAX_TIMER_NUM_IN_SELECTOR (16)

class ChannelSelect
{
  public:
    ~ChannelSelect();

    template <typename T> ChannelCase operator<<(const Channel<T> &chan)
    {
        ChannelCase cs(*chan.m);
        return cs;
    }

    template <typename T> ChannelCase operator<<(const IChannel<T> &chan)
    {
        ChannelCase cs(*chan.m.m);
        return cs;
    }

    template <typename T> ChannelCase operator<<(const OChannel<T> &chan)
    {
        ChannelCase cs(*chan.m.m);
        return cs;
    }

  public:
    bool valid = true;
    std::mutex mutex;
    ChannelTimer timers[MAX_TIMER_NUM_IN_SELECTOR];
    size_t timer_num = 0;
    std::vector<ChannelCase> cases;
    std::vector<std::function<void()>> defaults;
};

#define SELECT for (ChannelSelect cs; cs.valid; cs.valid = false)

#define CASE(channel_io_exp, success)                                                                                  \
    sizeof(decltype(channel_io_exp));                                                                                  \
    cs.cases.emplace_back(cs << channel_io_exp);                                                                       \
    cs.cases.back().inner_callback = [&](bool is_success) {                                                            \
        success = is_success;                                                                                          \
        return true;                                                                                                   \
    };                                                                                                                 \
    cs.cases.back().callback = [&]()

#define CASE_IF(channel_io_exp)                                                                                        \
    sizeof(decltype(channel_io_exp));                                                                                  \
    cs.cases.emplace_back(cs << channel_io_exp);                                                                       \
    cs.cases.back().inner_callback = [&](bool is_success) { return is_success; };                                      \
    cs.cases.back().callback = [&]()

#define CASE_ANYWAY(channel_io_exp)                                                                                    \
    sizeof(decltype(channel_io_exp));                                                                                  \
    cs.cases.emplace_back(cs << channel_io_exp);                                                                       \
    cs.cases.back().inner_callback = [&](bool) { return true; };                                                       \
    cs.cases.back().callback = [&]()

#define CASE_CALLBACK(channel_io_exp, callback)                                                                        \
    sizeof(decltype(channel_io_exp));                                                                                  \
    cs.cases.emplace_back(cs << channel_io_exp);                                                                       \
    cs.cases.back().inner_callback = [&](bool is_success) {                                                            \
        callback(is_success);                                                                                          \
        return false;                                                                                                  \
    }

#define CASE_AFTER_MSEC(milliseconds)                                                                                  \
    cs.timers[cs.timer_num++].reset(milliseconds);                                                                     \
    CASE_ANYWAY(cs.timers[cs.timer_num - 1].c >> true)

#define DEFAULT                                                                                                        \
    cs.defaults.emplace_back();                                                                                        \
    cs.defaults.back() = [&]()


#include "channel.h"

#include <algorithm>
#include <map>
#include <random>

using namespace std::chrono;

struct ChannelItemRef
{
    ChannelItemRef(std::thread::id tid,
                   void *object,
                   high_resolution_clock::time_point timestamp = high_resolution_clock::now(),
                   bool test = false)
        : tid(tid), object(object), timestamp(timestamp), test(test)
    {
    }
    bool test = false;
    void *object = nullptr;
    std::thread::id tid;
    high_resolution_clock::time_point timestamp;
};

class ThreadContext
{
  public:
    static ThreadContext &get(std::thread::id tid)
    {
        std::unique_lock<std::mutex> lock(context_mutex);
        return g_thread_contexts[tid];
    }

    void sleep(std::unique_lock<std::mutex> &lock, high_resolution_clock::time_point timestamp)
    {
        io_success = false;
        event.wait(lock, [&]() { return ref_timestamp == timestamp; });
    }

    bool wake_up(high_resolution_clock::time_point timestamp, bool success = false)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (timestamp > ref_timestamp)
        {
            ref_timestamp = timestamp;
            io_success = success;
            event.notify_one();
            return true;
        }
        return false;
    }

    std::mutex mutex;
    std::condition_variable event;
    bool io_success = false;
    high_resolution_clock::time_point ref_timestamp = high_resolution_clock::time_point();

    static std::mutex context_mutex;
    static std::map<std::thread::id, ThreadContext> g_thread_contexts;
};

std::mutex ThreadContext::context_mutex;
std::map<std::thread::id, ThreadContext> ThreadContext::g_thread_contexts;

struct ChannelItemQueue
{
    bool closed = false;
    size_t rear = 0;
    size_t front = 0;
    size_t count = 0;
    size_t capacity = 0;
    size_t item_size = 0;
    std::vector<char> buffer;

    std::mutex mutex;
    std::deque<ChannelItemRef> read_que;
    std::deque<ChannelItemRef> write_que;

    std::function<void(void *)> constructor;
    std::function<void(void *)> destructor;
    std::function<void(void *, const void *)> assignment;

    ChannelStatus try_write(const void *object)
    {
        if (closed)
        {
            return CHANNEL_CLOSED;
        }
        while (!read_que.empty())
        {
            ChannelItemRef t_data = read_que.front();
            read_que.pop_front();
            if (t_data.test)
            {
                if (ThreadContext::get(t_data.tid).wake_up(t_data.timestamp))
                {
                    return CHANNEL_NOT_READY;
                }
                else
                {
                    continue;
                }
            }
            assignment(t_data.object, object);
            ThreadContext::get(t_data.tid).wake_up(t_data.timestamp, true);
            return CHANNEL_OK;
        }
        if (count < capacity)
        {
            assignment(&buffer[rear], object);
            rear = (rear + item_size) % buffer.size();
            ++count;
            return CHANNEL_OK;
        }
        return CHANNEL_NOT_READY;
    }

    ChannelStatus try_read(void *object)
    {
        while (!write_que.empty())
        {
            ChannelItemRef t_data = write_que.front();
            write_que.pop_front();
            if (t_data.test)
            {
                if (ThreadContext::get(t_data.tid).wake_up(t_data.timestamp))
                {
                    return CHANNEL_NOT_READY;
                }
                else
                {
                    continue;
                }
            }
            else if (count > 0)
            {
                assignment(object, &buffer[front]);
                destructor(&buffer[front]);
                constructor(&buffer[front]);
                front = (front + item_size) % buffer.size();
                assignment(&buffer[rear], t_data.object);
                rear = (rear + item_size) % buffer.size();
            }
            else
            {
                assignment(object, t_data.object);
            }
            ThreadContext::get(t_data.tid).wake_up(t_data.timestamp, true);
            return CHANNEL_OK;
        }
        if (count > 0)
        {
            assignment(object, &buffer[front]);
            destructor(&buffer[front]);
            constructor(&buffer[front]);
            front = (front + item_size) % buffer.size();
            --count;
            return CHANNEL_OK;
        }
        if (closed)
        {
            return CHANNEL_CLOSED;
        }
        return CHANNEL_NOT_READY;
    }
};

ChannelImpl::ChannelImpl(size_t max_buffer_size,
                         size_t object_size,
                         const std::function<void(void *)> &constructor,
                         const std::function<void(void *)> &destructor,
                         const std::function<void(void *, const void *)> &assignment)
{
    m.reset(new ChannelItemQueue(), [](ChannelItemQueue *chan) {
        std::unique_lock<std::mutex> lock(chan->mutex);
        chan->closed = true;
        for (const auto &t_data : chan->write_que)
        {
            ThreadContext::get(t_data.tid).wake_up(t_data.timestamp);
        }
        for (const auto &t_data : chan->read_que)
        {
            ThreadContext::get(t_data.tid).wake_up(t_data.timestamp);
        }
        for (size_t i = 0; i < chan->capacity; ++i)
        {
            chan->destructor(chan->buffer.data() + i * chan->item_size);
        }
        lock.unlock();
        delete chan;
    });
    m->constructor = constructor;
    m->destructor = destructor;
    m->assignment = assignment;
    m->item_size = object_size;
    m->capacity = max_buffer_size;
    m->buffer.resize(m->capacity * m->item_size);
    for (size_t i = 0; i < m->capacity; ++i)
    {
        m->constructor(m->buffer.data() + i * m->item_size);
    }
}

bool ChannelImpl::input(const void *object)
{
    std::unique_lock<std::mutex> lock(m->mutex);
    auto status = m->try_write(object);
    if (status != CHANNEL_NOT_READY)
    {
        return status == CHANNEL_OK;
    }
    // channel is not ready, so put it in write_que then.
    auto thread_id = std::this_thread::get_id();
    m->write_que.push_back(ChannelItemRef(thread_id, const_cast<void *>(object)));
    ThreadContext::get(thread_id).sleep(lock, m->write_que.back().timestamp);
    return ThreadContext::get(thread_id).io_success;
}

bool ChannelImpl::input_and_close(const void *object)
{
    std::unique_lock<std::mutex> lock(m->mutex);
    std::shared_ptr<bool> defer(nullptr, [this](bool *) {
        m->closed = true;
        for (const auto &t_data : m->write_que)
        {
            ThreadContext::get(t_data.tid).wake_up(t_data.timestamp);
        }
        for (const auto &t_data : m->read_que)
        {
            ThreadContext::get(t_data.tid).wake_up(t_data.timestamp);
        }
    });
    auto status = m->try_write(object);
    if (status != CHANNEL_NOT_READY)
    {
        return status == CHANNEL_OK;
    }
    // channel is not ready, so put it in write_que then.
    auto thread_id = std::this_thread::get_id();
    m->write_que.push_back(ChannelItemRef(thread_id, const_cast<void *>(object)));
    ThreadContext::get(thread_id).sleep(lock, m->write_que.back().timestamp);
    return ThreadContext::get(thread_id).io_success;
}

ChannelStatus ChannelImpl::try_input(const void *object)
{
    std::unique_lock<std::mutex> lock(m->mutex);
    return m->try_write(object);
}

bool ChannelImpl::output(void *object)
{
    std::unique_lock<std::mutex> lock(m->mutex);
    auto status = m->try_read(object);
    if (status != CHANNEL_NOT_READY)
    {
        return status == CHANNEL_OK;
    }
    // channel is not ready, so put it in read_que then.
    auto thread_id = std::this_thread::get_id();
    m->read_que.push_back(ChannelItemRef(thread_id, object));
    ThreadContext::get(thread_id).sleep(lock, m->read_que.back().timestamp);
    return ThreadContext::get(thread_id).io_success;
}

ChannelStatus ChannelImpl::try_output(void *object)
{
    std::unique_lock<std::mutex> lock(m->mutex);
    return m->try_read(object);
}

void ChannelImpl::close()
{
    std::unique_lock<std::mutex> lock(m->mutex);
    m->closed = true;
    for (const auto &t_data : m->write_que)
    {
        ThreadContext::get(t_data.tid).wake_up(t_data.timestamp);
    }
    for (const auto &t_data : m->read_que)
    {
        ThreadContext::get(t_data.tid).wake_up(t_data.timestamp);
    }
}

size_t ChannelImpl::size() const
{
    return m->count;
}

size_t ChannelImpl::capacity() const
{
    return m->capacity;
}

ChannelSelect::~ChannelSelect()
{
    if (valid)
    {
        return;
    }

    std::sort(cases.begin(), cases.end(), [](const ChannelCase &a, const ChannelCase &b) {
        return a.chan.m.get() < b.chan.m.get();
    });
    auto lock_cases = [&](bool is_lock) {
        void *prev = nullptr;
        for (auto &cs : cases)
        {
            if (cs.chan.m.get() != prev)
            {
                is_lock ? cs.chan.m->mutex.lock() : cs.chan.m->mutex.unlock();
            }
            prev = cs.chan.m.get();
        }
    };
    auto lock_all = [&]() { lock_cases(true); };
    auto unlock_all = [&]() { lock_cases(false); };

    bool over = false;
    while (!over)
    {
        auto random_cases = cases;
        std::shuffle(random_cases.begin(), random_cases.end(), std::mt19937(std::random_device()()));

        lock_all();
        for (auto &cs : random_cases)
        {
            int status;
            if (cs.is_input)
            {
                status = cs.chan.m->try_write(cs.object);
            }
            else
            {
                status = cs.chan.m->try_read(cs.object);
            }
            if (status != CHANNEL_NOT_READY)
            {
                if (cs.inner_callback(status == CHANNEL_OK) && cs.callback)
                {
                    cs.callback();
                }
                over = true;
                break;
            }
        }

        if (over)
        {
            unlock_all();
            break;
        }

        if (!defaults.empty())
        {
            unlock_all();
            auto d_time = high_resolution_clock::now() - high_resolution_clock::time_point();
            std::default_random_engine e(duration_cast<milliseconds>(d_time).count());
            defaults[e() % defaults.size()]();
            break;
        }

        auto tid = std::this_thread::get_id();
        auto timestamp = std::chrono::high_resolution_clock::now();
        for (auto &cs : random_cases)
        {
            auto &&t_data = ChannelItemRef(tid, cs.object, timestamp, true);
            if (cs.is_input)
            {
                cs.chan.m->write_que.push_back(t_data);
            }
            else
            {
                cs.chan.m->read_que.push_back(t_data);
            }
        }

        std::unique_lock<std::mutex> lck(mutex);
        unlock_all();
        ThreadContext::get(std::this_thread::get_id()).sleep(lck, timestamp);
        lock_all();

        for (auto &cs : random_cases)
        {
            int status;
            if (cs.is_input)
            {
                status = cs.chan.m->try_write(cs.object);
            }
            else
            {
                status = cs.chan.m->try_read(cs.object);
            }
            if (status != CHANNEL_NOT_READY)
            {
                if (cs.inner_callback(status == CHANNEL_OK) && cs.callback)
                {
                    cs.callback();
                }
                over = true;
                break;
            }
        }

        unlock_all();
    }
}

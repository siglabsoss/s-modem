#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

/// A threadsafe-queue.
/// c-modem has as similar file:
/// https://github.com/siglabs/c-modem/blob/master/cc/src/modules/threadqueue/threadqueue.cc
template <class T>
class SafeQueue
{
public:
    SafeQueue(void)
        : q()
        , m()
        , c()
    {}

    ~SafeQueue(void)
    {}

    /// Add an element to the queue.
    /// Add elemnt to the Safe Queue
    /// 
    void enqueue(const T t)
    {
        ///.
        /// std lock_guard can only be locked/unlocked on destruction
        /// compared to a (not used) unique_lock which can be locked/unlocked 
        /// https://stackoverflow.com/questions/20516773/stdunique-lockstdmutex-or-stdlock-guardstdmutex
        /// https://stackoverflow.com/questions/17113619/whats-the-best-way-to-lock-multiple-stdmutexes/17113678#17113678
        /// https://stackoverflow.com/questions/13667810/massive-cpu-load-using-stdlock-c11
        
        /// 
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        c.notify_one();
    }

    // grab the lock from outside
    void unsafeEnqueue(const T t) {
        q.push(t);
        c.notify_one();
    }

    void enqueue(const std::vector<T>& vec)
    {
        std::lock_guard<std::mutex> lock(m);
        for(auto e : vec) {
            q.push(e);
        }
        c.notify_one();
    }

    // Get the "front"-element.
    // If the queue is empty, wait till a element is avaiable.
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        while(q.empty())
        {
          // release lock as long as the wait and reaquire it afterwards.
          c.wait(lock);
        }
        T val = q.front();
        q.pop();
        return val;
    }

    T& peek(void)
    {
        std::unique_lock<std::mutex> lock(m);
        return q.front();
    }

    size_t size(void) const
    {
        std::unique_lock<std::mutex> lock(m);
        return q.size();
    }

    size_t dump(void)
    {
        std::unique_lock<std::mutex> lock(m);
        size_t dumped = 0;
        while(!q.empty()) {
            q.pop();
            dumped++;
        }
        return dumped;
    }

    /// We need to replace this SafeQueue with a SafeVector
    /// This finds the total sum of every element in the q
    /// will only work if the T type has a T.size() method
    size_t expensiveSizeAll(void) const {
        std::queue<T> copy;
        {
            std::unique_lock<std::mutex> lock(m);
            copy = q;
        }
        size_t tally = 0;
        while(!copy.empty()) {
            const auto &x = q.front();
            tally += x.size();
            copy.pop();
        }
        return tally;
    }

    // Get the first "N" elements and resize
    std::vector<T> get(const size_t num)
    {
        std::vector<T> out(num);

        size_t i = 0;
        while(true)
        {
            // sleep on subsequent loops
            if( i != 0 ) {
                usleep(1);
            }

            size_t j = 0;

            std::unique_lock<std::mutex> lock(m);
            for(; i < num && (!q.empty()); i++) {
                out[i] = q.front();
                q.pop();

                if( j >= 1000 ) {
                    // every 10 k samples we read out, we release the lock
                    // and sleep for 1 us, allowing others to run
                    // causes lock to be released and re-acquired
                    continue;
                }
                j++;
            }
            break;
        }


        return out;
    }

private:
    std::queue<T> q;
public:
    mutable std::mutex m;
private:
    std::condition_variable c;
};



// template <class T>
// class SafeQueueTracked: public SafeQueue<T> {
// public:
//     SafeQueueTracked(void)
//         {}

// };


template <class T, class Y>
class SafeQueueTracked
{
public:
    SafeQueueTracked(void)
        : q()
        , q2()
        , m()
        , c()
    {}

    ~SafeQueueTracked(void)
    {}

    // Add an element to the queue.
    void enqueue(const T t, const Y y)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        q2.push(y);
        c.notify_one();
    }

    // Get the "front"-element.
    // If the queue is empty, wait till a element is avaiable.
    std::pair<T,Y> dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        while(q.empty())
        {
          // release lock as long as the wait and reaquire it afterwards.
          c.wait(lock);
        }
        T val = q.front();
        q.pop();

        Y yval = q2.front();
        q2.pop();
        return std::pair<T,Y>(val,yval);
    }

    size_t size(void) const
    {
        std::unique_lock<std::mutex> lock(m);
        return q.size();
    }

    // Get the first "N" elements and resize
    std::vector<std::pair<T,Y>> get(const size_t num)
    {
        std::vector<std::pair<T,Y>> out(num);
        {
            std::unique_lock<std::mutex> lock(m);
            for(size_t i = 0; i < num && (!q.empty()); i++) {
                out[i] = std::pair<T,Y>(q.front(), q2.front());
                q.pop();
                q2.pop();
            }
        }
        return out;
    }

    std::pair<T,Y> peek(void)
    {
        std::unique_lock<std::mutex> lock(m);
        return std::pair<T,Y>(q.front(), q2.front());
    }

    std::pair<T,Y> peekBack(void)
    {
        std::unique_lock<std::mutex> lock(m);
        return std::pair<T,Y>(q.back(), q2.back());
    }

private:
    std::queue<T> q;
    std::queue<Y> q2;
    mutable std::mutex m;
    std::condition_variable c;
};
#ifndef STRUCTS_EVENTQUEUE_H
#define STRUCTS_EVENTQUEUE_H

#include "./events.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>

namespace Events
{
    /**
	 * Queue definition for thread-safe writing and reading.
	 *
	 * Reading will be blocked if empty until there are events to read.
	 */
    struct Queue
    {
    private:
        std::mutex mutex_;
        std::condition_variable hasEvent_;
        std::condition_variable hasSpace_;
        std::queue<Event> queue_;

    public:
        /** Places an event in the queue (emplacement style) */
        template <typename T, typename... Args>
        inline void enqueue(Args &&...args)
        {
            std::unique_lock<std::mutex> lock(this->mutex_);

            // If queue is full, release the lock and go to sleep and wait to get notified
            this->hasSpace_.wait(
                lock,
                [this]
                { return this->queue_.size() < 1000; });

            // Emplace the event into the queue now that there is space
            this->queue_.emplace(T(std::forward<Args>(args)...));

            // Notify the condition var that there is at least an event now
            this->hasEvent_.notify_one();
            // Lock guard goes out of scope and releases
        }

        /** Reads and removes the first event in the queue */
        inline void dequeue(Event &event)
        {
            std::unique_lock<std::mutex> lock(this->mutex_);

            // If queue is empty, release the lock and go to sleep and wait to get notified
            this->hasEvent_.wait(
                lock,
                [this]
                { return this->queue_.size(); });

            // Move into the first event into the input reference
            event = std::move(this->queue_.front());

            // Pop the existing reference
            this->queue_.pop();

            // Notify the condition var that there is space now
            this->hasSpace_.notify_one();
            // Lock guard goes out of scope and releases
        }
    };
}

#endif

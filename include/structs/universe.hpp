#ifndef STRUCTS_UNIVERSE_H
#define STRUCTS_UNIVERSE_H

#include "build_config.h"

#include <mutex>
#include <thread>
#include <unordered_set>

namespace Universe
{
    struct Universe
    {
    private:
        std::mutex mutex_;
        std::unordered_set<securityId_t> universe_;

    public:
        /* Make the struct iterable on the internal set data structure */
        typedef std::unordered_set<securityId_t>::const_iterator const_iterator;
        const_iterator begin() const { return this->universe_.begin(); }
        const_iterator end() const { return this->universe_.end(); }

        inline size_t size() const
        {
            return this->universe_.size();
        }

        inline void update(Universe const &universe)
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->universe_ = universe.universe_;
            // Lock guard goes out of scope and releases
        }

        template <typename... Args>
        inline void emplace(Args &&...args)
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->universe_.emplace(std::forward<Args>(args)...);
            // Lock guard goes out of scope and releases
        }
    };
}

#endif

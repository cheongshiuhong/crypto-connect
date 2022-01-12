#ifndef STRUCTS_UNIVERSE_H
#define STRUCTS_UNIVERSE_H

#include <mutex>
#include <thread>
#include <unordered_set>

namespace Universe
{
    struct Universe
    {
    private:
        std::mutex mutex_;
        std::unordered_set<std::string> universe_;

    public:
        /* Make the struct iterable on the internal set data structure */
        typedef std::unordered_set<std::string>::const_iterator const_iterator;
        const_iterator begin() const { return this->universe_.begin(); }
        const_iterator end() const { return this->universe_.end(); }

        /* Default Constructor */
        Universe() {}

        /* Construct from an input set */
        Universe(std::unordered_set<std::string> universeSet)
        {
            for (auto const &item : universeSet)
                this->emplace(item);
        }

        /* Contruct from another universe object */
        Universe(Universe const &universe)
        {
            this->update(universe);
        }

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

        inline void update(Universe const &&universe)
        {
            this->update(universe);
        }

        inline void merge(Universe const &universe)
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            for (auto const &item : universe)
                this->universe_.emplace(item);
            // Lock guard goes out of scope and releases
        }

        inline void intersection(Universe const &universe)
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            std::unordered_set<std::string> copy(this->universe_);
            for (auto const &item : copy)
            {
                if (universe.universe_.find(item) == universe.universe_.end())
                    this->universe_.erase(item);
            }
            // Lock guard goes out of scope and releases
        }

        template <typename... Args>
        inline void emplace(Args &&...args)
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->universe_.emplace(std::forward<Args>(args)...);
            // Lock guard goes out of scope and releases
        }

        inline void erase(std::string productId)
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->universe_.erase(productId);
            // Lock guard goes out of scope and releases
        }

        inline void clear()
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            this->universe_.clear();
            // Lock guard goes out of scope and releases
        }
    };
}

#endif

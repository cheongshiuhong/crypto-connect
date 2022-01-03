#ifndef STRUCTS_EVENTS_H
#define STRUCTS_EVENTS_H

#include "build_config.h"

#include "./orders.hpp"

#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <variant>
#include <vector>
#include <thread>

/**
 * Event epoch times represented in nanoseconds
 */
namespace Events
{
	/* Bar event representing the aggregation of trade events in an interval */
	struct Bar
	{
		uint64_t epochTime_;
		securityId_t securityId_;
		double open_, high_, low_, close_, vol_;

		/* Default Constructor for empty event */
		Bar() : epochTime_(0), securityId_(""),
				open_(0.0), high_(0.0), low_(0.0),
				close_(0.0), vol_(0){};

		/* Constructor */
		Bar(uint64_t epochTime, securityId_t securityId, double open,
			double high, double low, double close, double vol)
			: epochTime_(epochTime), securityId_(securityId),
			  open_(open), high_(high), low_(low),
			  close_(close), vol_(vol){};
	};

	inline std::ostream &operator<<(std::ostream &os, Bar const &bar)
	{
		os << "Time since epoch: " << bar.epochTime_ << " | "
		   << "Security ID: " << bar.securityId_ << " | "
		   << "Open: " << bar.open_ << " | "
		   << "High: " << bar.high_ << " | "
		   << "Low: " << bar.low_ << " | "
		   << "Close: " << bar.close_ << " | "
		   << "Volume: " << bar.vol_;
		return os;
	}

	/* Tick event representing the update of best bid/ask */
	struct Tick
	{
		uint64_t epochTime_;
		securityId_t securityId_;
		double bid_, ask_, volBid_, volAsk_;
		bool isBuySide_;

		/* Default Constructor for empty event */
		Tick() : epochTime_(0), securityId_(""),
				 bid_(0.0), ask_(0.0), volBid_(0), volAsk_(0),
				 isBuySide_(false){};

		/* Constructor */
		Tick(uint64_t epochTime, securityId_t securityId,
			 double bid, double ask, double volBid, double volAsk,
			 bool isBuySide)
			: epochTime_(epochTime), securityId_(securityId),
			  bid_(bid), ask_(ask), volBid_(volBid), volAsk_(volAsk),
			  isBuySide_(isBuySide){};
	};

	inline std::ostream &operator<<(std::ostream &os, Tick const &tick)
	{
		os << "Time since epoch: " << tick.epochTime_ << " | "
		   << "Security ID: " << tick.securityId_ << " | "
		   << "Bid Price: " << tick.bid_ << " | "
		   << "Ask Price: " << tick.ask_ << " | "
		   << "Bid Volume: " << tick.volBid_ << " | "
		   << "Ask Volume: " << tick.volAsk_ << " | "
		   << "isBuySide: " << tick.isBuySide_;
		return os;
	}

	/* Trade event representing a trade happening in the exchange */
	struct Trade
	{
		uint64_t epochTime_;
		securityId_t securityId_;
		double lastPrice_, lastSize_;
		bool isBuySide_;

		/* Default Constructor for empty event */
		Trade() : epochTime_(0), securityId_(""),
				  lastPrice_(0.0), lastSize_(0), isBuySide_(false){};

		/* Constructor */
		Trade(uint64_t epochTime, securityId_t securityId,
			  double lastPrice, double lastSize, bool isBuySide)
			: epochTime_(epochTime), securityId_(securityId),
			  lastPrice_(lastPrice), lastSize_(lastSize),
			  isBuySide_(isBuySide){};
	};

	inline std::ostream &operator<<(std::ostream &os, Trade const &trade)
	{
		os << "Time since epoch: " << trade.epochTime_ << " | "
		   << "Security ID: " << trade.securityId_ << " | "
		   << "Last Price: " << trade.lastPrice_ << " | "
		   << "Last Size: " << trade.lastSize_ << " | "
		   << "isBuySide: " << trade.isBuySide_;
		return os;
	}

	/* OrderStatus event representing an update of the user's order */
	struct OrderStatus
	{
		Orders::orderId_t id_;
		uint64_t epochTime_;
		securityId_t securityId_;
		Orders::Status status_;
		double quantityLeft_;

		/* Constructor */
		OrderStatus(Orders::orderId_t id, uint64_t epochTime, securityId_t securityId,
					Orders::Status status, double quantityLeft)
			: id_(id), epochTime_(epochTime), securityId_(securityId),
			  status_(status), quantityLeft_(quantityLeft){};
	};

	inline std::ostream &operator<<(std::ostream &os, OrderStatus const &orderStatus)
	{
		os << "Time since epoch: " << orderStatus.epochTime_ << " | "
		   << "Security ID: " << orderStatus.securityId_ << " | "
		   << "Order ID: " << orderStatus.id_ << " | "
		   << "Type: " << (orderStatus.status_ == Orders::Status::OPEN ? "open" : "done") << " | "
		   << "Quantity Left: " << orderStatus.quantityLeft_;

		return os;
	}

	/* Transaction event representing a trade occuring for the user */
	struct Transaction
	{
		Orders::orderId_t id_;
		uint64_t epochTime_;
		securityId_t securityId_;
		double price_;
		double quantity_;

		/* Constructor */
		Transaction(Orders::orderId_t id, uint64_t epochTime, securityId_t securityId,
					double price, double quantity)
			: id_(id), epochTime_(epochTime), securityId_(securityId),
			  price_(price), quantity_(quantity){};
	};

	inline std::ostream &operator<<(std::ostream &os, Transaction const &transaction)
	{
		os << "Time since epoch: " << transaction.epochTime_ << " | "
		   << "Security ID: " << transaction.securityId_ << " | "
		   << "Order ID: " << transaction.id_ << " | "
		   << "Price: " << (transaction.price_) << " | "
		   << "Quantity: " << transaction.quantity_;

		return os;
	}

	/* Variant index matches with Types */
	using Event = std::variant<Bar, Tick, Trade, OrderStatus, Transaction>;

	/* Overloaded utility to visit an event variant*/
	template <class... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};

	template <class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

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
				{ return this->queue_.size() < MAX_EVENT_QUEUE_SIZE; });

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

			// Copy into the input reference
			event = this->queue_.front();

			// Pop the existing reference
			this->queue_.pop();

			// Notify the condition var that there is space now
			this->hasSpace_.notify_one();

			// Lock guard goes out of scope and releases
		}
	};

	/* Collections */
	using bars_t = std::vector<Bar>;
	using ticks_t = std::vector<Tick>;
	using trades_t = std::vector<Trade>;
}

#endif

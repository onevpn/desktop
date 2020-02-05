#pragma once

#include <map>
#include <deque>

#include "Message.h"

namespace PubSub {

namespace Details {
struct SubscriberBase;
}

/// Singletone dispatching object
/// Implements routing messages to subscribers
class Dispatcher
{
public:
	static Dispatcher & instance();

	/// Register subscriber
	void enlist(Details::SubscriberBase * sub);
	/// Unregister subscriber. No more messages will be delivered to `sub`
	void delist(Details::SubscriberBase * sub);

	/// Send message to all subscribers interested in such type of message
	void dispatch(Details::Message const &);

private:
	Dispatcher () = default;
	~Dispatcher() = default;
	Dispatcher (Dispatcher const & ) = delete;
	Dispatcher (Dispatcher       &&) = delete;
	Dispatcher & operator=(Dispatcher const &) = delete;
	Dispatcher & operator=(Dispatcher      &&) = delete;

	std::map<std::string, std::deque<Details::SubscriberBase*>> m_messageRouting;
};

}

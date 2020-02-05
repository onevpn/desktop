#include "Dispatcher.h"
#include "Subscriber.h"
#include "Agent.h"

#include <algorithm>
#include <cassert>

namespace PubSub {

namespace Details {

struct Deliverer
{
	static void deliver(Details::SubscriberBase * subscriber, Details::Message const & message)
	{
		subscriber->dispatch(message);
	}
	static void grabSubscriberTypes(Details::SubscriberBase *subscriber, std::vector<std::string> & types)
	{
		subscriber->grabMessageTypes(types);
	}
};

/// Helper class to access protected data in Message
struct MessageRouter
{
	static PubSub::Object const * routingObject(PubSub::Details::Message const & m)
	{
		return m.routingObject();
	}

	static std::string const & objectType(PubSub::Details::Message const & m)
	{
		return m.objectType();
	}
};

} // namespace Details

using Details::SubscriberBase;
using Details::Deliverer;
using Details::MessageRouter;

Dispatcher & Dispatcher::instance()
{
	static Dispatcher instance;
	return instance;
}

void Dispatcher::enlist(SubscriberBase * subscriber)
{
	std::vector<std::string> messageTypes;
	Deliverer::grabSubscriberTypes(subscriber, messageTypes);
	for (auto & type : messageTypes)
		m_messageRouting[type].push_back(subscriber);
}

void Dispatcher::delist(SubscriberBase * subscriber)
{
	std::vector<std::string> messageTypes;
	Deliverer::grabSubscriberTypes(subscriber, messageTypes);
	for (auto & type : messageTypes)
	{
		auto & subs = m_messageRouting[type];
		subs.erase(std::remove(subs.begin(), subs.end(), subscriber), subs.end());
		if (subs.empty())
			m_messageRouting.erase(type);
	}
}

void Dispatcher::dispatch(Details::Message const & message)
{
	auto subsIt = m_messageRouting.find(typeid(message).name());
	if (subsIt == m_messageRouting.end())
		return; // No subscribers for this message. May be assert?
//	assert(m_messageRouting.find(type) != m_messageRouting.end());

	auto object = MessageRouter::routingObject(message);
	auto type   = MessageRouter::objectType   (message);

	// Do not deliver message to new subscribers
	std::set<SubscriberBase *> subs;
	for (auto & sub : subsIt->second)
	{
		if (sub->isSubscribed(object) || sub->isSubscribed(type))
			subs.insert(sub);
	}
	for (auto & sub : subs)
		Deliverer::deliver(sub, message);
}

}

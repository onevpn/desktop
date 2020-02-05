#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cassert>
#include <typeinfo>

#include "Message.h"
#include "Dispatcher.h"
#include "Agent.h"

namespace PubSub {

namespace Details {

struct Deliverer;

struct SubscriberBase : virtual Agent
{
	using Message = Details::Message;
	friend struct Deliverer;

	virtual ~SubscriberBase()
	{
		Dispatcher::instance().delist(this);
	}

	// By object
	void subscribe   (Object const * o);
	void unsubscribe (Object const * o);
	bool isSubscribed(Object const * o);

	// By object type
	template<typename T> void subscribeAll  () { return subscribeAll  (typeid(T).name()); }
	template<typename T> void unsubscribeAll() { return unsubscribeAll(typeid(T).name()); }
	template<typename T> bool isSubscribed  () { return isSubscribed  (typeid(T).name()); }
	void subscribeAll  (std::string const & t);
	void unsubscribeAll(std::string const & t);
	bool isSubscribed  (std::string const & t);

	using Notify = std::function<void (Message const &)>;

protected:
	void registerMe(std::string const & type, Notify const & callback);

protected:
	void dispatch(Message const & m);
	void grabMessageTypes(std::vector<std::string> &) const;

private:
	std::map<std::string, Notify> m_messageMap;
	std::set<Object const *>      m_objects;
	std::set<std::string   >      m_types;
};

template <typename M> struct Subscriber : virtual SubscriberBase
{
	Subscriber()
	{
		this->registerMe(typeid(M).name(), [this](Message const & message)
			{ this->notify(static_cast<M const &>(message)); }
		);
	}

protected:
	virtual void notify(M const & m) = 0;
};

} // namespace Details


template <typename... Messages> struct Subscriber : virtual Details::Subscriber<Messages> ...
{
	Subscriber()
	{
		Dispatcher::instance().enlist(this);
	}
};

}

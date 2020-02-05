#include "Subscriber.h"

#include <algorithm>
#include <iterator>

namespace PubSub { namespace Details {

void SubscriberBase::subscribe   (Object const * o) { assert(m_objects.find(o) == m_objects.end() && !!o); m_objects.insert(o); }
void SubscriberBase::unsubscribe (Object const * o) { assert(m_objects.find(o) != m_objects.end() && !!o); m_objects.erase (o); }
bool SubscriberBase::isSubscribed(Object const * o) { return m_objects.find(o) != m_objects.end(); }

void SubscriberBase::subscribeAll  (std::string const & t) { assert(m_types.find(t) == m_types.end()); m_types.insert(t); }
void SubscriberBase::unsubscribeAll(std::string const & t) { assert(m_types.find(t) != m_types.end()); m_types.erase (t); }
bool SubscriberBase::isSubscribed  (std::string const & t) { return m_types.find(t) != m_types.end(); }

void SubscriberBase::registerMe(std::string const & type, Notify const & callback)
{
	m_messageMap[type] = callback;
}

void SubscriberBase::dispatch(Message const & m)
{
	m_messageMap[typeid(m).name()](m);
}

void SubscriberBase::grabMessageTypes(std::vector<std::string> & types) const
{
	types.clear();
	types.reserve(m_messageMap.size());
	std::transform(m_messageMap.begin(), m_messageMap.end(), std::back_inserter(types),
				   [](std::map<std::string, Notify>::value_type const& pair){ return pair.first; });
}

} }

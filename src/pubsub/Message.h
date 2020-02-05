#pragma once

#include "Logging.h"

#include <string>
#include <cassert>
#include <typeinfo>

namespace PubSub {

/// Base object for messaging
struct Object { };


namespace Details {

/// Base message. Should NOT be used in client API
struct Message
{
	friend struct MessageRouter;
	virtual ~Message() = default;

protected:
	virtual Object      const * routingObject() const = 0;
	virtual std::string const & objectType   () const = 0;
};

} // namespace Details

/// Base message type. `O` must be derived from `Object`
template <typename O> struct Message : Details::Message
{
	O const * object;
	Message(O const * object)
		: object(object)
		, m_type(typeid(O).name())
	{
	}

// TODO: Logging temporary disabled
//	int logLevel() const { return Logging::logLevel<O>(); }

protected:
	Object      const * routingObject() const override { return object; }
	std::string const & objectType   () const override { return m_type; }

private:
	std::string m_type;
};

}

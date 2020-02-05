#pragma once

#include <cassert>

#include "pubsub/Message.h"

namespace Message { namespace Types {

template<class O> struct Message : PubSub::Message<O>
{
	using PubSub::Message<O>::Message;
	typedef O Object;
};

template<class O> struct None : Message<O>
{
	None() : Message<O>(0) { assert(0); }
};

template<class O> struct Notification : Message<O>
{
	Notification(O const *o) : Message<O>(o) { }
};

template<class O> struct Request : Message<O>
{
	Request<O>(O const *o) : Message<O>(o) { }
};

} // namespace Types

using Message::Types::Request;
using Message::Types::Notification;

} // namespace Message

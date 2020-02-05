#pragma once

#include "Agent.h"
#include "Dispatcher.h"

namespace PubSub {

namespace Details {

template <typename M> struct SinglePublisher : virtual Agent
{
protected:
	void publish(M const &m)
	{
		Dispatcher::instance().dispatch(m);
	}
};

} // namespace Details

template <typename... Messages> struct Publisher;

template <typename M> struct Publisher<M> : Details::SinglePublisher<M>
{
	using Details::SinglePublisher<M>::publish;
};

template <typename M, typename... Messages> struct Publisher<M, Messages...>
	: Details::SinglePublisher<M>
	, Publisher<Messages...>
{
	using Details::SinglePublisher<M>::publish;
	using Publisher<Messages...>::publish;
};

}

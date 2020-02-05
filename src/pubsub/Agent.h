#pragma once

#include <string>
#include <cassert>
#include <utility>

namespace PubSub {

class Agent
{
protected:
	Agent  (std::string      && n) : m_isMaster(true)  , m_name(std::move(n)) { }
	Agent  (std::string const & n) : m_isMaster(true)  , m_name(n) { }
	Agent  (Agent       const & a) : m_isMaster(false) , m_name(a.name()) { }
	Agent  () { assert(0); } // must specify name if master, or master if slave
	virtual ~Agent () { }

	const std::string & name() const { return m_name; }

private:
	bool m_isMaster;
	std::string m_name;
};

}

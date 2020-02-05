#pragma once

#include <QObject>
#include <QEvent>
#include <QCoreApplication>

namespace Util {

template<class T> class Scheduler : QObject
{
	T *m_target;
    void (T::*m_action)();

	int m_timeout;
    int m_timerId = 0;

    bool m_isEnabled = true;

public:
    Scheduler( T *t , void (T::*a)() , int o = 0)
        : m_target(t)
        , m_action(a)
        , m_timeout(o)
    { }

	void disable()
    {
        m_isEnabled = false;
    }

    void schedule()
    {
        qApp->postEvent( this , new QEvent(QEvent::User) );
    }

	using QObject::moveToThread;

protected:
	void customEvent(QEvent *) override
    {
        if (!m_timerId && m_isEnabled)
            m_timerId = startTimer(m_timeout);
    }

	void timerEvent(QTimerEvent *) override
    {
        killTimer(m_timerId);
        m_timerId = 0;
        (m_target->*m_action)();
    }
};

}

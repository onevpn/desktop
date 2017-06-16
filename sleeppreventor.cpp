#include "sleeppreventor.h"

#ifdef Q_OS_MAC
#include <CoreServices/CoreServices.h>
#endif

namespace {
void internalPrevent()
{
#ifdef Q_OS_MAC
    UpdateSystemActivity(OverallAct);
#endif
}

}

void SleepPreventor::preventSleep(bool prevent)
{
    killTimer(m_timerId);
    m_timerId = 0;
    if(prevent)
        m_timerId = startTimer(2000);
}

void SleepPreventor::timerEvent(QTimerEvent* event)
{
    internalPrevent();
    QObject::timerEvent(event);
}

#pragma once

#include <QObject>

class SleepPreventor : public QObject
{
    Q_OBJECT
public:
    static SleepPreventor &instance()
    {
        static SleepPreventor pr;
        return pr;
    }

    void preventSleep(bool);
protected:
    void timerEvent(QTimerEvent*) Q_DECL_OVERRIDE;

    int m_timerId = 0;
};

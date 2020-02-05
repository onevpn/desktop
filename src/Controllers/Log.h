#pragma once

#include <memory>
#include <QObject>
#include <QPointer>

#include "pubsub/Subscriber.h"

#include "DAOs/Log.h"

class LogBrowserDialog;

namespace Log {

class Controller
	: public QObject
    , PubSub::Subscriber
        <LogNotification
        , EnableWidgetRequest
        , DisableWidgetRequest>
{
	Q_OBJECT

public:
	Controller();
	~Controller();

private:
    void notify(LogNotification const&) override;
    void notify(EnableWidgetRequest const&) override;
    void notify(DisableWidgetRequest const&) override;

private:
    QPointer<LogBrowserDialog> m_dialog;
    const QString m_currSessionId;

    struct ConnectionLogger;
    std::unique_ptr<ConnectionLogger> m_connectionLogger;
};

}

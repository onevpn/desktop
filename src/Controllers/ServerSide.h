#pragma once

#include <functional>
#include <memory>

#include <QTemporaryFile>
#include <QObject>
#include <QMap>

#include "pubsub/Subscriber.h"
#include "pubsub/Publisher.h"

#include "DAOs/ServerSide.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace ServerSide {

class Controller
	: public QObject
    , PubSub::Publisher<IpNotification, LoginOkNotification, LoginFailedNotification>
    , PubSub::Subscriber<IpRequest, LoginRequest>
{
	Q_OBJECT

public:
	Controller();
	~Controller();

private:
    void notify(IpRequest const&) override;
    void notify(LoginRequest const&) override;

private:
    void parseConfig(QNetworkReply*);

private slots:
    void replyFinished(QNetworkReply*);

private:
    QNetworkAccessManager *m_manager;
    QMap<QNetworkReply*, std::function<void(QNetworkReply*)>> m_reply2action;
    struct ConfigManager;
    std::unique_ptr<ConfigManager> m_configManager;
};

}

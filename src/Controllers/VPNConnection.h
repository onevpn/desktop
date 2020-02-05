#pragma once

#include <memory>
#include <QObject>

#include "pubsub/Publisher.h"
#include "pubsub/Subscriber.h"

#include "DAOs/VPNConnection.h"
#include "Interfaces/AbstractConnection.h"

class ConfigManager;

namespace VPNConnection {

class Controller
	: public QObject
    , PubSub::Subscriber
        <ConnectRequest
        , DisconnectRequest
        , GettingConfigRequest>
    , PubSub::Publisher
        <ConnectNotification
        , DisconnectNotification
        , ConnectErrorNotification
        , GettingConfigFailedNotification
        >
{
	Q_OBJECT

public:
	Controller();
	~Controller();

	bool initialize();

private slots:
	void onConnected();
	void onDisconnected();
	void onConnectionError(const QString &);
	void onLog(const QString &);
	void onStatisticsChanged(quint64, quint64);

private:
	void notify(ConnectRequest const&) override;
	void notify(DisconnectRequest const&) override;
    void notify(GettingConfigRequest const&) override;

private:
    std::unique_ptr<AbstractConnection> m_impl;
    std::unique_ptr<ConfigManager> m_configManager;
};

}

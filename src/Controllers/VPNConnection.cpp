#include "VPNConnection.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QDebug>
#include <QUrl>

#include "ConnectionFactory.h"
#include "ConfigManager.h"
#include "DAOs/configs.h"

namespace VPNConnection {

Controller::Controller()
	: PubSub::Agent("VPNConnection::Controller")
	, m_impl(ConnectionFactory().createConnection())
    , m_configManager(new ConfigManager)
{
	subscribeAll<VPNConnection::Object>();

	connect(m_impl.get(), &AbstractConnection::connected        , this, &Controller::onConnected);
	connect(m_impl.get(), &AbstractConnection::disconnected     , this, &Controller::onDisconnected);
	connect(m_impl.get(), &AbstractConnection::error            , this, &Controller::onConnectionError);
	connect(m_impl.get(), &AbstractConnection::log              , this, &Controller::onLog);
	connect(m_impl.get(), &AbstractConnection::statisticsChanged, this, &Controller::onStatisticsChanged);
}

bool Controller::initialize()
{
	bool res = m_impl->initialize();
	return res;
}

Controller::~Controller() = default;

void Controller::notify(VPNConnection::ConnectRequest const& r)
{
    m_configManager->generate(r.serverIP, r.protocol, r.port);
    auto const conf = Config::ConfigRequester(*this).getConfig();
    m_impl->connect(OPENVPN, r.serverIP, conf.auth.username, conf.auth.password, m_configManager->path(), "");
}

void Controller::notify(DisconnectRequest const& r)
{
    Q_UNUSED(r);
	m_impl->disconnect();
}

void Controller::onConnected()
{
    publish(ConnectNotification(nullptr));
}

void Controller::onDisconnected()
{
    publish(DisconnectNotification(nullptr));
}

void Controller::onConnectionError(const QString & message)
{
    publish(ConnectErrorNotification(nullptr, message));
}

void Controller::onLog(const QString &)
{
}

void Controller::onStatisticsChanged(quint64, quint64)
{
}

void Controller::notify(GettingConfigRequest const& r)
{
    m_configManager->get(r.username, r.password);
}

}

#include "connection.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

OpenVPNConnectorQt *g_openVPNConnection = NULL;

Connection::Connection(QObject *parent)
    : AbstractConnection(parent)
{
    qRegisterMetaType<OPENVPN_ERROR>("OPENVPN_ERROR");

    g_openVPNConnection = new OpenVPNConnectorQt(this);

    QObject::connect(g_openVPNConnection, SIGNAL(connected()), SLOT(onOpenVPNConnected()));
    QObject::connect(g_openVPNConnection, SIGNAL(disconnected()), SLOT(onOpenVPNDisconnected()));
    QObject::connect(g_openVPNConnection, SIGNAL(error(OPENVPN_ERROR)), SLOT(onOpenVPNError(OPENVPN_ERROR)));
    QObject::connect(g_openVPNConnection, SIGNAL(log(QString)), SLOT(onOpenVPNLog(QString)));
    QObject::connect(g_openVPNConnection, SIGNAL(statisticsUpdated(quint64,quint64)), SIGNAL(statisticsChanged(quint64,quint64)));
}

Connection::~Connection()
{
    delete g_openVPNConnection;
}

/*
void Connection::customEvent( QEvent *event )
{
	if (event->type() == (QEvent::Type)UM_CONNECTED)
	{
		emit connected();
		bFirstCalcStat_ = true;
		prevBytesRcved_ = 0;
		prevBytesXmited_ = 0;
		timerId_ = startTimer(1000);
	}
	else if (event->type() == (QEvent::Type)UM_ERROR)
	{
        Connection::disconnect();
		emit error(g_str);
	}
}*/

bool Connection::connect(PROTOCOL protocol, QString serverIP, QString username, QString password, QString ovpnFile, QString l2tpKey)
{
    if (protocol == OPENVPN)
	{
        emit log("Do connect to OpenVPN");
        g_openVPNConnection->connect(ovpnFile, username, password, "", "");
        return true;
    }

    return false;
}

void Connection::disconnect()
{
    g_openVPNConnection->disconnect();
}

void Connection::onOpenVPNConnected()
{
    m_isConnected = true;
    emit connected();
}

void Connection::onOpenVPNDisconnected()
{
    m_isConnected = false;
    emit disconnected();
}

void Connection::onOpenVPNError(OPENVPN_ERROR err)
{
    if (err == AUTH_ERROR)
    {
        emit error(tr("Incorrect username or password"));
    }
    else if (err == CANNOT_ALLOCATE_TUN_TAP)
    {
        emit error(tr("There are no TAP adapters on this system. Please install."));
    }
    else
    {
        emit error(tr("Error during connection (") + QString::number(err) + tr(")"));
    }
}

void Connection::onOpenVPNLog(const QString &logStr)
{
    emit log(logStr);
}

bool Connection::initialize()
{
    //return g_openVPNConnection->installHelper("OneVPNService");
    return true;
}

bool Connection::tapInstalled()
{
    return true;
}

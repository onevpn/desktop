#ifndef CONNECTION_H
#define CONNECTION_H

#include "../Utils.h"
#include <QObject>
#include <SystemConfiguration/SCSchemaDefinitions.h>
#include <SystemConfiguration/SCNetwork.h>
#include <SystemConfiguration/SCNetworkConnection.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#include "OpenVPNConnectorQt.h"

class Connection : public QObject
{
	Q_OBJECT

public:
	Connection(QObject *parent);
	~Connection();

    bool initialize();
    bool connect(PROTOCOL protocol, QString serverIP, QString username, QString password, QString ovpnFile, QString l2tpKey, QStringList dns);
    void disconnect();
    bool tapInstalled();

    bool isConnected() const { return bConnected_; }

signals:
    void connected();
    void disconnected();
    void error(const QString &error);
    void log(const QString &logStr);
    void statisticsChanged(quint64 download, quint64 upload);

protected:
    void timerEvent(QTimerEvent *event);

private slots:
    void onOpenVPNConnected();
    void onOpenVPNDisconnected();
    void onOpenVPNError(OPENVPN_ERROR err);
    void onOpenVPNLog(const QString &logStr);

private:
    SCPreferencesRef m_prefs;
    CFStringRef m_appName;
    CFStringRef m_l2tpServiceName;
    CFStringRef m_pptpServiceName;
    CFStringRef m_l2tpServiceId;
    CFStringRef m_pptpServiceId;

    SCNetworkServiceRef m_l2tpService;
    SCNetworkServiceRef m_pptpService;

    SCNetworkConnectionRef m_connection;
    bool    bConnected_;

    int	timerId_;
    bool bFirstCalcStat_;
    SInt64 prevBytesRcved_;
    SInt64 prevBytesXmited_;
    bool   bDisconnectInitiatedByUser_;

    CFStringRef initService(CFStringRef type ,CFStringRef name);
    SCNetworkConnectionRef initConnection(CFStringRef serviceId , CFDictionaryRef options);

    bool connectPPTP(QString serverIP, QString inUsername, QString inPassword);
    bool connectL2TP(QString serverIP, QString inUsername, QString inPassword, QString inL2tpKey);

    static void callback(SCNetworkConnectionRef, SCNetworkConnectionStatus, void *);
};

#endif // CONNECTION_H

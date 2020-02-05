#pragma once

#include "Interfaces/AbstractConnection.h"

#include <SystemConfiguration/SCSchemaDefinitions.h>
#include <SystemConfiguration/SCNetwork.h>
#include <SystemConfiguration/SCNetworkConnection.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#include "OpenVPNConnectorQt.h"

class Connection : public AbstractConnection
{
	Q_OBJECT

public:
    Connection(QObject *parent = nullptr);
	~Connection();

    bool initialize() override;
    bool connect(PROTOCOL protocol
				 , QString serverIP
				 , QString username
				 , QString password
				 , QString ovpnFile
                 , QString l2tpKey) override;

    void disconnect() override;
    bool tapInstalled() override;
    bool isConnected() const override { return bConnected_; }

protected:
    void timerEvent(QTimerEvent *event) override;

private slots:
    void onOpenVPNConnected();
    void onOpenVPNDisconnected();
    void onOpenVPNError(OPENVPN_ERROR err);
    void onOpenVPNLog(const QString &logStr);

private:
    SCPreferencesRef m_prefs;
    CFStringRef      m_appName;
    CFStringRef      m_l2tpServiceName;
    CFStringRef      m_pptpServiceName;
    CFStringRef      m_l2tpServiceId;
    CFStringRef      m_pptpServiceId;

    SCNetworkServiceRef m_l2tpService;
    SCNetworkServiceRef m_pptpService;

    SCNetworkConnectionRef m_connection;
    bool    bConnected_;

    int	   timerId_;
    bool   bFirstCalcStat_;
    SInt64 prevBytesRcved_;
    SInt64 prevBytesXmited_;
    bool   bDisconnectInitiatedByUser_;

    CFStringRef initService(CFStringRef type ,CFStringRef name);
    SCNetworkConnectionRef initConnection(CFStringRef serviceId , CFDictionaryRef options);

    bool connectPPTP(QString serverIP, QString inUsername, QString inPassword);
    bool connectL2TP(QString serverIP, QString inUsername, QString inPassword, QString inL2tpKey);

    static void callback(SCNetworkConnectionRef, SCNetworkConnectionStatus, void *);
};

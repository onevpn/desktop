#pragma once

#include <memory>

#include <QWidget>
#include <QMap>
#include <QScopedPointer>
#include <QPointer>

#include "formlogin.h"
#include "formloginwaitanimation.h"
#include "formconnect.h"
#include "serversmodel.h"
#include "dnsleaks.h"
#include "killswitch.h"

#include <QDialog>
#include <QSystemTrayIcon>

#include "dnsleaks.h"

#include "pubsub/Subscriber.h"
#include "DAOs/VPNConnection.h"

namespace Ui { class MMainWindow; }
class TrayPopover;

class MMainWindow
	: public QDialog
    , PubSub::Subscriber
        < VPNConnection::ConnectNotification
        , VPNConnection::DisconnectNotification
        , VPNConnection::ConnectErrorNotification
        , ServerSide::LoginOkNotification
        , ServerSide::LoginFailedNotification>
{
    Q_OBJECT
public:
    MMainWindow(QWidget * = nullptr);
    ~MMainWindow();

private:
    void showEvent(QShowEvent *) override;
    void closeEvent(QCloseEvent *) override;

private slots:
    void doConnection(QString const&, Config::Protocol const&);
    void doDisconnection();

    void onAcceptClicked(QString const&, QString const&);

    void onConnectDisconnect();
    void onSettings();

    void gotoFilling();
    void formHaveAccount();
    void signUp();
    void gotoPreferences();
    void gotoAccountPage();
    void forceQuit();
    void restoreFromTray();
    void serverChanged(int);

    void trayVpnConnection();
    void handlePopover();
    void restoreMainWindow();

private:
	void notify(VPNConnection::ConnectNotification const&) override;
	void notify(VPNConnection::DisconnectNotification const&) override;
    void notify(VPNConnection::ConnectErrorNotification const&) override;
    void notify(ServerSide::LoginOkNotification const&) override;
    void notify(ServerSide::LoginFailedNotification const&) override;

private:
    QScopedPointer<Ui::MMainWindow> m_ui;
    QSystemTrayIcon m_trayIcon;

    ServersModel *m_serversModel;
    bool m_needToQuit = false;
    bool m_bWasDisconnectClick;

    QScopedPointer<DnsLeaks> m_dnsLeaks;
    KillSwitch *m_killSwitch;

    QStringList m_logs;

    quint64 m_lastDownload = 0ULL;
    quint64 m_lastUpload   = 0ULL;
    qint64  m_lastStatTime = 0LL;

    QList<QAction*> m_serversActions;
    QAction* m_disconnectAction;
    bool m_isConnected = false;

    QPointer<TrayPopover> m_trayPopover;

	class ConnectionImpl;
	std::unique_ptr<ConnectionImpl> m_connectionImpl;
};

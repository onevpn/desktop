#pragma once

#include <QWidget>
#include <QMap>
#include <QScopedPointer>
#include "getmyip.h"
#include "serverapi.h"
#include "ovpnconfig.h"
#include "formlogin.h"
#include "formloginwaitanimation.h"
#include "formconnect.h"
#include "serversmodel.h"
#include "dnsleaks.h"
#include "killswitch.h"
#include "paranoik.h"

#include <QDialog>
#include <QSystemTrayIcon>

#if defined Q_OS_MAC
    #include "Mac/connection.h"
#elif defined Q_OS_WIN
    #include "Windows/connection.h"
#elif defined Q_OS_UNIX
    #include "Linux/connection.h"
#endif

#include "dnsleaks.h"
#include "getmyip.h"

class QMenu;

namespace Ui { class MMainWindow; }

class MMainWindow : public QDialog
{
    Q_OBJECT
public:
    MMainWindow(QWidget * = Q_NULLPTR);
    ~MMainWindow();

private:
    void fillServers();

    void showEvent(QShowEvent *) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;
    void initTrayMenu();

private slots:
    void doConnection(QString const&, TProtocol const&);
    void doDisconnection();

    void onAcceptClicked(QString const&, QString const&);
    void onLoginOk();
    void onLoginFailed(const QString &);

    /// @NOTE connection interface
    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString &);
    void onLog(const QString &);
    void onStatisticsChanged(quint64, quint64);

    void onIpFinished(const QString &ip, const QString &countryCode);
    void onStartGetIP();
    void resetSpeedCounters();
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

    void onParanoik();
    void trayVpnConnection();

private:
    QScopedPointer<Ui::MMainWindow> m_ui;
    QSystemTrayIcon m_trayIcon;

    ServerAPI *m_serverAPI;
    OVPNConfig m_ovpnConfig;
    ServersModel *m_serversModel;
    bool m_needToQuit;
    bool m_bWasDisconnectClick;

    Connection *m_connection;
    QScopedPointer<DnsLeaks> m_dnsLeaks;
    KillSwitch *m_killSwitch;

    QStringList m_logs;

    quint64 m_lastDownload = 0ULL;
    quint64 m_lastUpload   = 0ULL;
    qint64  m_lastStatTime = 0LL;
    QMenu * m_trayMenu;

    GetMyIp *m_getMyIP;
    Paranoik* m_paranoik;

    QList<QAction*> m_serversActions;
    QAction* m_disconnectAction;
    bool m_isConnected;
};

#include "mmainwindow.h"
#include "ui_mmainwindow.h"
#include "settings.h"
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QDesktopWidget>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QAction>
#include <QDateTime>
#include <QFile>

#include "dialogrestoreinternet.h"
#include "dialogsettings.h"
#include "showlog.h"
#include "formhaveaccount.h"
#include "utils.h"
#include "platformutils.h"
#include "settingskeys.h"

MMainWindow::MMainWindow(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::MMainWindow)
    , m_serverAPI(new ServerAPI(this))
    , m_serversModel(new ServersModel(this))
    , m_connection(new Connection(this))
    , m_dnsLeaks(new DnsLeaks())
    , m_killSwitch(new KillSwitch(this))
    , m_needToQuit(false)
    , m_paranoik(new Paranoik(this))
{
    m_ui->setupUi(this);
    setWindowTitle(QStringLiteral("OneVPN"));

    m_trayMenu = Q_NULLPTR;
    m_bWasDisconnectClick = false;
    m_disconnectAction = Q_NULLPTR;

    QIcon wIcon(QStringLiteral(":/OneVPN.ico"));
    m_trayIcon.setIcon(wIcon);
#ifdef Q_OS_UNIX
    setWindowIcon(wIcon);
#endif
    QFile qss(QStringLiteral(":/style/MainWindow.qss"));
    qss.open(QIODevice::ReadOnly);
    setStyleSheet(qss.readAll());

    m_isConnected = false;

    connect(m_connection, SIGNAL(connected()),         this, SLOT(onConnected()));
    connect(m_connection, SIGNAL(disconnected()),      this, SLOT(onDisconnected()));
    connect(m_connection, SIGNAL(error(QString)),      this, SLOT(onConnectionError(QString)));
    connect(m_connection, SIGNAL(log(QString)),        this, SLOT(onLog(QString)));
    connect(m_connection, SIGNAL(statisticsChanged(quint64,quint64)), this, SLOT(onStatisticsChanged(quint64,quint64)));

    connect(m_ui->accountWidget, SIGNAL(yes()), SLOT(formHaveAccount()));
    connect(m_ui->accountWidget, SIGNAL(no()), SLOT(signUp()));

    connect(m_ui->loginWidget, SIGNAL(acceptClicked(QString,QString)), this, SLOT(onAcceptClicked(QString,QString)));

    connect(m_serverAPI, SIGNAL(loginOk()),     this, SLOT(onLoginOk()));
    connect(m_serverAPI, SIGNAL(loginFailed(QString)), this, SLOT(onLoginFailed(QString)));

    connect(m_ui->connectWidget, SIGNAL(gotoPreferences()), this, SLOT(gotoPreferences()));
    connect(m_ui->preferencesWidget, SIGNAL(goBack()), this, SLOT(gotoAccountPage()));

    if (!m_connection->initialize())
    {
        QMessageBox::information(this, QApplication::applicationName(), tr("OpenVPN helper initialize error"));
        QTimer::singleShot(0, this, SLOT(close()));
    }
    if (m_dnsLeaks->isEnable())
        m_dnsLeaks->enable(false);

    if (m_killSwitch->isActive())
        m_killSwitch->restoreInternet();

    m_getMyIP = new GetMyIp(this);
    connect(m_getMyIP, SIGNAL(myIpFinished(QString,QString)), this,  SLOT(onIpFinished(QString,QString)));
    onStartGetIP();

    connect(m_ui->connectWidget, SIGNAL(connectionRequested(bool)), this, SLOT(onConnectDisconnect()));
    connect(m_ui->connectWidget, SIGNAL(settingsRequested(bool)),   this, SLOT(onSettings()));
    connect(m_ui->connectWidget, SIGNAL(logoutRequested()),     this, SLOT(gotoFilling()));

    connect(m_paranoik, SIGNAL(shouldRestart()), this, SLOT(onParanoik()));
    connect(m_ui->connectWidget, SIGNAL(serverChanged(int)), this, SLOT(serverChanged(int)));

    const QString FIRST_LAUNCH_KEY = QStringLiteral("firstLaunch");
    QSettings settings;
    const bool isFirstStart = settings.value(FIRST_LAUNCH_KEY, true).toBool();
    if(isFirstStart)
        settings.setValue(FIRST_LAUNCH_KEY, false);
    else
        formHaveAccount();
}

MMainWindow::~MMainWindow()
{
}

void MMainWindow::gotoFilling()
{
    m_ui->loginWidget->gotoFilling();
    m_ui->stackedWidget->setCurrentWidget(m_ui->login);
}

void MMainWindow::gotoAccountPage()
{
    m_ui->stackedWidget->setCurrentWidget(m_ui->account);
}

void MMainWindow::formHaveAccount()
{
    m_ui->stackedWidget->setCurrentWidget(m_ui->login);
}

void MMainWindow::signUp()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://onevpn.co/members/signup.php")));
}

void MMainWindow::gotoPreferences()
{
//    int ind = m_ui->connectWidget->currentServer();
//    const TConfig &cfg = m_serverAPI->config();
//    m_ui->preferencesWidget->setCurrentServer(cfg.servers[ind]);
//    m_ui->preferencesWidget->setPlan(cfg.plan);
//    m_ui->preferencesWidget->setMessage(cfg.message);

    m_ui->stackedWidget->setCurrentWidget(m_ui->preferences);
}

void MMainWindow::serverChanged(int index)
{
    const TConfig &cfg = m_serverAPI->config();
    m_ui->preferencesWidget->setCurrentServer(cfg.servers[index]);
    m_ui->preferencesWidget->setPlan(cfg.plan);
    m_ui->preferencesWidget->setMessage(cfg.message);
}

void MMainWindow::initTrayMenu()
{
    m_serversActions.clear();
    if(m_trayMenu)
    {
        delete m_trayMenu;
        m_trayMenu = nullptr;
    }
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(tr("Show Window"), this, SLOT(restoreFromTray()));
    m_trayMenu->addAction(tr("Exit"), this, SLOT(forceQuit()));

    m_disconnectAction = new QAction("Drop connection", m_trayMenu);
    m_trayMenu->addAction(m_disconnectAction);
    connect(m_disconnectAction, SIGNAL(triggered(bool)), this, SLOT(doDisconnection()));
    m_disconnectAction->setEnabled(m_isConnected);

    QVector<TServer> servers = m_serverAPI->config().servers;
    for(int i = 0; i < servers.size(); ++i)
    {
        m_trayMenu->addSeparator();
        TServer const& s = servers[i];
        foreach(TProtocol const& pr, s.protocols)
        {
            QAction* action = new QAction(QString("%1 %2").arg(s.name).arg(pr.toString()), m_trayMenu);
            m_serversActions.push_back(action);
            action->setEnabled(!m_isConnected);
            action->setProperty("dns"         , s.dns);
            action->setProperty("port"        , pr.port);
            action->setProperty("protocol"    , pr.protocol);
            action->setProperty("serialnumber", i);
            connect(action, SIGNAL(triggered(bool)), SLOT(trayVpnConnection()));
            m_trayMenu->addAction(action);
        }
    }

    m_trayIcon.setContextMenu(m_trayMenu);
}

void MMainWindow::trayVpnConnection()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;

    const QString dns               = action->property("dns").toString();
    const int port                  = action->property("port").toInt();
    const PROTOCOL_OPENVPN protocol = static_cast<PROTOCOL_OPENVPN>(action->property("protocol").toInt());
    const int serialNumber          = action->property("serialnumber").toInt();

    m_ui->connectWidget->setCurrentServerIndex(serialNumber);
    QSettings settings;
    settings.setValue(Keys::LastServer, serialNumber);
    doConnection(dns, TProtocol(protocol, port));
}

void MMainWindow::forceQuit()
{
    m_needToQuit = true;
    close();
}

void MMainWindow::restoreFromTray()
{
//#ifdef Q_OS_MAC
//    [NSApp setActivationPolicy: NSApplicationActivationPolicyProhibited];
//#endif
    showInDock();
    show();
}

void MMainWindow::onAcceptClicked(QString const& username, QString const& password)
{
    qDebug() << username << password;
    m_serverAPI->login(username, password);
    m_ui->loginWidget->gotoWaiting();
}

void MMainWindow::onLoginOk()
{
    m_ovpnConfig.get(m_serverAPI->username(), m_serverAPI->password());
    m_ui->loginWidget->writeSettings();

    const TConfig &cfg = m_serverAPI->config();
    QVector<TServer const*> servers;
    for (QVector<TServer>::const_iterator it = cfg.servers.begin(); it != cfg.servers.end(); ++it)
       servers << &(*it);

    m_serversModel->setServers(servers);
    m_ui->stackedWidget->setCurrentWidget(m_ui->account);
//    changeForm(formConnect_);
    fillServers();

    QString strRemaining = cfg.bandwidth.sizeInStr() + " remaining";
//    formConnect_->ui->lblRemaining->setText(strRemaining);
}

void MMainWindow::onLoginFailed(const QString &msg)
{
    m_ui->loginWidget->gotoFilling();
    m_ui->loginWidget->setErrorMessage("Sorry, your login does't exist");
}

void MMainWindow::fillServers()
{
    const TConfig &config = m_serverAPI->config();
    m_ui->connectWidget->fillServers(m_serverAPI->config());
}

void MMainWindow::showEvent(QShowEvent *event)
{
    if(m_trayIcon.isVisible())
        m_trayIcon.hide();
    QWidget::showEvent(event);
}

void MMainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_needToQuit)
    {
        initTrayMenu();
        event->ignore();
        m_trayIcon.show();
        hideFromDock();
//#ifdef Q_OS_MAC
//        [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];
//#endif
        hide();
        return;
    }
    QWidget::closeEvent(event);
}

void MMainWindow::onConnected()
{
    m_isConnected = true;
    qDebug() << "onConnected";
    m_ui->connectWidget->setConnectingState(FormConnect::CONNECT_BUTTON_ON);
    m_ui->connectWidget->enableAll(true);

    if(m_trayMenu)
    {
        foreach(QAction* action, m_serversActions)
            action->setEnabled(false);
    }
    if(m_disconnectAction)
        m_disconnectAction->setEnabled(true);

    QSettings settings;
    if (settings.value("dnsLeak").toBool())
        m_dnsLeaks->enable(true);
    QTimer::singleShot(1000, this, &MMainWindow::onStartGetIP);
}

void MMainWindow::onDisconnected()
{
    m_isConnected = false;
    qDebug() << "onDisconnected";
    m_dnsLeaks->enable(false);

    if(m_trayMenu)
    {
        foreach(QAction* action, m_serversActions)
            action->setEnabled(true);
    }
    if(m_disconnectAction)
        m_disconnectAction->setEnabled(false);

    if (!m_bWasDisconnectClick && m_ui->connectWidget->connectingState() != FormConnect::CONNECT_BUTTON_OFF) // disconnect without user iteraction
    {
        QSettings settings;
        if (settings.value("killSwitch").toBool())
        {
            m_killSwitch->killInternet();

//            DialogRestoreInternet dlg(this);
//            if (dlg.exec() == QDialog::Accepted)
//            {
//                if (dlg.getResult() == DialogRestoreInternet::RESTORE)
//                {
//                    m_killSwitch->restoreInternet();
//                }
//                else if (dlg.getResult() == DialogRestoreInternet::RESTORE_AND_RECONNECT)
//                {
//                    m_killSwitch->restoreInternet();
//                    QThread::msleep(100);
//                    onClickConnect();
//                }
//                else
//                {
//                    QMessageBox::information(this, QApplication::applicationName(), "Internet will be restored after exiting the program.");
//                }
//            }
        }
    }

    m_ui->connectWidget->enableAll(true);
    m_ui->connectWidget->setConnectingState(FormConnect::CONNECT_BUTTON_OFF);
    QTimer::singleShot(1000, this, &MMainWindow::onStartGetIP);
    resetSpeedCounters();
}

void MMainWindow::onConnectionError(const QString & error)
{
    qDebug() << "onConnectionError: " << error;
    if (!error.isEmpty())
        QMessageBox::information(this, QApplication::applicationName(), error);

    m_ui->connectWidget->enableAll(true);
    m_ui->connectWidget->setConnectingState(FormConnect::CONNECT_BUTTON_OFF);
    resetSpeedCounters();
}

void MMainWindow::onLog(const QString & log)
{
    qDebug() << log;
    m_logs << log;
}

void MMainWindow::onStatisticsChanged(quint64 download, quint64 upload)
{
    // if (m_ui->connectWidget->connectingState() == FormConnect::CONNECT_BUTTON_ON)
    // {
    //     if (m_lastStatTime == 0)
    //     {
    //         m_lastStatTime = QDateTime::currentMSecsSinceEpoch();
    //         m_lastDownload = download;
    //         m_lastUpload = upload;
    //     }
    //     else
    //     {
    //         qint64 time = QDateTime::currentMSecsSinceEpoch();
    //         double periodInSec = ((double)time - (double)m_lastStatTime) / 1000.0;

    //         double d = ((double)download - (double)m_lastDownload) / periodInSec;
    //         double u = ((double)upload - (double)m_lastUpload) / periodInSec;

    //         m_ui->connectWidget->setDownloaded(Utils::sizeInStr(d).toUpper() + "/s");
    //         m_ui->connectWidget->setUploaded(Utils::sizeInStr(d).toUpper() + "/s");

    //         m_lastStatTime = QDateTime::currentMSecsSinceEpoch();
    //         m_lastDownload = download;
    //         m_lastUpload = upload;
    //     }
    // }
}

void MMainWindow::onIpFinished(const QString &ip, const QString &countryCode)
{
    m_ui->connectWidget->setIP(ip, countryCode);
}

void MMainWindow::onStartGetIP()
{
    ///@TODO burn with fire
    m_getMyIP->start(QThread::LowPriority);
}

void MMainWindow::resetSpeedCounters()
{
    m_ui->connectWidget->setDownloaded("0 MB/s");
    m_ui->connectWidget->setUploaded("0 MB/s");
}

void MMainWindow::onParanoik()
{
//    if(m_connection->isConnected())
//    {
//        doDisconnection();
//        doConnection();
//    }
}

void MMainWindow::doDisconnection()
{
    m_bWasDisconnectClick = true;
    m_connection->disconnect();
    m_paranoik->stop();
}

void MMainWindow::doConnection(QString const& dns, TProtocol const& protocol)
{
    if (!m_ovpnConfig.configReceived())
    {
        QMessageBox::information(this, QApplication::applicationName(), tr("Failed get configs from server. Please contact support"));
        return;
    }

    m_ovpnConfig.generate(dns, protocol.protocol, protocol.port);

    if (!m_ovpnConfig.makeSuccess())
    {
        QMessageBox::information(this, QApplication::applicationName(), tr("Failed create OVPN file"));
        return;
    }
    m_lastStatTime = 0;
    m_bWasDisconnectClick = false;

    if (m_killSwitch->isActive())
        m_killSwitch->restoreInternet();

    QSettings settings;
    if (settings.value("killSwitch").toBool())
    {
        m_killSwitch->saveDefaultGateway();
        m_paranoik->start();
    }

    QString username = m_serverAPI->config().auth.username;
    QString password = m_serverAPI->config().auth.password;

    m_connection->connect(OPENVPN, dns, username, password, m_ovpnConfig.path(), "", QStringList());

    m_ui->connectWidget->setConnectingState(FormConnect::CONNECT_BUTTON_CONNECTING);
    m_ui->connectWidget->enableAll(false);
}

void MMainWindow::onConnectDisconnect()
{
    const int state = m_ui->connectWidget->connectingState();

    if (state == FormConnect::CONNECT_BUTTON_OFF)
    {
        const int ind = m_ui->connectWidget->currentServer();
        QSettings settings;
        settings.setValue(Keys::LastServer, ind);
        const TConfig &cfg = m_serverAPI->config();
        const TServer &si = cfg.servers[ind];

        doConnection(si.dns, m_ui->preferencesWidget->currentProtocol());
    }
    else if (state == FormConnect::CONNECT_BUTTON_ON)
    {
        doDisconnection();
    }
    else if (state == FormConnect::CONNECT_BUTTON_CONNECTING || state == FormConnect::CONNECT_BUTTON_ERROR)
    {
        doDisconnection();
    }
    else Q_ASSERT(false);
}

void MMainWindow::onSettings()
{
//    DialogSettings dlg(this);
//    dlg.exec();
}

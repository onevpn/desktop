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
#include "formhaveaccount.h"
#include "utils.h"
#include "platformutils.h"
#include "settingskeys.h"

#include "pubsub/Publisher.h"
#include "pubsub/Subscriber.h"

#include "DAOs/VPNConnection.h"
#include "DAOs/ServerSide.h"

#include "traypopover.h"

#include <memory>

class MMainWindow::ConnectionImpl
    : public PubSub::Publisher
        < VPNConnection::ConnectRequest
        , VPNConnection::DisconnectRequest
        , VPNConnection::GettingConfigRequest>
{
	std::unique_ptr<VPNConnection::Object> m_object;
public:
	ConnectionImpl() : PubSub::Agent("ConnectionImpl"), m_object(new VPNConnection::Object) {}

    void connect(PROTOCOL_OPENVPN protocol
			  , QString const& serverIP
              , int port)
    {
        publish(VPNConnection::ConnectRequest(m_object.get(), protocol, serverIP, m_username, m_password, port));
    }

	void disconnect()
	{ publish(VPNConnection::DisconnectRequest(m_object.get())); }

    void getConfig()
    {
        publish(VPNConnection::GettingConfigRequest(nullptr, m_username, m_password));
    }

    void setUsername(QString const& v) { m_username = v; }
    void setPassword(QString const& v) { m_password = v; }

    QString username() const { return m_username; }
    QString password() const { return m_password; }

private:
    QString m_username;
    QString m_password;
};

namespace {

struct IpHelper : PubSub::Publisher<ServerSide::IpRequest>
{
    IpHelper() : PubSub::Agent("IpHelper") { }
    void requestIp() { publish(ServerSide::IpRequest(nullptr)); }
};

}

MMainWindow::MMainWindow(QWidget *parent)
    : PubSub::Agent("MainWindow")
	, QDialog(parent)
    , m_ui(new Ui::MMainWindow)
    , m_serversModel(new ServersModel(this))
    , m_dnsLeaks(new DnsLeaks())
    , m_killSwitch(new KillSwitch(this))
	, m_connectionImpl(new ConnectionImpl())
{
	subscribeAll<VPNConnection::Object>();
    subscribeAll<ServerSide::Object>();

    m_ui->setupUi(this);
    setWindowTitle(QStringLiteral("OneVPN"));

    m_bWasDisconnectClick = false;
    m_disconnectAction = Q_NULLPTR;

    QIcon wIcon(QStringLiteral(":/AppIcon.ico"));
    m_trayIcon.setIcon(wIcon);

#ifdef Q_OS_MAC
    connect(&m_trayIcon, &QSystemTrayIcon::activated, this, &MMainWindow::handlePopover);
#else
    QMenu* trayMenu = new QMenu;
    QAction* fullMode = new QAction("Full mode", this);
    QAction* quitApp = new QAction("Quit", this);
    connect(fullMode, &QAction::triggered, this, &MMainWindow::restoreFromTray);
    connect(quitApp, &QAction::triggered, [app = qApp]{ app->quit(); });
	connect(&m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason)
		{
			if (reason == QSystemTrayIcon::Trigger)
				restoreFromTray();
		});
    trayMenu->addAction(fullMode);
    trayMenu->addAction(quitApp);
    m_trayIcon.setContextMenu(trayMenu);
    setWindowIcon(wIcon);
#endif
    QFile qss(QStringLiteral(":/style/MainWindow.qss"));
    qss.open(QIODevice::ReadOnly);
    setStyleSheet(qss.readAll());

    connect(m_ui->accountWidget, SIGNAL(yes()), SLOT(formHaveAccount()));
    connect(m_ui->accountWidget, SIGNAL(no()), SLOT(signUp()));

    connect(m_ui->loginWidget, SIGNAL(acceptClicked(QString,QString)), this, SLOT(onAcceptClicked(QString,QString)));

    connect(m_ui->connectWidget, SIGNAL(gotoPreferences()), this, SLOT(gotoPreferences()));
    connect(m_ui->preferencesWidget, SIGNAL(goBack()), this, SLOT(gotoAccountPage()));

    if (m_dnsLeaks->isEnable())
        m_dnsLeaks->enable(false);

    if (m_killSwitch->isActive())
        m_killSwitch->restoreInternet();

    IpHelper().requestIp();

    connect(m_ui->connectWidget, SIGNAL(connectionRequested(bool)), this, SLOT(onConnectDisconnect()));
    connect(m_ui->connectWidget, SIGNAL(settingsRequested(bool)),   this, SLOT(onSettings()));
    connect(m_ui->connectWidget, SIGNAL(logoutRequested()),     this, SLOT(gotoFilling()));

    connect(m_ui->connectWidget, &FormConnect::serverChanged, this, &MMainWindow::serverChanged);

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

void MMainWindow::handlePopover()
{
    if(m_trayPopover && m_trayPopover->isVisible())
    {
        m_trayPopover->hide();
        return;
    }

    assert(!m_trayPopover);
    m_trayPopover = new TrayPopover(this);
    connect(m_trayPopover.data(), &TrayPopover::gotoFullMode, this, &MMainWindow::restoreMainWindow);
    connect(m_trayPopover.data(), &TrayPopover::closed, [this]
    { m_ui->gridLayout_3->addWidget(m_ui->connectWidget, 0, 0, 1, 1); });

    auto g = m_trayIcon.geometry();
    m_trayPopover->move(g.bottomLeft());
    m_trayPopover->steal(m_ui->connectWidget);
    m_trayPopover->exec();
}

void MMainWindow::restoreMainWindow()
{
    restoreFromTray();
    if(m_trayPopover)
        m_trayPopover->hide();
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
    m_ui->stackedWidget->setCurrentWidget(m_ui->preferences);
}

void MMainWindow::serverChanged(int index)
{
    auto const cfg = Config::ConfigRequester(*this).getConfig();
    m_ui->preferencesWidget->setCurrentServer(cfg.servers[index]);
    m_ui->preferencesWidget->setPlan(cfg.plan);
    m_ui->preferencesWidget->setMessage(cfg.message);
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
    doConnection(dns, Config::Protocol(protocol, port));
}

void MMainWindow::forceQuit()
{
    m_needToQuit = true;
    QApplication::exit();
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
    {
        struct Publisher : PubSub::Publisher<ServerSide::LoginRequest>
        {
            Publisher() : PubSub::Agent("MainWindow::Publisher") {  }
        } p;
        p.publish(ServerSide::LoginRequest(nullptr, username, password));
    }
    m_connectionImpl->setUsername(username);
    m_connectionImpl->setPassword(password);

    m_ui->loginWidget->gotoWaiting();
}

void MMainWindow::showEvent(QShowEvent *event)
{
    //todo workaround - cause crash on mac
    QTimer::singleShot(1000, [this]
    {
        if(m_trayIcon.isVisible())
            m_trayIcon.hide();
    });

    QWidget::showEvent(event);
}

void MMainWindow::closeEvent(QCloseEvent *event)
{
    if(m_ui->stackedWidget->currentWidget() == m_ui->login)
    {
        QWidget::closeEvent(event);
        return;
    }

    if (!m_needToQuit)
    {
        event->ignore();
        m_trayIcon.show();
        hideFromDock();
        hide();
        return;
    }
    QWidget::closeEvent(event);
}

void MMainWindow::notify(VPNConnection::ConnectNotification const& n)
{
	Q_UNUSED(n);
	m_isConnected = true;
	qDebug() << "onConnected";
	m_ui->connectWidget->setConnectingState(FormConnect::CONNECT_BUTTON_ON);
	m_ui->connectWidget->enableAll(true);

	if(m_disconnectAction)
		m_disconnectAction->setEnabled(true);

	QSettings settings;
	if (settings.value("dnsLeak").toBool())
		m_dnsLeaks->enable(true);
    QTimer::singleShot(1000, []{ IpHelper().requestIp(); });
}

void MMainWindow::notify(VPNConnection::DisconnectNotification const& n)
{
	Q_UNUSED(n);
	m_isConnected = false;
	qDebug() << "onDisconnected";
	m_dnsLeaks->enable(false);

	if(m_disconnectAction)
		m_disconnectAction->setEnabled(false);

	if (!m_bWasDisconnectClick && m_ui->connectWidget->connectingState() != FormConnect::CONNECT_BUTTON_OFF) // disconnect without user iteraction
	{
		QSettings settings;
		if (settings.value("killSwitch").toBool())
		{
			m_killSwitch->killInternet();
		}
	}

	m_ui->connectWidget->enableAll(true);
	m_ui->connectWidget->setConnectingState(FormConnect::CONNECT_BUTTON_OFF);
    QTimer::singleShot(1000, []{ IpHelper().requestIp(); });
}

void MMainWindow::notify(VPNConnection::ConnectErrorNotification const& n)
{
	qDebug() << "onConnectionError: " << n.message;
	if (!n.message.isEmpty())
    {
        restoreMainWindow();
		QMessageBox::information(this, QApplication::applicationName(), n.message);
    }

	m_ui->connectWidget->enableAll(true);
	m_ui->connectWidget->setConnectingState(FormConnect::CONNECT_BUTTON_OFF);
}

void MMainWindow::notify(ServerSide::LoginOkNotification const& n)
{
    Q_UNUSED(n);
    m_connectionImpl->getConfig();
    m_ui->loginWidget->writeSettings();
    const auto config = Config::ConfigRequester(*this).getConfig();
    m_serversModel->setServers(config.servers);
    m_ui->preferencesWidget->setPlan(config.plan);
    m_ui->preferencesWidget->setMessage(config.message);
    m_ui->connectWidget->fillServers(config.servers);
    m_ui->stackedWidget->setCurrentWidget(m_ui->account);
}

void MMainWindow::notify(ServerSide::LoginFailedNotification const& n)
{
    Q_UNUSED(n);
    m_ui->loginWidget->gotoFilling();
    m_ui->loginWidget->setErrorMessage(tr("Sorry, your login does't exist"));
}

void MMainWindow::doDisconnection()
{
    m_bWasDisconnectClick = true;
    m_connectionImpl->disconnect();
}

void MMainWindow::doConnection(QString const& dns, Config::Protocol const& protocol)
{
    //todo
//    if (!m_ovpnConfig.configReceived())
//    {
//        QMessageBox::information(this, QApplication::applicationName(), tr("Failed get configs from server. Please contact support"));
//        return;
//    }

//    if (!m_ovpnConfig.makeSuccess())
//    {
//        QMessageBox::information(this, QApplication::applicationName(), tr("Failed create OVPN file"));
//        return;
//    }

    m_lastStatTime = 0;
    m_bWasDisconnectClick = false;

    if (m_killSwitch->isActive())
        m_killSwitch->restoreInternet();

    QSettings settings;
    if (settings.value("killSwitch").toBool())
    {
        m_killSwitch->saveDefaultGateway();
    }

    m_connectionImpl->connect(protocol.protocol, dns, protocol.port);

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

        auto const conf = Config::ConfigRequester(*this).getConfig();
        doConnection(conf.servers[ind].dns, m_ui->preferencesWidget->currentProtocol());
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

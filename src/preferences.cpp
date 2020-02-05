#include "preferences.h"
#include "ui_preferences.h"

#include <QButtonGroup>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSignalBlocker>

#include "pubsub/Publisher.h"

#include "settingskeys.h"

namespace {

struct Publisher : PubSub::Publisher<Log::EnableWidgetRequest, Log::DisableWidgetRequest>
{
    Publisher() : PubSub::Agent("Publisher") { }

    void enableGuiLogging()
    { publish(Log::EnableWidgetRequest()); }

    void disableGuiLogging()
    { publish(Log::DisableWidgetRequest()); }
};

}

Preferences::Preferences(QWidget *parent)
	: PubSub::Agent("Preferences")
    , QWidget(parent)
    , m_ui(new Ui::Preferences)
{
    m_ui->setupUi(this);
	subscribeAll<Log::Object>();

    QButtonGroup* group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(m_ui->general);
    group->addButton(m_ui->connection);
    group->addButton(m_ui->blackList);
    group->addButton(m_ui->wifi);
    group->addButton(m_ui->account);

    m_ui->account->hide();
    m_ui->wifi->hide();
    m_ui->blackList->hide();

    ///@ TODO not now
    m_ui->autoUpdate->hide();
    m_ui->notification->hide();

    m_ui->unprotectedLabel->hide();
    m_ui->unprotectedWifi->hide();
    ///

    connect(m_ui->close, SIGNAL(clicked(bool)), this, SIGNAL(goBack()));
    connect(m_ui->general,    SIGNAL(clicked(bool)), SLOT(gotoGeneral()));
    connect(m_ui->connection, SIGNAL(clicked(bool)), SLOT(gotoConnection()));
    connect(m_ui->save, SIGNAL(clicked(bool)), this, SLOT(saveSettings()));
    connect(m_ui->guiLogging, &QCheckBox::toggled, this, [](bool checked)
    {
        Publisher p;
        checked ? p.enableGuiLogging() : p.disableGuiLogging();
    });

    QSettings settings;
    m_ui->launchOnStart       ->setChecked(settings.value(Keys::LaunchOnStartup, false).toBool());
    m_ui->unprotectedWifi     ->setChecked(settings.value(Keys::AutoConnect2UnprotectedWifi, false).toBool());
    m_ui->autoUpdate          ->setChecked(settings.value(Keys::AutoUpdate, false).toBool());
    m_ui->selectServerLocation->setChecked(settings.value(Keys::ConnectToRecent, true).toBool());

    m_protocols = QList<Config::Protocol>() << Config::Protocol(UDP, 1194) << Config::Protocol(TCP, 443) << Config::Protocol(TCP, 80);
    for(const auto& prot : m_protocols)
        m_ui->connectionMode->addItem(prot.toString());
    m_ui->connectionMode->setCurrentIndex(settings.value(Keys::CurrentProtocol, 0).toInt());
}

Preferences::~Preferences() = default;

void Preferences::showEvent(QShowEvent* event)
{
    style()->unpolish(m_ui->groupBox);
    style()->polish(m_ui->groupBox);

    style()->unpolish(m_ui->freeTrial);
    style()->polish(m_ui->freeTrial);

    style()->unpolish(m_ui->upgradeLabel);
    style()->polish(m_ui->upgradeLabel);

    QWidget::showEvent(event);
}

void Preferences::gotoGeneral()
{
    m_ui->stackedWidget->setCurrentWidget(m_ui->generalPage);
}

void Preferences::gotoConnection()
{
    m_ui->stackedWidget->setCurrentWidget(m_ui->connectionPage);
}

void Preferences::saveSettings()
{
    QSettings appSettings;
    ///@ Launch on start
    const bool launchOnStart = m_ui->launchOnStart->isChecked();
#ifdef Q_OS_WIN
    {
        const QString appName = QStringLiteral("OneVpn");
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                             QSettings::NativeFormat);
        if(launchOnStart)
            settings.setValue(appName, QCoreApplication::applicationFilePath().replace('/', '\\'));
        else
            settings.remove(appName);
    }
#endif
#ifdef Q_OS_MAC
    {
        const QString launchDaemon = QStringLiteral("com.aaa.ONEVpn.launcher.plist");
        const QString pathToDaemons = QString("%1/Library/LaunchAgents/").arg(QDir::homePath());
        QFile launcher(pathToDaemons + launchDaemon);
        if(launcher.exists())
            launcher.remove();
        if(launchOnStart)
        {
            QFile dummy(":/daemon/com.aaa.ONEVpn.launcher.plist");
            dummy.open(QIODevice::ReadOnly);
            QString dummyContent = dummy.readAll();

            launcher.open(QIODevice::WriteOnly);
            launcher.write(dummyContent.arg(qApp->applicationFilePath()).toLatin1());
        }
    }
#endif
    appSettings.setValue(Keys::LaunchOnStartup, launchOnStart);

    ///@ Auto connection with unprotected wi - fi
    appSettings.setValue(Keys::AutoConnect2UnprotectedWifi, m_ui->unprotectedWifi->isChecked());

    ///@ AUto update
    appSettings.setValue(Keys::AutoUpdate, m_ui->autoUpdate->isChecked());

    ///@ Connection to recent server or not
    appSettings.setValue(Keys::ConnectToRecent, m_ui->selectServerLocation->isChecked());

    ///@ Save last selected protocol
    appSettings.setValue(Keys::CurrentProtocol, m_ui->connectionMode->currentIndex());
    emit goBack();
}

Config::Protocol Preferences::currentProtocol() const
{
    return m_protocols.at(m_ui->connectionMode->currentIndex());
}

void Preferences::setCurrentServer(Config::Server const& server)
{
    m_server = server;
}

void Preferences::setPlan(Config::Plan const& plan)
{
    m_ui->daysTrial->setText(QString("%1 days").arg(plan.daysPaid));

    bool isTrial = plan.planName == "Trial";
    m_ui->groupBox->setProperty("styleHint", isTrial ? "trial" : "activated");
    m_ui->daysTrial->setVisible(isTrial);

    m_ui->freeTrial->setText(isTrial ? tr("Free trial:") : tr("PREMIUM ACCOUNT"));
    m_ui->freeTrial->setProperty("styleHint", isTrial ? "trial" : "activated");

    m_ui->upgradeLabel->setText(isTrial ? tr("UPGRADE") : plan.planExpired);
    m_ui->upgradeLabel->setProperty("styleHint", isTrial ? "trial" : "activated");
}

void Preferences::setMessage(Config::Message const& message)
{
    m_ui->versionLabel->setText(QString("Version: %1").arg(message.clientVersion));
}

void Preferences::notify(Log::DisabledWidgetRequest const&)
{
	QSignalBlocker blocker(m_ui->guiLogging);
	m_ui->guiLogging->setChecked(false);
}

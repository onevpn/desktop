#include "preferences.h"
#include "ui_preferences.h"

#include <QButtonGroup>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include "settingskeys.h"

Preferences::Preferences(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::Preferences)
{
    m_ui->setupUi(this);

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

    QSettings settings;
    m_ui->launchOnStart       ->setChecked(settings.value(Keys::LaunchOnStartup, false).toBool());
    m_ui->unprotectedWifi     ->setChecked(settings.value(Keys::AutoConnect2UnprotectedWifi, false).toBool());
    m_ui->autoUpdate          ->setChecked(settings.value(Keys::AutoUpdate, false).toBool());
    m_ui->selectServerLocation->setChecked(settings.value(Keys::ConnectToRecent, true).toBool());

    m_protocols = QList<TProtocol>() << TProtocol(UDP, 1194) << TProtocol(TCP, 443) << TProtocol(TCP, 80);
    Q_FOREACH(const TProtocol& prot, m_protocols)
        m_ui->connectionMode->addItem(prot.toString());
    m_ui->connectionMode->setCurrentIndex(settings.value(Keys::CurrentProtocol, 0).toInt());
}

Preferences::~Preferences()
{
}

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

TProtocol Preferences::currentProtocol() const
{
    return m_protocols.at(m_ui->connectionMode->currentIndex());
}

void Preferences::setCurrentServer(TServer const& server)
{
    m_server = server;
}

void Preferences::setPlan(TPlan const& plan)
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

void Preferences::setMessage(TMessage const& message)
{
    m_ui->versionLabel->setText(QString("Version: %1").arg(message.clientVersion));
}

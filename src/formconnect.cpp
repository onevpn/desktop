#include "formconnect.h"
#include "ui_formconnect.h"
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMovie>
#include <QUrl>
#include <QDesktopServices>
#include <QStandardItemModel>
#include <QMenu>
#include <QSettings>
#include <QFile>
#include <QVector>

#include <QSignalBlocker>

#include <algorithm>

#include "settingskeys.h"

FormConnect::FormConnect(QWidget *parent)
    : PubSub::Agent("FormConnect")
    , QWidget(parent)
    , ui(new Ui::FormConnect)
    , m_state(CONNECT_BUTTON_OFF)
    , m_accessManager(new QNetworkAccessManager(this))
{
    subscribeAll<ServerSide::Object>();
    ui->setupUi(this);

    QStyledItemDelegate* itemDelegate = new QStyledItemDelegate();
    ui->cbServer->setItemDelegate(itemDelegate);

    connect(ui->tbConnect,  SIGNAL(clicked(bool)), this, SIGNAL(connectionRequested(bool)));
    connect(ui->tbSettings, SIGNAL(clicked(bool)), this, SIGNAL(settingsRequested(bool)));
    connect(ui->tbSettings, SIGNAL(clicked(bool)), this, SLOT(onSettingsClicked()));

    connect(m_accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    connect(ui->paranoic, SIGNAL(toggled(bool)), SLOT(makekillswitch(bool)));
    connect(ui->paranoic, SIGNAL(stateChanged(int)), SLOT(repolish(int)));
    repolish(Qt::Unchecked);

    connect(ui->cbServer, SIGNAL(currentIndexChanged(int)), this, SIGNAL(serverChanged(int)));
    connect(ui->cbServer, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [](int index)
    {
        QSettings settings;
        settings.setValue(Keys::LastServer, index);
    });
}

void FormConnect::makekillswitch(bool value)
{
    QSettings settings;
    settings.setValue("killSwitch", value);
}

void FormConnect::repolish(int state)
{
    QWidget* w = state != Qt::Checked ? ui->onLabel : ui->offLabel;
    QWidget* ot = state == Qt::Checked ? ui->onLabel : ui->offLabel;
    w->setProperty("styleHint", "inState");
    ot->setProperty("styleHint", "");

    style()->unpolish(w);
    style()->polish(w);

    style()->unpolish(ot);
    style()->polish(ot);
}

FormConnect::~FormConnect()
{
    qDeleteAll(m_reply2item.keys());
}

void FormConnect::setConnectingState(int state)
{
    if (state == CONNECT_BUTTON_ON)
    {
        ui->tbConnect->stopAnimation();
        ui->tbConnect->setProperty("styleHint", "conn");
        style()->unpolish(ui->tbConnect);
        style()->polish(ui->tbConnect);

        ui->tbConnect->update();
    }
    else if (state == CONNECT_BUTTON_OFF)
    {
        ui->tbConnect->stopAnimation();
        ui->tbConnect->setProperty("styleHint", "");
        style()->unpolish(ui->tbConnect);
        style()->polish(ui->tbConnect);

        ui->tbConnect->update();
    }
    else if (state == CONNECT_BUTTON_CONNECTING)
    {
        ui->tbConnect->startAnimation();
    }
    else if (state == CONNECT_BUTTON_ERROR)
    {
    }
    m_state = state;
}

void FormConnect::enableAll(bool enable)
{
    ui->cbServer->setEnabled(enable);
    ui->tbSettings->setEnabled(enable);
//    ui->tbAccount->setEnabled(enable);
}

void FormConnect::onMovieChanged()
{
    ui->tbConnect->repaint();
}

void FormConnect::onMyAccount()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://onevpn.co/members/index.php")));
}

void FormConnect::fillServers(Config::ServerList const& servers)
{
    QStringList countries;

    QSignalBlocker blocker(ui->cbServer);
    ui->cbServer->clear();
    for(int i = 0; i < servers.size(); ++i)
    {
        auto &si = servers[i];
        ui->cbServer->addItem(si.name);

        QNetworkReply* reply = m_accessManager->get(QNetworkRequest(QUrl(si.flag)));
        m_reply2item.insert(reply, i);
    }

    QSettings settings;
    bool recent = settings.value(Keys::ConnectToRecent, true).toBool();
    if(!recent)
    {
        struct {
            bool operator() (Config::Server const& lhs, Config::Server const& rhs) const
            { return lhs.km < rhs.km; }
        } comparer;

        auto nearestIt = std::min_element(servers.begin(), servers.end(), comparer);
        if(nearestIt != servers.end())
            ui->cbServer->setCurrentIndex(std::distance(servers.begin(), nearestIt));
    }
    else
    {
        QSettings settings;
        int idx = settings.value(Keys::LastServer, 0).toInt();
        ui->cbServer->setCurrentIndex(idx);
    }
}

void FormConnect::setCurrentServerIndex(int idx)
{
    ui->cbServer->setCurrentIndex(idx);
}

void FormConnect::replyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    const int index = m_reply2item.value(reply);
    m_reply2item.remove(reply);
    if(reply->error() != QNetworkReply::NoError)
        return;

    QPixmap img;
    img.loadFromData(reply->readAll());
    if(!img.isNull())
        ui->cbServer->setItemIcon(index, img);
}

void FormConnect::setDownloaded(QString const& value)
{
    Q_UNUSED(value);
}

void FormConnect::setUploaded(QString const& value)
{
    Q_UNUSED(value);
}

int FormConnect::currentServer() const
{
    return ui->cbServer->currentIndex();
}

void FormConnect::onSettingsClicked()
{
    ui->tbSettings->setProperty("styleHint", "on");
    style()->unpolish(ui->tbSettings);
    style()->polish(ui->tbSettings);
    qApp->processEvents();

    QMenu *menu = new QMenu();
    menu->setProperty("styleHint", "preferencesMenu");
    QAction* prefAction = new QAction(tr("Preferences"), this);
    connect(prefAction, SIGNAL(triggered(bool)), this, SIGNAL(gotoPreferences()));

    QAction* accAction  = new QAction(tr("My Account"), this);
    QAction* logOutAction  = new QAction(tr("Sign Out"), this);
    QAction* windAction  = new QAction(tr("Quit"), this);
    menu->addAction(prefAction);
    menu->addAction(accAction);
    menu->addAction(logOutAction);
    menu->addAction(windAction);

    connect(accAction, SIGNAL(triggered(bool)), SLOT(getUrl()));
    connect(logOutAction, SIGNAL(triggered(bool)), this, SIGNAL(logoutRequested()));
    connect(windAction, SIGNAL(triggered(bool)), qApp, SLOT(quit()));

    QFile ss(":/style/QMenu.qss");
    ss.open(QIODevice::ReadOnly);
    menu->setStyleSheet(ss.readAll());

    QPoint globPos = mapToGlobal(geometry().topRight());

    menu->exec(globPos);
    menu->deleteLater();

    ui->tbSettings->setProperty("styleHint", "");
    style()->unpolish(ui->tbSettings);
    style()->polish(ui->tbSettings);
}

void FormConnect::getUrl()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://onevpn.co/members/index.php")));
}

void FormConnect::showEvent(QShowEvent* event)
{
    QSettings settings;
    ui->paranoic->setChecked(settings.value("killSwitch", false).toBool());
    QWidget::showEvent(event);
}

void FormConnect::notify(ServerSide::IpNotification const& n)
{
    ui->lblIP->setText(n.ip);
    m_countryCode = n.isoCode;
}

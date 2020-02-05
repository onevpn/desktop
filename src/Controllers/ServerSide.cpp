#include "ServerSide.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>
#include <QUrl>

#include "DAOs/configs.h"

namespace ServerSide {

struct Controller::ConfigManager
        : PubSub::Publisher<Config::ConfigNotification>
        , PubSub::Subscriber<Config::ConfigRequest>
{
    ConfigManager(PubSub::Agent const& a) : PubSub::Agent(a)
    {
        subscribeAll<Config::Object>();
    }

    void setConfig(Config::Config const& c) { m_config = c; }
    Config::Config config() const { return m_config; }

private:
    void notify(Config::ConfigRequest const&) override
    {
        publish(Config::ConfigNotification(nullptr, m_config));
    }

    Config::Config m_config;
};

Controller::Controller()
    : PubSub::Agent("ServerAPI::Controller")
    , m_manager(new QNetworkAccessManager(this))
    , m_configManager(new ConfigManager(*this))
{
    subscribeAll<ServerSide::Object>();
    connect(m_manager, &QNetworkAccessManager::finished, this, &Controller::replyFinished);
}

Controller::~Controller() = default;

void Controller::notify(IpRequest const& r)
{
    Q_UNUSED(r);
    QUrl url(QStringLiteral("http://onevpn.co/who"));

    QNetworkRequest request(url);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    conf.setProtocol(QSsl::TlsV1_0);
    request.setSslConfiguration(conf);

    auto reply = m_manager->get(request);
    m_reply2action.insert(reply, [this](QNetworkReply * reply)
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            publish(IpNotification(nullptr, "", ""));
            return;
        }

        QByteArray arr = reply->readAll();
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            publish(IpNotification(nullptr, "", ""));
            return;
        }

        const QJsonObject obj = doc.object();
        publish(IpNotification(nullptr, obj["ip"].toString(), obj["iso_code"].toString()));
    });
}

void Controller::replyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    assert(m_reply2action.contains(reply));
    if(auto action = m_reply2action.take(reply))
        action(reply);
}

void Controller::notify(LoginRequest const& r)
{
#define QS QStringLiteral
    const QString strRequest = QS("https://onevpn.co/members/mobile/api/");

    QUrl url(strRequest);
    QUrlQuery query;
    query.addQueryItem(QS("login"), r.username);
    QString passStr = r.password + QS("Fuck_The_Internet");
    query.addQueryItem(QS("pass"), QCryptographicHash::hash(passStr.toUtf8(), QCryptographicHash::Md5).toHex());
    query.addQueryItem(QS("mode"), QS("auth"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QS("application/x-www-form-urlencoded"));
    auto reply = m_manager->post(request, QByteArray());
    m_reply2action.insert(reply, [this](QNetworkReply* r) { parseConfig(r); });
#undef QS
}

void Controller::parseConfig(QNetworkReply* reply)
{
    if(reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
        publish(LoginFailedNotification(nullptr, "Failed connect to server"));
        return;
    }

    QByteArray arr = reply->readAll();
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject())
    {
        publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
        return;
    }
    QJsonObject jsonObject = doc.object();

    // --- message ---
    if (!jsonObject.contains("message"))
    {
        publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
        return;
    }
    QJsonObject jsonMessage =  jsonObject["message"].toObject();
    if (!jsonMessage.contains("value"))
    {
        publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
        return;
    }

    Config::Config config;
    config.message.value =  jsonMessage["value"].toString();
    config.message.clientVersion = jsonMessage["client_version"].toString();
    if (config.message.value.compare("Success", Qt::CaseInsensitive) != 0)
    {
        publish(LoginFailedNotification(nullptr, config.message.value));
        return;
    }
    config.message.suspended = jsonMessage["suspended"].toString();

    // --- auth ---
    if (!jsonObject.contains("auth"))
    {
        publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
        return;
    }
    QJsonObject jsonAuth =  jsonObject["auth"].toObject();
    config.auth.username = jsonAuth["login"].toString();
    config.auth.password = jsonAuth["password"].toString();

    qDebug() << " <<<<<<<<< " << config.auth.username << config.auth.password;

    // --- bandwidth ---
    if (!jsonObject.contains("bandwidth"))
    {
        publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
        return;
    }
    QJsonObject jsonBandwidth =  jsonObject["bandwidth"].toObject();
    config.bandwidth.in = jsonBandwidth["in"].toDouble();
    config.bandwidth.out = jsonBandwidth["out"].toDouble();
    config.bandwidth.total = jsonBandwidth["total"].toDouble();
    config.bandwidth.limit = jsonBandwidth["limit"].toDouble();

    // --- plan ---
    if (!jsonObject.contains("plan"))
    {
        publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
        return;
    }
    QJsonObject jsonPlan =  jsonObject["plan"].toObject();
    config.plan.planName = jsonPlan["plan_name"].toString();
    config.plan.planExpired = jsonPlan["plan_expired"].toString();
    config.plan.daysPaid = jsonPlan["days_paid"].toInt();

    // --- servers ---
    if (!jsonObject.contains("servers"))
    {
        publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
        return;
    }

    QJsonArray jsonServers =  jsonObject["servers"].toArray();
    Q_FOREACH(const QJsonValue &value, jsonServers)
    {
        QJsonObject server = value.toObject();
        Config::Server s;
        s.name    = server["Name"].toString();
        s.country = server["Country"].toString();
        s.dns     = server["DNS"].toString();
        s.flag    = server["Flag"].toString();
        s.km      = server["KM"].toInt();

        QJsonArray jsonProtocols =  server["protocol"].toArray();
        Q_FOREACH(const QJsonValue &protocol, jsonProtocols)
        {
            QString strProtocol = protocol.toString();
            QStringList strList = strProtocol.split(' ');
            if (strList.count() != 2)
            {
                publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
                return;
            }
            Config::Protocol proto;
            proto.protocol = strList[0] == "TCP" ? TCP : UDP;
            bool bOk;
            proto.port = strList[1].toInt(&bOk);
            if (!bOk)
            {
                publish(LoginFailedNotification(nullptr, "Incorrect answer from server. Please contact support."));
                return;
            }
            s.protocols << proto;
        }
        config.servers << s;
    }
    m_configManager->setConfig(config);
    publish(LoginOkNotification(nullptr));
}

}

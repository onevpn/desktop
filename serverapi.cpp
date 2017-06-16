#include "serverapi.h"
#include "utils.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>
#include <QNetworkRequest>
#include <QFile>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ServerAPI::ServerAPI(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
    connect(m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReplyFinished(QNetworkReply*)));
}

void ServerAPI::login(const QString &username, const QString &password)
{
    m_config.clear();
    m_username = username;
    m_password = password;

    const QString strRequest = QStringLiteral("https://onevpn.co/members/mobile/api/");

    QUrl url(strRequest);
    QUrlQuery query;
    query.addQueryItem("login", m_username);
    QString passStr = m_password + QStringLiteral("Fuck_The_Internet");
    query.addQueryItem("pass", QCryptographicHash::hash(passStr.toUtf8(), QCryptographicHash::Md5).toHex());
    query.addQueryItem("mode", "auth");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    m_manager->post(request, QByteArray());
}

void ServerAPI::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if(reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
        emit loginFailed("Failed connect to server");
        return;
    }

    QByteArray arr = reply->readAll();
//    qDebug() << " <<< " << Q_FUNC_INFO << arr;
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject())
    {
        emit loginFailed("Incorrect answer from server. Please contact support.");
        return;
    }
    QJsonObject jsonObject = doc.object();

    // --- message ---
    if (!jsonObject.contains("message"))
    {
        emit loginFailed("Incorrect answer from server. Please contact support.");
        return;
    }
    QJsonObject jsonMessage =  jsonObject["message"].toObject();
    if (!jsonMessage.contains("value"))
    {
        emit loginFailed("Incorrect answer from server. Please contact support.");
        return;
    }

    m_config.message.value =  jsonMessage["value"].toString();
    m_config.message.clientVersion = jsonMessage["client_version"].toString();
    if (m_config.message.value.compare("Success", Qt::CaseInsensitive) != 0)
    {
        emit loginFailed(m_config.message.value);
        return;
    }
    m_config.message.suspended = jsonMessage["suspended"].toString();

    // --- auth ---
    if (!jsonObject.contains("auth"))
    {
        emit loginFailed("Incorrect answer from server. Please contact support.");
        return;
    }
    QJsonObject jsonAuth =  jsonObject["auth"].toObject();
    m_config.auth.username = jsonAuth["login"].toString();
    m_config.auth.password = jsonAuth["password"].toString();

    // --- bandwidth ---
    if (!jsonObject.contains("bandwidth"))
    {
        emit loginFailed("Incorrect answer from server. Please contact support.");
        return;
    }
    QJsonObject jsonBandwidth =  jsonObject["bandwidth"].toObject();
    m_config.bandwidth.in = jsonBandwidth["in"].toDouble();
    m_config.bandwidth.out = jsonBandwidth["out"].toDouble();
    m_config.bandwidth.total = jsonBandwidth["total"].toDouble();
    m_config.bandwidth.limit = jsonBandwidth["limit"].toDouble();

    // --- plan ---
    if (!jsonObject.contains("plan"))
    {
        emit loginFailed("Incorrect answer from server. Please contact support.");
        return;
    }
    QJsonObject jsonPlan =  jsonObject["plan"].toObject();
    m_config.plan.planName = jsonPlan["plan_name"].toString();
    m_config.plan.planExpired = jsonPlan["plan_expired"].toString();
    m_config.plan.daysPaid = jsonPlan["days_paid"].toInt();

    // --- servers ---
    if (!jsonObject.contains("servers"))
    {
        emit loginFailed("Incorrect answer from server. Please contact support.");
        return;
    }

    QJsonArray jsonServers =  jsonObject["servers"].toArray();
    Q_FOREACH(const QJsonValue &value, jsonServers)
    {
        QJsonObject server = value.toObject();

        TServer s;
        s.name = server["Name"].toString();
        s.country = server["Country"].toString();
        s.dns = server["DNS"].toString();
        s.flag = server["Flag"].toString();
        s.km = server["KM"].toInt();

        QJsonArray jsonProtocols =  server["protocol"].toArray();
        Q_FOREACH(const QJsonValue &protocol, jsonProtocols)
        {
            QString strProtocol = protocol.toString();
            QStringList strList = strProtocol.split(' ');
            if (strList.count() != 2)
            {
                emit loginFailed("Incorrect answer from server. Please contact support.");
                return;
            }
            TProtocol proto;
            proto.protocol = strList[0] == "TCP" ? TCP : UDP;
            bool bOk;
            proto.port = strList[1].toInt(&bOk);
            if (!bOk)
            {
                emit loginFailed("Incorrect answer from server. Please contact support.");
                return;
            }
            s.protocols << proto;
        }

        m_config.servers << s;
    }
    emit loginOk();
}


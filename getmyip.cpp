#include "getmyip.h"
#include <QThread>
#include <QtNetwork>
#include <QDebug>

GetMyIp::GetMyIp(QObject *parent) :
    QThread(parent)
{

}

GetMyIp::~GetMyIp()
{
    terminate();
    wait();
}

void GetMyIp::run()
{
    ///@TODO burn with fire утечки и прочие радости жизни!!
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QEventLoop eventLoop;
    eventLoop.connect(manager, SIGNAL(finished(QNetworkReply *)), SLOT(quit()));

    const QString strRequest = QStringLiteral("http://onevpn.co/who");

    QUrl url(strRequest);
    QNetworkRequest request(url);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    conf.setProtocol(QSsl::TlsV1_0);
    request.setSslConfiguration(conf);

    QNetworkReply *reply = manager->get(request);
    eventLoop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        emit myIpFinished("", "");
        return;
    }

    QByteArray arr = reply->readAll();
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject())
    {
        emit myIpFinished("", "");
        return;
    }

    QJsonObject obj = doc.object();
    emit myIpFinished(obj["ip"].toString(), obj["iso_code"].toString());
}

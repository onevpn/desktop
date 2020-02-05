#include "ConfigManager.h"

#include <QtNetwork>
#include <QDebug>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_bSuccess(false)
    , m_manager(new QNetworkAccessManager(this))
{
    connect(m_manager, SIGNAL(finished(QNetworkReply*)), SLOT(onReplyFinished(QNetworkReply*)));
}

void ConfigManager::get(const QString &username, const QString &password)
{
    m_username = username;
    m_password = password;
    m_bSuccess = false;

    const QString strRequest = QStringLiteral("https://onevpn.co/members/mobile/api/");
    QUrl url(strRequest);
    QUrlQuery query;
    query.addQueryItem("login", m_username);
    const QString passStr = m_password + QStringLiteral("Fuck_The_Internet");
    query.addQueryItem("pass", QCryptographicHash::hash(passStr.toStdString().c_str(), QCryptographicHash::Md5).toHex());
    query.addQueryItem("mode", "getconfig");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    m_manager->post(request, QByteArray());
}

bool ConfigManager::generate(const QString &server, PROTOCOL_OPENVPN protocol, int port)
{
    m_bMakeSuccess = false;

    if (m_tempFile.open())
    {
#ifdef Q_OS_LINUX
        m_data.replace("persist-tun", "");
#endif

        m_tempFile.resize(0);
        m_tempFile.write(m_data);

        if (protocol == TCP)
        {
            m_tempFile.write("proto tcp\r\n");
        }
        else if (protocol == UDP)
        {
            m_tempFile.write("proto udp\r\n");
        }

        QString ip = server;
#ifdef Q_OS_LINUX
        char command[1024];
        sprintf(command, "nslookup %s | tail -2 | awk -F \":\" '{print $2}'", server.toUtf8().data());
        FILE * fp = popen(command, "r");
        char buffer[1024];
        char* output = fgets(buffer, sizeof(buffer), fp);
        pclose(fp);
        ip = QString(output).trimmed();
        qDebug() << " << ip = " << ip;
        QSettings settings;
        settings.setValue("SERVER_IP", ip);
#endif
        QString str = QStringLiteral("remote ") + ip + QStringLiteral(" ") + QString::number(port) + QStringLiteral("\r\n");
        str += "auth-user-pass\r\n";
        m_tempFile.write(str.toLocal8Bit());

#ifdef Q_OS_MAC
        m_tempFile.write("script-security 2\r\n");

        m_tempFile.write("push \"dhcp-option DNS 8.8.8.8\"\r\n");
        m_tempFile.write("push \"dhcp-option DNS 8.8.4.4\"\r\n");

        const QString upScript = qApp->applicationDirPath() + "/../Resources/client.up.sh";
        const QString downScript = qApp->applicationDirPath() + "/../Resources/client.down.sh";
        m_tempFile.write(QString("up %1\r\n").arg(upScript).toUtf8());
        m_tempFile.write(QString("down %1\r\n").arg(downScript).toUtf8());
#endif

#ifdef Q_OS_LINUX
        m_tempFile.write("script-security 2\r\n");
        m_tempFile.write("up /etc/openvpn/update-resolv-conf\r\n");
        m_tempFile.write("down /etc/openvpn/update-resolv-conf\r\n");
#endif
        qDebug() << " <<<< Temp filename = " << m_tempFile.fileName();

        m_tempFile.close();
        m_bMakeSuccess = true;
    }
    m_path = m_tempFile.fileName();
    return m_bMakeSuccess;
}

void ConfigManager::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
        return;
    m_data = reply->readAll();
    m_bSuccess = true;
}

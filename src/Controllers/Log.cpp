#include "Log.h"

#include <iostream>

#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include "pubsub/Publisher.h"
#include "DAOs/VPNConnection.h"
#include "LogBrowserDialog.h"

namespace Log {

namespace {

void customLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    struct Publisher : PubSub::Publisher<LogNotification> { Publisher() : PubSub::Agent("PublisherLocal") {} } pub;
    pub.publish(LogNotification(type, context, msg));
}

QString generateLogFileName(QString const& name)
{
    QStringList locations =  QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
	auto location = QString("%1/%2.log").arg(locations.front(), name);
    return QString("%1/%2.log").arg(locations.front(), name);
}

}

struct Controller::ConnectionLogger
        : QObject
        , PubSub::Subscriber<VPNConnection::ConnectRequest, VPNConnection::DisconnectRequest, VPNConnection::ConnectErrorNotification>
{
    ConnectionLogger() : PubSub::Agent("Controller::ConnectionLogger")
    {
        subscribeAll<VPNConnection::Object>();
        connect(&m_manager, &QNetworkAccessManager::finished, this, &ConnectionLogger::replyFinished);
    }

    void append(QString const& txt)
    {
        if(!m_logging)
            return;
        m_log.append("\n");
        m_log.append(txt);
    }

private:
    void notify(VPNConnection::ConnectRequest const& n) override
    {
        m_logging = true;
        m_username = n.username;
    }

    void notify(VPNConnection::DisconnectRequest const&) override
    {
        m_logging = false;
        m_log.clear();
    }

    void notify(VPNConnection::ConnectErrorNotification const&) override
    {
        m_logging = false;

        const QString strRequest = QStringLiteral("https://onevpn.co/members/mobile/api/");
        QUrl url(strRequest);
        QUrlQuery query;

        query.addQueryItem("userid", m_username);
        query.addQueryItem("post_data", m_log);
        url.setQuery(query);

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        m_manager.post(request, QByteArray());

        m_log.clear();
    }

    void replyFinished(QNetworkReply* r)
    {
        r->deleteLater();
        if(r->error() != QNetworkReply::NoError)
        {
            std::cout << " <<< Something is gonna wrong " << r->errorString().toStdString() << std::endl;
            return;
        }
        std::cout << " <<< Send logs just fine " << std::endl;
    }

private:
    QString m_log;
    bool m_logging = false;
    QNetworkAccessManager m_manager;
    QString m_username;
};

Controller::Controller()
    : PubSub::Agent("Log::Controller")
    , m_currSessionId(QUuid::createUuid().toString() + QDateTime::currentDateTime().toString(Qt::ISODate).replace(":", "-"))
    , m_connectionLogger(new ConnectionLogger)
{
    subscribeAll<Log::Object>();
    qInstallMessageHandler(customLogger);
}

Controller::~Controller() = default;

void Controller::notify(LogNotification const& n)
{
    struct ScopeExitter
    {
        ScopeExitter () { m_prev = qInstallMessageHandler(0); }
        ~ScopeExitter() { qInstallMessageHandler(m_prev); }
        QtMessageHandler m_prev;
    } scopeGuard;

    QByteArray localMsg = n.msg.toLocal8Bit();
    QString txt;
    switch (n.type)
    {
        case QtDebugMsg:
            txt = QString("Debug: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(n.context.file).arg(n.context.line).arg(n.context.function);
            break;
        case QtInfoMsg:
            txt = QString("Info: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(n.context.file).arg(n.context.line).arg(n.context.function);
            break;
        case QtWarningMsg:
            txt = QString("Warning: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(n.context.file).arg(n.context.line).arg(n.context.function);
            break;
        case QtCriticalMsg:
            txt = QString("Critical: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(n.context.file).arg(n.context.line).arg(n.context.function);
            break;
        case QtFatalMsg:
            txt = QString("Fatal: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(n.context.file).arg(n.context.line).arg(n.context.function);
            break;
    }

    if(m_dialog) m_dialog->outputMessage(txt);
    m_connectionLogger->append(txt);

    QFileInfo fi(generateLogFileName(m_currSessionId));
    QDir().mkpath(fi.absoluteDir().absolutePath());
    QFile outFile(fi.absoluteFilePath());
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;

    std::cout << txt.toStdString() << std::endl;
}

void Controller::notify(EnableWidgetRequest const& r)
{
    Q_UNUSED(r)
    m_dialog = new LogBrowserDialog;
	m_dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(m_dialog.data(), &QObject::destroyed, this, []{
		struct Publisher : PubSub::Publisher<DisabledWidgetRequest> { Publisher() : PubSub::Agent("Publisher") {} } pub;
		pub.publish(DisabledWidgetRequest());
	});
    m_dialog->show();
}

void Controller::notify(DisableWidgetRequest const& r)
{
    Q_UNUSED(r)
	m_dialog->deleteLater();
}

}

#ifndef SERVERAPI_H
#define SERVERAPI_H

#include <QObject>
#include <QVector>
#include "utils.h"

struct TAuth
{
    QString username;
    QString password;
};

struct TMessage
{
    QString value;
    QString clientVersion;
    QString suspended;
};

struct TBandwidth
{
    double in;
    double out;
    double total;
    double limit;

    QString sizeInStr() const
    {
        qint64 size = limit - total;
        return Utils::sizeInStr(size);
    }
};

struct TPlan
{
    QString planName;
    QString planExpired;
    int     daysPaid;
};

struct TProtocol
{
    PROTOCOL_OPENVPN protocol;
    int port;

    TProtocol() {}

    TProtocol(PROTOCOL_OPENVPN prot, int por) : protocol(prot), port(por)
    { }

    QString toString() const
    {
        QString ret;
        if (protocol == TCP)
            ret += "TCP ";
        else if (protocol == UDP)
            ret += "UDP ";
        ret += QString::number(port);
        return ret;
    }
};

struct TServer
{
    QString name;
    int km;
    QString country;
    QString dns;
    QString flag;
    QVector<TProtocol> protocols;
};

struct TConfig
{
    TAuth auth;
    TMessage message;
    TBandwidth bandwidth;
    TPlan plan;
    QVector<TServer> servers;

    TConfig()
    {
        clear();
    }

    void clear()
    {
        auth.username.clear();
        auth.password.clear();
        message.clientVersion.clear();
        message.suspended.clear();
        message.value.clear();
        bandwidth.in = 0.0;
        bandwidth.out = 0.0;
        bandwidth.limit = 0.0;
        bandwidth.total = 0.0;
        plan.daysPaid = 0;
        plan.planExpired.clear();
        plan.planName.clear();
        servers.clear();
    }
};

class QNetworkReply;
class QNetworkAccessManager;

class ServerAPI : public QObject
{
    Q_OBJECT
public:
    explicit ServerAPI(QObject *parent = Q_NULLPTR);
    void login(const QString &username, const QString &password);
    TConfig const &config() const { return m_config; }

    QString username() const { return m_username; }
    QString password() const { return m_password; }

signals:
    void loginOk();
    void loginFailed(const QString &msg);

private slots:
    void onReplyFinished(QNetworkReply*);

private:
    QString m_username;
    QString m_password;
    TConfig m_config;

    QNetworkAccessManager* m_manager;
};

#endif // SERVERAPI_H

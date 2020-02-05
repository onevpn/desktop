#pragma once

#include <QString>
#include <QList>

#include "utils.h"
#include "Types.h"
#include "pubsub/Subscriber.h"
#include "pubsub/Publisher.h"

namespace Config {

struct Auth
{
    QString username;
    QString password;
};

struct Message
{
    QString value;
    QString clientVersion;
    QString suspended;
};

struct Bandwidth
{
    double in = 0;
    double out = 0;
    double total = 0;
    double limit = 0;

    QString sizeInStr() const
    {
        qint64 size = limit - total;
        return Utils::sizeInStr(size);
    }
};

struct Plan
{
    QString planName;
    QString planExpired;
    int     daysPaid = 0;
};

struct Protocol
{
    PROTOCOL_OPENVPN protocol;
    int port;

    Protocol() {}

    Protocol(PROTOCOL_OPENVPN prot, int por) : protocol(prot), port(por)
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

struct Server
{
    QString name;
    int km = 0;
    QString country;
    QString dns;
    QString flag;
    QList<Protocol> protocols;
};

using ServerList = QList<Server>;

struct Config
{
    Auth auth;
    Message message;
    Bandwidth bandwidth;
    Plan plan;
    ServerList servers;

    void clear()
    {
        this->~Config();
        new (this) Config();
    }
};

using ::Message::Types::Notification;
using ::Message::Types::Request;

struct Object : PubSub::Object
{
};

struct ConfigRequest : Request<Object>
{
    ConfigRequest(Object const* o) : Request<Object>(o) { }
};

struct ConfigNotification : Notification<Object>
{
    ConfigNotification(Object const* o, Config const& c)
        : Notification<Object>(o)
        , config(c)
    { }

    Config const& config;
};

class ConfigRequester
        : PubSub::Publisher<ConfigRequest>
        , PubSub::Subscriber<ConfigNotification>
{
public:
    ConfigRequester(PubSub::Agent const& a) : PubSub::Agent(a)
    {
        subscribeAll<Object>();
        publish(ConfigRequest(nullptr));
    }

    Config getConfig() const { return m_config; }

private:
    void notify(ConfigNotification const& n) override
    {
        m_config = n.config;
    }

    Config m_config;
};

}

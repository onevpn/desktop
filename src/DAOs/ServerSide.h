#pragma once

#include <QString>
#include "Types.h"

namespace ServerSide {

using Message::Types::Notification;
using Message::Types::Request;

struct Object : PubSub::Object
{
};

struct IpRequest : Request<Object>
{
    IpRequest(Object const* o) : Request<Object>(o)
    { }
};

struct IpNotification : Notification<Object>
{
    IpNotification(Object const* o, QString const& ip, QString const& isoCode)
        : Notification<Object>(o)
        , ip(ip)
        , isoCode(isoCode)
    { }

    QString const& ip;
    QString const& isoCode;
};

struct LoginRequest : Request<Object>
{
    LoginRequest(Object const* o, QString const& username, QString const& password)
        : Request<Object>(o)
        , username(username)
        , password(password)
    { }

    QString const& username;
    QString const& password;
};

struct LoginFailedNotification : Request<Object>
{
    LoginFailedNotification(Object const* o, QString const& message)
        : Request<Object>(o)
        , message(message)
    { }

    QString const& message;
};

struct LoginOkNotification : Request<Object>
{
    LoginOkNotification(Object const* o)
        : Request<Object>(o)
    { }
};

}

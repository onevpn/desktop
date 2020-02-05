#pragma once

#include <QString>

#include "Types.h"
#include "utils.h"

namespace VPNConnection {

using Message::Types::Request;
using Message::Types::Notification;

struct Object : PubSub::Object
{
};

struct ConnectRequest : Request<Object>
{
	ConnectRequest(Object const* o
                   , PROTOCOL_OPENVPN protocol
				   , QString const& serverIP
				   , QString const& username
				   , QString const& password
                   , int port)
		: Request<Object>(o)
 		, protocol(protocol)
		, serverIP(serverIP)
		, username(username)
		, password(password)
        , port(port)
	{ }

    PROTOCOL_OPENVPN protocol;
	QString serverIP;
	QString username;
	QString password;
    int port;
};

struct ConnectNotification : Notification<Object>
{
	ConnectNotification(Object const* o) : Notification<Object>(o) { }
};

struct ConnectErrorNotification : Notification<Object>
{
	ConnectErrorNotification(Object const* o, QString const& message)
		: Notification<Object>(o)
		, message(message)
	{ }

	QString const& message;
};

struct DisconnectRequest : Request<Object>
{
	DisconnectRequest(Object const* o) : Request<Object>(o) { }
};

struct DisconnectNotification : Notification<Object>
{
	DisconnectNotification(Object const* o) : Notification<Object>(o) { }
};

struct GettingConfigRequest : Request<Object>
{
    GettingConfigRequest(Object const* o, QString const & username, QString const & password)
        : Request<Object>(o)
        , username(username)
        , password(password)
    { }

    QString const & username;
    QString const & password;
};

struct GettingConfigFailedNotification : Notification<Object>
{
    GettingConfigFailedNotification(Object const* o)
        : Notification<Object>(o)
    { }
};

struct GenerateConfigRequest : Request<Object>
{
    GenerateConfigRequest(Object const* o, QString const& server, PROTOCOL_OPENVPN protocol, int port)
        : Request<Object>(o)
        , server(server)
        , protocol(protocol)
        , port(port)
    { }

    QString const& server;
    PROTOCOL_OPENVPN protocol;
    int port;
};

struct GeneratedConfigNotification : Notification<Object>
{
    GeneratedConfigNotification(Object const* o)
        : Notification<Object>(o)
    { }
};

}

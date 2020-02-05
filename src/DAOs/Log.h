#pragma once

#include <QDebug>
#include "Types.h"

namespace Log {

using Message::Types::Notification;
using Message::Types::Request;

struct Object : PubSub::Object
{
};

struct LogNotification : Notification<Object>
{
    LogNotification(QtMsgType type, const QMessageLogContext &context, const QString &msg)
        : Notification<Object>(nullptr)
        , type(type)
        , context(context)
        , msg(msg)
    { }

    QtMsgType type;
    const QMessageLogContext &context;
    const QString &msg;
};

struct EnableWidgetRequest : Request<Object>
{
    EnableWidgetRequest() : Request<Object>(nullptr) { }
};

struct DisableWidgetRequest : Request<Object>
{
    DisableWidgetRequest() : Request<Object>(nullptr) { }
};

struct DisabledWidgetRequest : Request<Object>
{
	DisabledWidgetRequest() : Request<Object>(nullptr) { }
};

}

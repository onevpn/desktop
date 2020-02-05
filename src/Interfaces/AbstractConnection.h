#pragma once

#include <QObject>
#include "utils.h"

class AbstractConnection : public QObject
{
	Q_OBJECT

public:
    using QObject::QObject;

    virtual bool initialize() = 0;
    virtual  bool connect(PROTOCOL protocol
						  , QString serverIP
						  , QString username
						  , QString password
						  , QString ovpnFile
                          , QString l2tpKey) = 0;
    virtual void disconnect  () = 0;
    virtual bool tapInstalled() = 0;
    virtual bool isConnected () const = 0;

signals:
    void connected();
    void disconnected();
    void error(const QString &error);
    void log(const QString &logStr);
    void statisticsChanged(quint64 download, quint64 upload);
};

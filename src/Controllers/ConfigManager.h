#pragma once

#include <QObject>
#include <QTemporaryFile>
#include "utils.h"

class QNetworkAccessManager;
class QNetworkReply;

class ConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManager(QObject *parent = Q_NULLPTR);

    void get(const QString &username, const QString &password);

    bool generate(const QString &server, PROTOCOL_OPENVPN protocol, int port);
    bool configReceived() const { return m_bSuccess; }
    bool makeSuccess()    const { return m_bMakeSuccess; }
    QString path()        const { return m_path; }

private slots:
    void onReplyFinished(QNetworkReply*);

private:
    QString m_username;
    QString m_password;

    QByteArray m_data;
    bool m_bSuccess;

    QTemporaryFile m_tempFile;
    QString m_path;
    bool m_bMakeSuccess;

    QNetworkAccessManager* m_manager;
};

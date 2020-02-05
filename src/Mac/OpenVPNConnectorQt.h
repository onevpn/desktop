
#ifndef OPENVPNCONNECTORQT_H
#define OPENVPNCONNECTORQT_H

#include <QObject>

enum OPENVPN_ERROR {AUTH_ERROR, NO_OPENVPN_SOCKET, CANNOT_ALLOCATE_TUN_TAP,
                    CANNOT_CONNECT_TO_SERVICE_PIPE, CANNOT_WRITE_TO_SERVICE_PIPE,
                    NO_AVAILABLE_PORT, PROXY_AUTH_ERROR};

class OpenVPNConnectorQt : public QObject
{
    Q_OBJECT

public:
    explicit OpenVPNConnectorQt(QObject *parent = 0);
    virtual ~OpenVPNConnectorQt();

    bool installHelper(const QString &label);
    void connect(const QString &configPath, const QString &username, const QString &password, const QString &proxyUsername, const QString &proxyPassword);
    void disconnect();

    bool executeRootCommand(const QString commandLine, quint32 *exitCode);

    void emitConnected();
    void emitDisconnected();
    void emitStateChanged(const QString &state);
    void emitError(OPENVPN_ERROR err);
    void emitLog(const QString &logStr);
    void emitStats(quint64 bytesIn, quint64 bytesOut);

signals:
    void connected();
    void disconnected();
    void stateChanged(const QString &state);
    void error(OPENVPN_ERROR err);
    void log(const QString &logStr);

    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut);
};

Q_DECLARE_METATYPE(OPENVPN_ERROR);

#endif // OPENVPNCONNECTORQT_H

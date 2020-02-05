#ifndef OPENVPNCONNECTORQT_H
#define OPENVPNCONNECTORQT_H

#include <QThread>

enum OPENVPN_ERROR {AUTH_ERROR, NO_OPENVPN_SOCKET, CANNOT_ALLOCATE_TUN_TAP,
                    CANNOT_CONNECT_TO_SERVICE_PIPE, CANNOT_WRITE_TO_SERVICE_PIPE,
                    NO_AVAILABLE_PORT, PROXY_AUTH_ERROR};

//include openvpn connector for Windows
class OpenVPNConnectorQt : public QThread
{
    Q_OBJECT
public:
    explicit OpenVPNConnectorQt(QObject *parent = 0);
    virtual ~OpenVPNConnectorQt();

    bool installHelper(const QString &label);
    void connect(const QString &configPath, const QString &username, const QString &password,
                 const QString &proxyUsername, const QString &proxyPassword);
    void disconnect();

    bool executeRootCommand(const QString &commandLine, quint32 *exitCode);

signals:
    void connected();
    void disconnected();
    void stateChanged(const QString &state);
    void error(OPENVPN_ERROR err);
    void log(const QString &logStr);

    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut);

protected:
    virtual void run();

private:
    enum CONNECTION_STATUS {STATUS_DISCONNECTED, STATUS_CONNECTING,  STATUS_CONNECTED};

    CONNECTION_STATUS currentState_;
    QString configPath_;
    QString username_;
    QString password_;
    QString proxyUsername_;
    QString proxyPassword_;

    bool bStopThread_;
    bool bSockConnected_;
    int openVpnPort_;
    bool connectToPipeAndRunOpenVPN();

    int getAvailablePort();
};

Q_DECLARE_METATYPE(OPENVPN_ERROR);

#endif // OPENVPNCONNECTORQT_H

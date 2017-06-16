#include "openvpnconnectorqt.h"
#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QProcess>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <QSettings>
#include <QTemporaryFile>

int socketOpenVPN = -1;

int recvAll(int socket, char *buf, int size)
{
    int bytes_received = 0;

    while (bytes_received != size)
    {
        int r = read(socket, buf + bytes_received, size - bytes_received);
        if (r > 0)
        {
            bytes_received += r;
        }
        else
        {
            return -1;
        }
    }
    return bytes_received;
}

int sendAll(int socket, char *buf, int size)
{
    int bytes_transfered = 0;
    while (bytes_transfered < size)
    {
        int r = write(socket, buf + bytes_transfered, size - bytes_transfered);
        if (r < 0)
        {
            return -1;
        }
        else
        {
            bytes_transfered += r;
        }
    }

    return bytes_transfered;
}

int sgetline(int sock, char *outbuf)
{
    int bytesloaded = 0;
    int ret;
    char buf;

    do
    {
        // read a single byte
        ret = read(sock, &buf, 1);
        if (ret < 1)
        {
            // error or disconnect
            return -1;
        }

        outbuf[bytesloaded] = buf;
        bytesloaded++;

        // has end of line been reached?
        if (buf == '\n')
            break; // yes

    } while (1);

    outbuf[bytesloaded - 1] = '\0';
    return bytesloaded; // number of bytes in the line, not counting the line break
}

OpenVPNConnectorQt::OpenVPNConnectorQt(QObject *parent) : QThread(parent), bSockConnected_(false),
    currentState_(STATUS_DISCONNECTED)
{
}

OpenVPNConnectorQt::~OpenVPNConnectorQt()
{
    wait();

    if (socketOpenVPN != -1)
    {
        close(socketOpenVPN);
        socketOpenVPN = -1;
    }
}

bool OpenVPNConnectorQt::installHelper(const QString &label)
{
    return true;
}

void OpenVPNConnectorQt::connect(const QString &configPath, const QString &username, const QString &password,
                                 const QString &proxyUsername, const QString &proxyPassword)
{
    configPath_ = configPath;
    username_ = username;
    password_ = password;
    proxyUsername_ = proxyUsername;
    proxyPassword_ = proxyPassword;
    bSockConnected_ = false;
    bStopThread_ = false;

    if (socketOpenVPN != -1)
    {
        close(socketOpenVPN);
        socketOpenVPN = -1;
    }
    currentState_ = STATUS_CONNECTING;

    start(QThread::LowPriority);
}

void OpenVPNConnectorQt::disconnect()
{
    if (socketOpenVPN != -1 && bSockConnected_)
    {
        char *message = "signal SIGTERM\n";
        sendAll(socketOpenVPN, message, strlen(message));
    }
}

bool OpenVPNConnectorQt::executeRootCommand(const QString &commandLine, quint32 *exitCode)
{
    return false;
}

void OpenVPNConnectorQt::run()
{
    openVpnPort_ = getAvailablePort();
    if (openVpnPort_ == -1)
    {
        emit error(NO_AVAILABLE_PORT);
        return;
    }

    if (!runOpenVPN())
        return;

    msleep(300);

    int numberOfConnectRetries = 0;
    char server_reply[10000];
    bool bStateModeOn = false;
    bool bProxyAuthErrorEmited = false;
    bool bTapErrorEmited = false;
    bSockConnected_ = false;

    while (!bStopThread_)
    {
        if (!bSockConnected_)
        {
            socketOpenVPN = socket(AF_INET, SOCK_STREAM, 0);
            if (socketOpenVPN < 0)
            {
                qDebug() << "socket failed with error";
                return ;
            }
            struct hostent *server = gethostbyname("127.0.0.1");
            if (server == NULL)
            {
                qDebug() << "gethostbyname failed, no such host";
                return;
            }            struct sockaddr_in serv_addr;
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            bcopy((char *)server->h_addr,
                  (char *)&serv_addr.sin_addr.s_addr,
                  server->h_length);
            serv_addr.sin_port = htons(openVpnPort_);

            if (::connect(socketOpenVPN,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
            {
                close(socketOpenVPN);
                numberOfConnectRetries++;

                if (numberOfConnectRetries > 10)
                {
                    qDebug() << "Can't connect to openvpn socket after 10 retries";
                    currentState_ = STATUS_DISCONNECTED;
                    emit error(NO_OPENVPN_SOCKET);
                    return;
                }
                msleep(300);
            }
            else
            {
                bSockConnected_ = true;
            }
        }
        if (bSockConnected_)
        {
            if (!bStateModeOn)
            {
                char *message = "state on all\n";
                sendAll(socketOpenVPN, message, strlen(message));

                char *message2 = "log on\n";
                sendAll(socketOpenVPN, message2, strlen(message2));

                char *message3 = "bytecount 1\n";
                sendAll(socketOpenVPN, message3, strlen(message3));

                bStateModeOn = true;
            }

            //Receive a reply line from the server
            int ret = sgetline(socketOpenVPN, server_reply);
            if (ret < 0)
            {
                break;
            }

            QString strServerReply = QString::fromUtf8(server_reply);
            qDebug() << strServerReply;

            if (strServerReply.contains("PASSWORD:Need 'Auth' username/password", Qt::CaseInsensitive))
            {
                char message[1024];
                sprintf(message, "username \"Auth\" %s\n", username_.toUtf8().data());
                qDebug() << "<<<<<< Auth username: " << message;
                sendAll(socketOpenVPN, message, strlen(message));
            }
            else if (strServerReply.contains("'Auth' username entered, but not yet verified", Qt::CaseInsensitive))
            {
                char message[1024];
                sprintf(message, "password \"Auth\" %s\n", password_.toUtf8().data());
                qDebug() << "<<<<<< Auth password: " << message;
                sendAll(socketOpenVPN, message, strlen(message));
            }
            else if (strServerReply.contains("PASSWORD:Need 'HTTP Proxy' username/password", Qt::CaseInsensitive))
            {
                char message[1024];
                sprintf(message, "username \"HTTP Proxy\" %s\n", proxyUsername_.toUtf8().data());
                sendAll(socketOpenVPN, message, strlen(message));
            }
            else if (strServerReply.contains("'HTTP Proxy' username entered, but not yet verified", Qt::CaseInsensitive))
            {
                char message[1024];
                sprintf(message, "password \"HTTP Proxy\" %s\n", proxyPassword_.toUtf8().data());
                sendAll(socketOpenVPN, message, strlen(message));
            }
            else if (strServerReply.contains("PASSWORD:Verification Failed: 'Auth'", Qt::CaseInsensitive))
            {
                emit error(AUTH_ERROR);
            }
            else if (strServerReply.contains("There are no TAP-Windows adapters on this system", Qt::CaseInsensitive))
            {
                if (!bTapErrorEmited)
                {
                    emit error(CANNOT_ALLOCATE_TUN_TAP);
                    bTapErrorEmited = true;
                }
            }
            else if (strServerReply.contains("Proxy requires authentication", Qt::CaseInsensitive))
            {
                if (!bProxyAuthErrorEmited)
                {
                    emit error(PROXY_AUTH_ERROR);
                    bProxyAuthErrorEmited = true;
                }
            }
            else if (strServerReply.contains(">BYTECOUNT:", Qt::CaseInsensitive))
            {
                QStringList pars = strServerReply.split(":");
                if (pars.count() > 1)
                {
                    QStringList pars2 = pars[1].split(",");
                    if (pars2.count() == 2)
                    {
                        quint64 l1 = pars2[0].toULong();
                        quint64 l2 = pars2[1].toULong();
                        emit statisticsUpdated(l1, l2);
                    }
                }
            }
            else if (strServerReply.contains(">STATE:", Qt::CaseInsensitive))
            {
                QStringList pars = strServerReply.split(",");
                emit stateChanged(pars[1]);

                if (strServerReply.contains("CONNECTED,SUCCESS", Qt::CaseInsensitive))
                {
                    currentState_ = STATUS_CONNECTED;
                    QSettings settings;
                    if (settings.value("killSwitch").toBool())
                    {
                        QString ip = settings.value("SERVER_IP").toString();
                        if(!ip.isEmpty())
                        {
                            QTemporaryFile tunConfig;
                            tunConfig.setAutoRemove(false);
                            if(tunConfig.open())
                            {
                                QFile f(":/Linux/oneVpnTunIpTablesConfig.conf");
                                f.open(QIODevice::ReadOnly);
                                QByteArray readen(f.readAll());
                                readen.replace("__SERVER_IP__", ip.toLatin1().data());
                                tunConfig.write(readen);
                                tunConfig.close();
                                QProcess pr;
                                QString fn = tunConfig.fileName();
                                pr.start("onevpniptablesloader", QStringList() << tunConfig.fileName());
                                pr.waitForFinished();
                            }
                        }
                    }

                    emit connected();
                }
            }
            else if (strServerReply.contains(">LOG:", Qt::CaseInsensitive))
            {
                emit log(strServerReply);
            }
        }
    }
    qDebug() << "OpenVPN socket thread stopped";

    if (socketOpenVPN != -1)
    {
        close(socketOpenVPN);
        socketOpenVPN = -1;
    }

    bSockConnected_ = false;
    if (currentState_ != STATUS_DISCONNECTED)
    {
        QProcess::startDetached("onevpniptablesloader", QStringList() << "/tmp/onevpniptablesConfig.conf");
        emit disconnected();
        currentState_ = STATUS_DISCONNECTED;
    }
}


bool OpenVPNConnectorQt::runOpenVPN()
{
    return QProcess::startDetached("onevpnhelper", QStringList() << configPath_ << QString::number(openVpnPort_));
}

int OpenVPNConnectorQt::getAvailablePort()
{
    return 9544;
}


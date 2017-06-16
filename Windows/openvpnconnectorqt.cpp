#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "openvpnconnectorqt.h"
#include <QDebug>
#include <QApplication>
#include <QDir>

#define SERVICE_PIPE_NAME  (L"\\\\.\\pipe\\OneVPNService")

#pragma pack(push, 1)
struct MessagePacket
{
    bool blocking;
    wchar_t szCommandLine[MAX_PATH*3];
};

struct MessagePacketResult
{
    bool success;
    DWORD exitCode;
};
#pragma pack(pop)

SC_HANDLE schSCManager_ = NULL;
SC_HANDLE schService_ = NULL;
SOCKET socketOpenVPN = INVALID_SOCKET;

int recvAll(SOCKET socket, char *buf, int size)
{
    int bytes_received = 0;

    while (bytes_received != size)
    {
        int r = recv(socket, buf + bytes_received, size - bytes_received, 0);
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

int sendAll(SOCKET socket, const char *buf, int size)
{
    int bytes_transfered = 0;
    while (bytes_transfered < size)
    {
        int r = send(socket, buf + bytes_transfered, size - bytes_transfered, 0);
        if (r == SOCKET_ERROR)
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

int sgetline(SOCKET sock, char *outbuf)
{
    int bytesloaded = 0;
    int ret;
    char buf;

    do
    {
        // read a single byte
        ret = recv(sock, &buf, 1, 0);
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
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        qDebug() << "WSAStartup failed with error: " << QString::number(iResult);
    }
}

OpenVPNConnectorQt::~OpenVPNConnectorQt()
{
    wait();
    if (schService_)
    {
        // stop service
        //SERVICE_STATUS ss;
        //ControlService(schService_, SERVICE_CONTROL_STOP, &ss);
        CloseServiceHandle(schService_);
    }
    if (schSCManager_)
    {
        CloseServiceHandle(schSCManager_);
    }
    if (socketOpenVPN != INVALID_SOCKET)
    {
        closesocket(socketOpenVPN);
        socketOpenVPN = INVALID_SOCKET;
    }
    WSACleanup();
}

bool OpenVPNConnectorQt::installHelper(const QString &label)
{
    Q_ASSERT(schSCManager_ == NULL);
    Q_ASSERT(schService_ == NULL);

    schSCManager_ = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager_ == NULL)
    {
        return false;
    }

    schService_ = OpenService(schSCManager_, (LPCTSTR)label.utf16(), SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (schService_ == NULL)
    {
        return false;
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;

    if (!QueryServiceStatusEx(schService_, SC_STATUS_PROCESS_INFO,
                (LPBYTE) &ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded ))
    {
        return false;
    }

    if (ssStatus.dwCurrentState != SERVICE_RUNNING)
    {
        if (!StartService(schService_, NULL, NULL))
        {
            return false;
        }
    }

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

    if (socketOpenVPN != INVALID_SOCKET)
    {
        closesocket(socketOpenVPN);
        socketOpenVPN = INVALID_SOCKET;
    }
    currentState_ = STATUS_CONNECTING;

    start(QThread::LowPriority);
}

void OpenVPNConnectorQt::disconnect()
{
    if (socketOpenVPN != INVALID_SOCKET && bSockConnected_)
    {
        const char *message = "signal SIGTERM\n";
        sendAll(socketOpenVPN, message, strlen(message));
    }
}

bool OpenVPNConnectorQt::executeRootCommand(const QString &commandLine, quint32 *exitCode)
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwWrite = 0;
    MessagePacket packet;

    packet.blocking = true;
    wcscpy(packet.szCommandLine, (wchar_t *)commandLine.utf16());
    if (!(WriteFile(hPipe, (LPVOID)&packet, sizeof(packet), &dwWrite, 0)))
    {
        CloseHandle(hPipe);
        return false;
    }

    DWORD dwRead = 0;
    MessagePacketResult mpr;
    if (!ReadFile(hPipe, &mpr, sizeof(mpr), &dwRead, 0))
    {
        CloseHandle(hPipe);
        return false;
    }

    CloseHandle(hPipe);
    *exitCode = mpr.exitCode;
    return mpr.success;
}

void OpenVPNConnectorQt::run()
{
    openVpnPort_ = getAvailablePort();
    if (openVpnPort_ == -1)
    {
        emit error(NO_AVAILABLE_PORT);
        return;
    }

    if (!connectToPipeAndRunOpenVPN())
        return;

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
            struct addrinfo *result = NULL, *ptr = NULL, hints;
            ZeroMemory( &hints, sizeof(hints) );
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            char szPort[128];
            itoa(openVpnPort_, szPort, 10);

            int iResult = getaddrinfo("127.0.0.1", szPort, &hints, &result);
            if( iResult != 0 )
            {
                qDebug() << "getaddrinfo failed with error";
                currentState_ = STATUS_DISCONNECTED;
                return;
            }

            // Attempt to connect to an address until one succeeds
            for(ptr=result; ptr != NULL ;ptr=ptr->ai_next)
            {
                socketOpenVPN = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if (socketOpenVPN == INVALID_SOCKET)
                {
                    qDebug() << "socket failed with error";
                    return ;
                }

                iResult = ::connect( socketOpenVPN, ptr->ai_addr, (int)ptr->ai_addrlen);
                if (iResult == SOCKET_ERROR)
                {
                    closesocket(socketOpenVPN);
                    socketOpenVPN = INVALID_SOCKET;
                    continue;
                }
                break;
            }
            freeaddrinfo(result);

            if (socketOpenVPN == INVALID_SOCKET)
            {
                closesocket(socketOpenVPN);
                numberOfConnectRetries++;

                if (numberOfConnectRetries > 10)
                {
                    qDebug() << "Can't connect to openvpn socket after 10 retries";
                    currentState_ = STATUS_DISCONNECTED;
                    emit error(NO_OPENVPN_SOCKET);
                    return;
                }
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
                const char *message = "state on all\n";
                sendAll(socketOpenVPN, message, strlen(message));

                const char *message2 = "log on\n";
                sendAll(socketOpenVPN, message2, strlen(message2));

                const char *message3 = "bytecount 1\n";
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
                sendAll(socketOpenVPN, message, strlen(message));
            }
            else if (strServerReply.contains("'Auth' username entered, but not yet verified", Qt::CaseInsensitive))
            {
                char message[1024];
                sprintf(message, "password \"Auth\" %s\n", password_.toUtf8().data());
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

    if (socketOpenVPN != INVALID_SOCKET)
    {
        closesocket(socketOpenVPN);
        socketOpenVPN = INVALID_SOCKET;
    }

    bSockConnected_ = false;
    if (currentState_ != STATUS_DISCONNECTED)
    {
        emit disconnected();
        currentState_ = STATUS_DISCONNECTED;
    }
}


bool OpenVPNConnectorQt::connectToPipeAndRunOpenVPN()
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    int ms = 0;
    while (true)
    {
        hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            break;
        }

        ms++;
        if (ms > 100)
        {
            emit error(CANNOT_CONNECT_TO_SERVICE_PIPE);
            return false;
        }
        Sleep(1);
    }
    DWORD dwWrite = 0;
    MessagePacket packet;

    packet.blocking = false;
    QString strCommand = "\"" + QDir::toNativeSeparators(QApplication::applicationDirPath() + "/openvpn.exe") + "\"";
    strCommand += " --config \"" + configPath_ + "\" --management 127.0.0.1 " + QString::number(openVpnPort_) + " --management-query-passwords";
    wcscpy(packet.szCommandLine, (wchar_t *)strCommand.utf16());
    if (!(WriteFile(hPipe, (LPVOID)&packet, sizeof(packet), &dwWrite, 0)))
    {
        CloseHandle(hPipe);
        emit error(CANNOT_WRITE_TO_SERVICE_PIPE);
        return false;
    }

    DWORD dwRead = 0;
    MessagePacketResult mpr;
    if (!ReadFile(hPipe, &mpr, sizeof(mpr), &dwRead, 0))
    {
        CloseHandle(hPipe);
        emit error(CANNOT_WRITE_TO_SERVICE_PIPE);
        return false;
    }

    CloseHandle(hPipe);
    return true;
}

int OpenVPNConnectorQt::getAvailablePort()
{
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult = getaddrinfo("127.0.0.1", 0, &hints, &result);
    if ( iResult != 0 )
    {
        return -1;
    }

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        return -1;
    }

    iResult = bind( sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        freeaddrinfo(result);
        closesocket(sock);
        return -1;
    }
    struct sockaddr_in foo;
    int len = sizeof(struct sockaddr);
    int retPort = -1;
    if (getsockname(sock, (struct sockaddr *) &foo, &len) != SOCKET_ERROR)
    {
        retPort = ntohs(foo.sin_port);
    }

    freeaddrinfo(result);
    closesocket(sock);
    return retPort;
}


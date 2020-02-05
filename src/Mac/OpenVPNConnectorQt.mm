#include "OpenVPNConnectorQt.h"
#import "OpenVPNConnector.h"

OpenVPNConnector *g_openVPNConnector = nil;
OpenVPNConnectorQt *g_openVPNConnectorQt = NULL;

@interface EventListener : NSObject <OpenVPNConnectorEvents>
- (void)onConnected;
- (void)onDisconnected;
- (void)onStateChanged: (NSString *) state;
- (void)onError: (enum ERROR) error;
- (void)onLog: (NSString *)logStr;
@end

@implementation EventListener : NSObject

- (void)onConnected
{
    g_openVPNConnectorQt->emitConnected();
}
- (void)onDisconnected
{
    g_openVPNConnectorQt->emitDisconnected();
}
- (void)onStateChanged: (NSString *) state
{
    g_openVPNConnectorQt->stateChanged(QString::fromCFString((__bridge CFStringRef)state));
}
- (void)onError: (enum ERROR) error
{
    if (error == MAC_AUTH_ERROR)
    {
        g_openVPNConnectorQt->error(AUTH_ERROR);
    }
    else if (error == MAC_NO_OPENVPN_SOCKET)
    {
        g_openVPNConnectorQt->error(NO_OPENVPN_SOCKET);
    }
    else if (error == MAC_CANNOT_ALLOCATE_TUN_TAP)
    {
        g_openVPNConnectorQt->error(CANNOT_ALLOCATE_TUN_TAP);
    }
    else if (error == MAC_PROXY_AUTH_ERROR)
    {
        g_openVPNConnectorQt->error(PROXY_AUTH_ERROR);
    }
}
- (void)onLog: (NSString *)logStr
{
    g_openVPNConnectorQt->emitLog(QString::fromCFString((__bridge CFStringRef)logStr));
}

-(void)onStats: (long)bytesIn: (long)bytesOut
{
    g_openVPNConnectorQt->emitStats(bytesIn, bytesOut);
}
@end


EventListener *g_eventListener = nil;

OpenVPNConnectorQt::OpenVPNConnectorQt(QObject *parent) : QObject(parent)
{
    if (g_openVPNConnector)
    {
        assert(0);  // only one instance of OpenVPNConnectorQt possible
    }
    g_openVPNConnector = [[OpenVPNConnector alloc] init];
    g_eventListener = [[EventListener alloc] init];
    [g_openVPNConnector setEventDelegate: g_eventListener];

    qRegisterMetaType<OPENVPN_ERROR>();

    g_openVPNConnectorQt = this;
}

OpenVPNConnectorQt::~OpenVPNConnectorQt()
{
    g_openVPNConnector = nil;
    g_eventListener = nil;
    g_openVPNConnectorQt = NULL;
}

bool OpenVPNConnectorQt::installHelper(const QString &label)
{
    return [g_openVPNConnector installHelper: (__bridge NSString *)label.toCFString()];
}

void OpenVPNConnectorQt::connect(const QString &configPath, const QString &username, const QString &password, const QString &proxyUsername, const QString &proxyPassword)
{
    QString configPathWithQuotes = "'" + configPath + "'";
    g_openVPNConnector.configPath = (__bridge NSString *)configPathWithQuotes.toCFString();
    g_openVPNConnector.username = (__bridge NSString *)username.toCFString();
    g_openVPNConnector.password = (__bridge NSString *)password.toCFString();
    g_openVPNConnector.proxyUsername = (__bridge NSString *)proxyUsername.toCFString();
    g_openVPNConnector.proxyPassword = (__bridge NSString *)proxyPassword.toCFString();
    [g_openVPNConnector connect];
}

void OpenVPNConnectorQt::disconnect()
{
    [g_openVPNConnector disconnect];
}

bool OpenVPNConnectorQt::executeRootCommand(const QString commandLine, quint32 *exitCode)
{
    *exitCode = 0;
    [g_openVPNConnector executeRootCommand: (__bridge NSString *)commandLine.toCFString()];
    return true;
}

void OpenVPNConnectorQt::emitConnected()
{
    emit connected();
}

void OpenVPNConnectorQt::emitDisconnected()
{
    emit disconnected();
}

void OpenVPNConnectorQt::emitStateChanged(const QString &state)
{
    emit stateChanged(state);
}

void OpenVPNConnectorQt::emitError(OPENVPN_ERROR err)
{
    emit error(err);
}

void OpenVPNConnectorQt::emitLog(const QString &logStr)
{
    emit log(logStr);
}

void OpenVPNConnectorQt::emitStats(quint64 bytesIn, quint64 bytesOut)
{
    emit statisticsUpdated(bytesIn, bytesOut);
}

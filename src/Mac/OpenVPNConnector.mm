//
//  OpenVPNConnector.m
//  OVPNClient

#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>
#import <Security/Authorization.h>

#import "OpenVPNConnector.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <IOKit/kext/KextManager.h>

#include <QDebug>
#include <QDir>

@implementation OpenVPNConnector : NSObject

const int DEFAULT_PORT = 9544;

static const QString path2kexts = "/Library/Extensions/";

enum CONNECTION_STATUS currentState_ = STATUS_DISCONNECTED;

NSTask *task_;
NSFileHandle *file_;
NSPipe *pipe_;
NSThread *thread_;
bool bStopThread_ = false;
xpc_connection_t xpc_connection_ = nil;
bool bSockConnected_ = false;

int sock_ = -1;

-(id)init
{
    if (self = [super init])
    {
        eventDelegate_ = nil;
        self.port = DEFAULT_PORT;
    }
    return self;
}

- (void) setEventDelegate: (id <OpenVPNConnectorEvents>)delegate
{
    eventDelegate_ = delegate;
}

- (BOOL) installHelper: (NSString *)label
{
    return YES;
}

- (void) create_xpc_connection
{
    if (!xpc_connection_)
    {
        xpc_connection_ = xpc_connection_create_mach_service("com.aaa.onevpn.OVPNHelper", NULL, XPC_CONNECTION_MACH_SERVICE_PRIVILEGED);

        if (!xpc_connection_)
        {
            [NSException raise:@"OpenVPNConnector" format:@"Failed to create XPC connection"];
        }

        xpc_connection_set_event_handler(xpc_connection_, ^(xpc_object_t event)
        {
            xpc_type_t type = xpc_get_type(event);

            if (type == XPC_TYPE_ERROR) {

                if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
                    qInfo() << "XPC connection interupted.";
                }
                else if (event == XPC_ERROR_CONNECTION_INVALID) {
                    qInfo() << "XPC connection invalid, releasing.";

                }
                else {
                    qInfo() << "Unexpected XPC connection error.";
                }
                xpc_connection_ = nil;

            } else {
                qInfo() << "Unexpected XPC connection event.";
            }
        });

        xpc_connection_resume(xpc_connection_);
    }
}

- (void) executeRootCommand: (NSString *)commandLine
{
    [self create_xpc_connection];

    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(message, "cmd", "anycmd");
    xpc_dictionary_set_string(message, "command_line", [commandLine UTF8String]);

    xpc_object_t event = xpc_connection_send_message_with_reply_sync(xpc_connection_, message);
    const char* response = xpc_dictionary_get_string(event, "reply");
    qInfo() << [[NSString stringWithFormat:@"Received response: %s.", response] UTF8String];
}

- (void) connect
{
    [eventDelegate_ onLog: @"Start connect"];

    if (_configPath == nil)
    {
        [NSException raise:@"OpenVPNConnector" format:@"configPath can't be NULL"];
    }
    if (_username == nil)
    {
        [NSException raise:@"OpenVPNConnector" format:@"username can't be NULL"];
    }
    if (_password == nil)
    {
        [NSException raise:@"OpenVPNConnector" format:@"_password can't be NULL"];
    }
    if (eventDelegate_ == nil)
    {
        [NSException raise:@"OpenVPNConnector" format:@"Neet set OpenVPNConnectorEvents for get events from connector"];
    }
    
    if (currentState_ != STATUS_DISCONNECTED)
    {
        [NSException raise:@"OpenVPNConnector" format:@"already used for connection. Need to disconnect first"];
    }
    
    currentState_ = STATUS_CONNECTING;
   
    // get path to openvpn util
   NSString *pathToOpenVPN = [NSString stringWithFormat:@"'%@%@'", [[NSBundle mainBundle] bundlePath], @"/Contents/Resources/openvpn"];
    // NSString *pathToOpenVPN = @"/usr/local/opt/openvpn/sbin/openvpn";
//     NSString *pathToOpenVPN = @"/Users/sergey/onevpn-wrapper/Mac/Resources/openvpn";

        // get path for tun/tap kexts
//    NSString *pathToTunKext = [NSString stringWithFormat:@"'%@%@'", [[NSBundle mainBundle] bundlePath], @"/Contents/Resources/tun-signed.kext"];
//    NSString *pathToTapKext = [NSString stringWithFormat:@"'%@%@'", [[NSBundle mainBundle] bundlePath], @"/Contents/Resources/tap-signed.kext"];

    NSString *pathToTunKext = (path2kexts + "tun.kext").toNSString();
    NSString *pathToTapKext = (path2kexts + "tap.kext").toNSString();

    qDebug() << [pathToTunKext UTF8String];
    qDebug() << [pathToTapKext UTF8String];
    
    [self create_xpc_connection];
    
    //https://zorrovpn.com/articles/osx-pf-vpn-only?lang=en - thats good!
    NSString *cmdOpenVPN = [NSString stringWithFormat:@"%@ %@ %@ %@ %@ %@ %@ &", pathToOpenVPN, @"--config", _configPath, @"--management", @"127.0.0.1", [NSString stringWithFormat:@"%d",_port], @"--management-query-passwords"];
    qInfo() << [cmdOpenVPN UTF8String];

    [eventDelegate_ onLog: cmdOpenVPN];
    
    xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
    
    xpc_dictionary_set_string(message, "cmd", "openvpn");
    xpc_dictionary_set_string(message, "openvpncmd", [cmdOpenVPN UTF8String]);
    xpc_dictionary_set_string(message, "path_tun", [pathToTunKext UTF8String]);
    xpc_dictionary_set_string(message, "path_tap", [pathToTapKext UTF8String]);
    
    xpc_connection_send_message_with_reply(xpc_connection_, message, dispatch_get_main_queue(), ^(xpc_object_t event)
    {
        const char* response = xpc_dictionary_get_string(event, "reply");

        qInfo() << [[NSString stringWithFormat:@"Received response: %s.", response] UTF8String];
    });
    
    bStopThread_ = false;
    bSockConnected_ = false;
    thread_ = [[NSThread alloc] initWithTarget:self selector:@selector(controlOpenVPNThread) object:nil];
    [thread_ start];
}

- (void) disconnect
{
    if (sock_ != -1 && bSockConnected_)
    {
        char *message = "signal SIGTERM\n";
        [self sendAll:message :strlen(message)];
    }
    if (sock_ != -1)
    {
        close(sock_);
        sock_ = -1;
    }
}

-(enum CONNECTION_STATUS)getConnectionStatus
{
    return currentState_;
}

-(int) sendAll: (char *)buf : (ssize_t)len
{
    @synchronized(self)
    {
        ssize_t total = 0;
        ssize_t l = len;
        ssize_t bytesleft = l;
        ssize_t n = 0;
        while(total < l)
        {
            n = send(sock_, buf+total, bytesleft, 0);
            if (n == -1)
            {
                qInfo() << "send to socket failed";
                break;
            }
            total += n;
            bytesleft -= n;
        }
        return n==-1?-1:0; // return -1 on failure, 0 on success
    }
}

-(int)sgetline: (int)sock : (char *) outbuf
{
    int bytesloaded = 0;
    ssize_t ret;
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

-(void)controlOpenVPNThread
{
    qInfo() << "controlOpenVPNThread started";
    int numberOfConnectRetries = 0;
    char server_reply[10000];
    bool bStateModeOn = false;
    bool bProxyAuthErrorEmited = false;
    
    
    while (!bStopThread_)
    {
        if (!bSockConnected_)
        {
            sock_ = socket(AF_INET, SOCK_STREAM, 0 );
            if (sock_ == -1)
            {
                [NSException raise:@"OpenVPNConnector" format:@"cannot create socket"];
                currentState_ = STATUS_DISCONNECTED;
                return;
            }
            
            struct sockaddr_in serv_addr;
            bzero(&serv_addr, sizeof(serv_addr));
            serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(_port);
            
            if (connect(sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
            {
                close(sock_);
                numberOfConnectRetries++;
                
                if (numberOfConnectRetries > 1000)
                {
                    qWarning() << "Can't connect to openvpn socket after 1000 retries";
                    currentState_ = STATUS_DISCONNECTED;
                    [eventDelegate_ onError: MAC_NO_OPENVPN_SOCKET];
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
                char *message = "state on all\n";
                [self sendAll:message :strlen(message)];
                
                char *message2 = "log on\n";
                [self sendAll:message2 :strlen(message2)];

                char *message3 = "bytecount 1\n";
                [self sendAll:message3 :strlen(message3)];

                bStateModeOn = true;
            }
            
            //Receive a reply line from the server
            ssize_t ret = [self sgetline:sock_: server_reply];
            if(ret < 0)
            {
                break;
            }
            
            NSString *strServerReply = [NSString stringWithUTF8String:server_reply];
            
            qInfo() << [strServerReply UTF8String];
            
            if ([strServerReply rangeOfString:@"PASSWORD:Need 'Auth' username/password"].location != NSNotFound)
            {
                char *message[1024];
                sprintf((char *)message, "username \"Auth\" %s\n", [_username UTF8String]);
                [self sendAll:(char *)message :strlen((char *)message)];
                sprintf((char *)message, "password \"Auth\" %s\n", [_password UTF8String]);
                [self sendAll:(char *)message :strlen((char *)message)];
            }
            else if ([strServerReply rangeOfString:@"PASSWORD:Need 'HTTP Proxy' username/password"].location != NSNotFound)
            {
                char *message[1024];
                sprintf((char *)message, "username \"HTTP Proxy\" %s\n", [_proxyUsername UTF8String]);
                [self sendAll:(char *)message :strlen((char *)message)];
                sprintf((char *)message, "password \"HTTP Proxy\" %s\n", [_proxyPassword UTF8String]);
                [self sendAll:(char *)message :strlen((char *)message)];
            }
            else if ([strServerReply rangeOfString:@"PASSWORD:Verification Failed: 'Auth'"].location != NSNotFound)
            {
                [eventDelegate_ onError: MAC_AUTH_ERROR];
            }
            else if ([strServerReply rangeOfString:@"FATAL:Cannot allocate TUN/TAP dev dynamically"].location != NSNotFound)
            {
                [eventDelegate_ onError: MAC_CANNOT_ALLOCATE_TUN_TAP];
            }
            else if ([strServerReply rangeOfString:@"Proxy requires authentication"].location != NSNotFound)
            {
                if (!bProxyAuthErrorEmited)
                {
                    [eventDelegate_ onError: MAC_PROXY_AUTH_ERROR];
                    bProxyAuthErrorEmited = true;
                }
            }
            else if ([strServerReply rangeOfString:@">BYTECOUNT:"].location != NSNotFound)
            {
                NSArray *pars = [strServerReply componentsSeparatedByString: @":"];
                if (pars.count > 1)
                {
                    NSArray *pars2 = [pars[1] componentsSeparatedByString: @","];
                    if (pars2.count == 2)
                    {
                        long l1 = [pars2[0] longLongValue];
                        long l2 = [pars2[1] longLongValue];
                        [eventDelegate_ onStats: l1: l2];
                    }
                }

            }
            else if ([strServerReply rangeOfString:@">STATE:"].location != NSNotFound)
            {
                NSArray *pars = [strServerReply componentsSeparatedByString: @","];
                [eventDelegate_ onStateChanged: pars[1]];
                
                if ([strServerReply rangeOfString:@"CONNECTED,SUCCESS"].location != NSNotFound)
                {
                    currentState_ = STATUS_CONNECTED;
                    [eventDelegate_ onConnected];
                }
            }
            else if ([strServerReply rangeOfString:@">LOG:"].location != NSNotFound)
            {
                //NSArray *pars = [strServerReply componentsSeparatedByString: @","];
                //[eventDelegate_ onLog: pars[2]];
                [eventDelegate_ onLog: strServerReply];

            }
        }
        
        [NSThread sleepForTimeInterval:0.001];
    }
    qInfo() << "controlOpenVPNThread stopped";
    bSockConnected_ = false;
    if (currentState_ != STATUS_DISCONNECTED)
    {
        [eventDelegate_ onDisconnected];
        currentState_ = STATUS_DISCONNECTED;
    }
}

@end

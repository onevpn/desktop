#include "Connection.h"
#include <QApplication>
#include <QDebug>
#include <QStandardPaths>

Connection *g_connectionThis;

OpenVPNConnectorQt *g_openVPNConnection = NULL;

Connection::Connection(QObject *parent)
    : AbstractConnection(parent)
    , m_connection(NULL)
    , bConnected_(false)
    , bDisconnectInitiatedByUser_(false)
{
    qRegisterMetaType<OPENVPN_ERROR>("OPENVPN_ERROR");

    g_openVPNConnection = new OpenVPNConnectorQt(this);

    QObject::connect(g_openVPNConnection, SIGNAL(connected()), SLOT(onOpenVPNConnected()));
    QObject::connect(g_openVPNConnection, SIGNAL(disconnected()), SLOT(onOpenVPNDisconnected()));
    QObject::connect(g_openVPNConnection, SIGNAL(error(OPENVPN_ERROR)), SLOT(onOpenVPNError(OPENVPN_ERROR)));
    QObject::connect(g_openVPNConnection, SIGNAL(log(QString)), SLOT(onOpenVPNLog(QString)));
    QObject::connect(g_openVPNConnection, SIGNAL(statisticsUpdated(quint64,quint64)), SIGNAL(statisticsChanged(quint64,quint64)));

    g_connectionThis = this;
    m_l2tpServiceId  = NULL;
    m_pptpServiceId  = NULL;

    QString qAppName = qApp->applicationName();

    m_appName = CFStringCreateWithCString( NULL
                         , QString(qAppName).toUtf8().data()
                         , kCFStringEncodingUTF8 );
    m_l2tpServiceName = CFStringCreateWithCString( NULL
                          , QString(qAppName + ":L2TP").toUtf8().data()
                          , kCFStringEncodingUTF8 );
    m_pptpServiceName = CFStringCreateWithCString( NULL
                          , QString(qAppName + ":PPTP").toUtf8().data()
                          , kCFStringEncodingUTF8 );

    m_prefs        = NULL;
    m_l2tpService  = NULL;
    m_pptpService  = NULL;

    AuthorizationFlags flags = kAuthorizationFlagDefaults
                             | kAuthorizationFlagExtendRights
                             | kAuthorizationFlagInteractionAllowed
                             | kAuthorizationFlagPreAuthorize;
    OSStatus err;
    AuthorizationRef auth;
    err = AuthorizationCreate(0, kAuthorizationEmptyEnvironment, flags, &auth);
    if (err == noErr)
        m_prefs = SCPreferencesCreateWithAuthorization(0, m_appName, 0, auth);
    else
        m_prefs = SCPreferencesCreate(0, m_appName, 0);

    if (!m_prefs) return;

    CFArrayRef connArray = SCNetworkServiceCopyAll(m_prefs);
    if (!connArray) return;

    for (CFIndex i = 0, sz = CFArrayGetCount(connArray); i < sz; ++i)
    {
        SCNetworkServiceRef service;
        service = (SCNetworkServiceRef) CFArrayGetValueAtIndex(connArray, i);

        CFStringRef serviceName = SCNetworkServiceGetName(service);

        if (kCFCompareEqualTo == CFStringCompare(m_l2tpServiceName
                                   , serviceName
                                   , kCFCompareCaseInsensitive)
        ) { m_l2tpServiceId = SCNetworkServiceGetServiceID(service); }

        if (kCFCompareEqualTo == CFStringCompare(m_pptpServiceName
                                   , serviceName
                                   , kCFCompareCaseInsensitive)
        ) { m_pptpServiceId = SCNetworkServiceGetServiceID(service); }
    }

    CFRelease(connArray);
}

Connection::~Connection()
{
    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }
    delete g_openVPNConnection;
}

bool Connection::connect(PROTOCOL protocol, QString serverIP, QString username, QString password, QString ovpnFile, QString l2tpKey)
{
    if (protocol == PPTP)
    {
        emit log("Do connect to PPTP");
        return connectPPTP(serverIP, username, password);
    }
    else if (protocol == L2TP)
    {
        emit log("Do connect to L2TP");
        return connectL2TP(serverIP, username, password, l2tpKey);
    }
    else if (protocol == SSTP)
    {
        Q_ASSERT(false);
    }
    else if (protocol == OPENVPN)
    {
        emit log("Do connect to OpenVPN");
        g_openVPNConnection->connect(ovpnFile, username, password, "", "");
        return true;
    }
}

bool Connection::connectPPTP(QString serverIP, QString inUsername, QString inPassword)
{
    if (!m_pptpServiceId)
        m_pptpServiceId = initService( kSCNetworkInterfaceTypePPTP
                                     , m_pptpServiceName );
    if (!m_pptpServiceId)
        return false;

    std::string username = inUsername.toStdString();
    std::string password = inPassword.toStdString();
    std::string hostname = serverIP.toStdString();


    CFStringRef cfUsername = CFStringCreateWithCString( NULL
                                                      , username.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfPassword = CFStringCreateWithCString( NULL
                                                      , password.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfHostname = CFStringCreateWithCString( NULL
                                                      , hostname.c_str()
                                                      , kCFStringEncodingUTF8 );
   CFIndex index = 0;
    const void *pppKeys[3], *pppVals[3];
    pppKeys[index] = (void*) kSCPropNetPPPAuthName;
    pppVals[index] = (void*) cfUsername;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPAuthPassword;
    pppVals[index] = (void*) cfPassword;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPCommRemoteAddress;
    pppVals[index] = (void*) cfHostname;
    index++;

    CFDictionaryRef pppDialOptions;
    pppDialOptions = CFDictionaryCreate( NULL
                                       , pppKeys
                                       , pppVals
                                       , index
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    CFStringRef     keys[] = { kSCEntNetPPP   };
    CFDictionaryRef vals[] = { pppDialOptions };

    CFDictionaryRef optionsForDial;
    optionsForDial = CFDictionaryCreate( NULL
                                       , (const void **) &keys
                                       , (const void **) &vals
                                       , 1
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    bConnected_ = false;

    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }

    m_connection = initConnection( m_pptpServiceId, optionsForDial );
    return true;
}

bool Connection::connectL2TP(QString serverIP, QString inUsername, QString inPassword, QString inL2tpKey)
{
    if (!m_l2tpServiceId)
        m_l2tpServiceId = initService( kSCNetworkInterfaceTypeL2TP
                                     , m_l2tpServiceName );
    if (!m_l2tpServiceId)
        return false;

    std::string username = inUsername.toStdString();
    std::string password = inPassword.toStdString();
    std::string hostname = serverIP.toStdString();
    std::string l2tpKey = inL2tpKey.toStdString();

    CFStringRef cfUsername = CFStringCreateWithCString( NULL
                                                      , username.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfPassword = CFStringCreateWithCString( NULL
                                                      , password.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfHostname = CFStringCreateWithCString( NULL
                                                      , hostname.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfSecret = CFStringCreateWithCString( NULL
                                                    , l2tpKey.c_str()
                                                    , kCFStringEncodingUTF8 );
    CFIndex index = 0;
    const void *pppKeys[3], *pppVals[3];
    pppKeys[index] = (void*) kSCPropNetPPPAuthName;
    pppVals[index] = (void*) cfUsername;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPAuthPassword;
    pppVals[index] = (void*) cfPassword;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPCommRemoteAddress;
    pppVals[index] = (void*) cfHostname;
    index++;

    CFDictionaryRef pppDialOptions;
    pppDialOptions = CFDictionaryCreate( NULL
                                       , pppKeys
                                       , pppVals
                                       , index
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    index = 0;
    const void *l2tpKeys[3], *l2tpVals[3];


    l2tpKeys[index] = (void*) kSCPropNetL2TPIPSecSharedSecret;
    l2tpVals[index] = (void*) cfSecret;
    index++;

    l2tpKeys[index] = (void*) kSCPropNetL2TPIPSecSharedSecretEncryption;
    l2tpVals[index] = (void*) kSCValNetL2TPIPSecSharedSecretEncryptionKeychain;
    index++;

    l2tpKeys[index] = (void*) kSCPropNetL2TPTransport;
    l2tpVals[index] = (void*) kSCValNetL2TPTransportIP; // kSCValNetL2TPTransportIPSec
    index++;

    CFDictionaryRef l2tpDialOptions;
    l2tpDialOptions = CFDictionaryCreate( NULL
                                        , l2tpKeys
                                        , l2tpVals
                                        , index
                                        , &kCFTypeDictionaryKeyCallBacks
                                        , &kCFTypeDictionaryValueCallBacks );

    CFStringRef     keys[] = { kSCEntNetPPP   , kSCEntNetL2TP   };
    CFDictionaryRef vals[] = { pppDialOptions , l2tpDialOptions };

    CFDictionaryRef optionsForDial;
    optionsForDial = CFDictionaryCreate( NULL
                                       , (const void **) &keys
                                       , (const void **) &vals
                                       , 2
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    bConnected_ = false;
    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }
    m_connection = initConnection( m_l2tpServiceId, optionsForDial );
    return true;
 }

void Connection::disconnect()
{
    //bDisconnectInitiatedByUser_ = true;
    SCNetworkConnectionStop(m_connection, TRUE);
    g_openVPNConnection->disconnect();
}

bool Connection::initialize()
{
    return g_openVPNConnection->installHelper("com.aaa.onevpn.OVPNHelper");
}

bool Connection::tapInstalled()
{
    //return openManager_.tapInstalled();
    return true;
}

CFStringRef Connection::initService( CFStringRef type , CFStringRef name )
{
    if (!SCPreferencesLock(m_prefs, FALSE))
        return NULL;

    SCNetworkInterfaceRef IF;
    IF = SCNetworkInterfaceCreateWithInterface(kSCNetworkInterfaceIPv4, type);
    if (IF)
        IF = SCNetworkInterfaceCreateWithInterface(IF, kSCNetworkInterfaceTypePPP);
    if (!IF) return NULL;

    SCNetworkServiceRef service;
    service = SCNetworkServiceCreate(m_prefs, IF);
    if (!service) return NULL;

    Boolean operationOk;
    operationOk = SCNetworkServiceSetName(service, name);
    if (!operationOk) return NULL;

    operationOk = SCNetworkServiceEstablishDefaultConfiguration(service);
    if (!operationOk) return NULL;

    SCNetworkProtocolRef protocol;
    protocol = SCNetworkServiceCopyProtocol(service, CFSTR("IPv4"));
    if (!protocol) return NULL;

    CFDictionaryRef ip4DialOptions = NULL;
    const void *ip4Keys[1];
    const void *ip4Vals[1];
    const int val = 1;
    ip4Keys[0] = (void *) kSCPropNetOverridePrimary;
    ip4Vals[0] = (void *) CFNumberCreate(NULL, kCFNumberIntType, &val);
    ip4DialOptions = CFDictionaryCreate( NULL
                                       , (const void**)ip4Keys
                                       , (const void**)ip4Vals
                                       , 1
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    operationOk = SCNetworkProtocolSetConfiguration( protocol, ip4DialOptions );

    SCNetworkSetRef set = SCNetworkSetCopyCurrent(m_prefs);
    if (!set) return NULL;

    if (set && SCNetworkSetAddService(set, service))
    {
        if (!SCPreferencesCommitChanges(m_prefs))
            qDebug() << "Error adding service (on commit) : " << SCErrorString(SCError());

        if (!SCPreferencesApplyChanges(m_prefs))
            qDebug() << "Error adding service (on apply) : " << SCErrorString(SCError());

        SCPreferencesSynchronize(m_prefs);
        // sleep(2);
    }
    CFStringRef serviceId = SCNetworkServiceGetServiceID(service);

    SCPreferencesUnlock(m_prefs);

    CFRelease(set);
    CFRelease(ip4DialOptions);
    CFRelease(protocol);

    return serviceId;
}

SCNetworkConnectionRef Connection::initConnection(CFStringRef serviceId , CFDictionaryRef options)
{
    if (!serviceId) return NULL;

    for (int I = 0; I < 3; I++)
    {
        SCNetworkConnectionRef connection;
        connection = SCNetworkConnectionCreateWithServiceID( NULL
                       , serviceId
                       , Connection::callback
                       , NULL );
        if (!connection) return NULL;

        Boolean ok;
        ok = SCNetworkConnectionScheduleWithRunLoop( connection
                                                   , CFRunLoopGetCurrent()
                                                   , kCFRunLoopDefaultMode );
        if (!ok) return NULL;

        ok = SCNetworkConnectionStart(connection, options, false);
        if (ok) return connection;

        sleep(3);
    }

    return NULL;
}


void Connection::callback( SCNetworkConnectionRef connection, SCNetworkConnectionStatus status, void *info)
{
    CFDictionaryRef dict;
    CFIndex count;
    switch(status)
    {
        case kSCNetworkConnectionDisconnected:
           if (!g_connectionThis->bConnected_)
           {
               g_connectionThis->emit error(QString());
           }
           else
           {
               g_connectionThis->bConnected_ = false;
               g_connectionThis->killTimer(g_connectionThis->timerId_);

               /*if (g_connectionThis->bDisconnectInitiatedByUser_)
               {
                   g_connectionThis->bDisconnectInitiatedByUser_ = false;
                   g_connectionThis->emit disconnected(false);
               }
               else
               {
                   g_connectionThis->emit disconnected(true);
               }*/
               g_connectionThis->emit disconnected();
           }
           g_connectionThis->emit log("Disconnected");
           //dict = SCNetworkConnectionCopyExtendedStatus(connection);
           //CFDictionaryApplyFunction(dict, printKeys, NULL);

        //count = CFDictionaryGetCount(ct);
           //CFDictionaryGetKeysAndValues(dict, )


            break;
        case kSCNetworkConnectionConnecting:
            g_connectionThis->emit log("Connecting");

                break;
        case kSCNetworkConnectionConnected:
            {
                g_connectionThis->emit log("Connected");
                g_connectionThis->bConnected_ = true;
                g_connectionThis->emit connected();
                g_connectionThis->bFirstCalcStat_ = true;
                g_connectionThis->prevBytesRcved_ = 0;
                g_connectionThis->prevBytesXmited_ = 0;
                g_connectionThis->timerId_ = g_connectionThis->startTimer(1000);
            }
             break;
        case kSCNetworkConnectionDisconnecting:
            g_connectionThis->emit log("Disconnecting");

                break;
        case kSCNetworkConnectionInvalid:
            g_connectionThis->emit log("Connection Invalid");

                break;

    }
}


static void printKeys (const void* key, const void* value, void* context)
{
  CFShow(key);
}

void Connection::timerEvent( QTimerEvent *event )
{
    if (bConnected_)
    {
        CFDictionaryRef dic = SCNetworkConnectionCopyStatistics(m_connection);
        if (dic)
        {
            CFDictionaryRef dictRef = (CFDictionaryRef)CFDictionaryGetValue(dic, CFSTR("PPP"));
            CFNumberRef numberRecvRef = (CFNumberRef)CFDictionaryGetValue(dictRef, kSCNetworkConnectionBytesIn);
            CFNumberRef numberSendRef = (CFNumberRef)CFDictionaryGetValue(dictRef, kSCNetworkConnectionBytesOut);

            SInt64 bytesRcved;
            CFNumberGetValue(numberRecvRef, kCFNumberSInt64Type, &bytesRcved);
            SInt64 bytesXmited;
            CFNumberGetValue(numberSendRef, kCFNumberSInt64Type, &bytesXmited);

            emit statisticsChanged(bytesRcved, bytesXmited);
            /*if (bFirstCalcStat_)
            {
                prevBytesRcved_ = bytesRcved;
                prevBytesXmited_ = bytesXmited;
                bFirstCalcStat_ = false;
            }
            else
            {
                emit statisticsChanged(bytesRcved - prevBytesRcved_, bytesXmited - prevBytesXmited_);
                prevBytesRcved_ = bytesRcved;
                prevBytesXmited_ = bytesXmited;
            }*/
         }
    }
}

void Connection::onOpenVPNConnected()
{
    emit connected();
}

void Connection::onOpenVPNDisconnected()
{
    emit disconnected();
}

void Connection::onOpenVPNError(OPENVPN_ERROR err)
{
    if (err == AUTH_ERROR)
    {
        emit error("Incorrect username or password");
    }
    else if (err == CANNOT_ALLOCATE_TUN_TAP)
    {
        emit error("There are no TAP-Windows adapters on this system. Please install.");
    }
    else
    {
        emit error("Error during connection (" + QString::number(err) + ")");
    }
}

void Connection::onOpenVPNLog(const QString &logStr)
{
    emit log(logStr);
}

#include "dnsleaks.h"
#include <QSettings>

#if defined Q_OS_WIN
    #include <qDebug>
    #include <windows.h>
    #include <Iphlpapi.h>
    #include "Windows/openvpnconnectorqt.h"
    #include <string>

extern OpenVPNConnectorQt *g_openVPNConnection;

typedef struct _Adapt_Info
{
    char			AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    std::string			NameSever;
} AdaptInfo, *PADAPTINFO;
#endif


DnsLeaks::DnsLeaks()
{

}

DnsLeaks::~DnsLeaks()
{
    enable(false);
}

void DnsLeaks::enable(bool bEnabled)
{
    if (isEnable() == bEnabled)
        return;

#if defined Q_OS_WIN
    if (bEnabled)
    {
        qDebug() <<"------------------ DNS LEAK ON -------------";

        QMap<QString, QVariant> oldDnsLeak;
        oldDnsLeak.clear();

        assignNewDns("0.0.0.0,0.0.0.0", oldDnsLeak);
        qDebug() << oldDnsLeak;
        qDebug() <<"--------------------------------------------";

        QSettings settings;
        settings.setValue("oldDnsLeak", oldDnsLeak);
    }
    else
    {
        qDebug() <<"------------------ DNS LEAK OFF -------------";
        QSettings settings;
        QMap<QString, QVariant> oldDnsLeak = settings.value("oldDnsLeak").toMap();
        restoreOldDns(oldDnsLeak);
        qDebug() <<"--------------------------------------------";
        settings.remove("oldDnsLeak");
    }
#endif
}

bool DnsLeaks::isEnable()
{
    QSettings settings;
    return settings.contains("oldDnsLeak");
}

#if defined Q_OS_WIN

void DnsLeaks::assignNewDns(const QString &newDns, QMap<QString, QVariant> &dns)
{
    dns.clear();

    PIP_ADDR_STRING   pAddrStr;

    ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
    QByteArray pAdapterInfo(ulAdapterInfoSize, Qt::Uninitialized);

    if( GetAdaptersInfo((IP_ADAPTER_INFO *)pAdapterInfo.data(), &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW ) // out of buff
    {
        pAdapterInfo.resize(ulAdapterInfoSize);
    }

    if( GetAdaptersInfo((IP_ADAPTER_INFO *)pAdapterInfo.data(), &ulAdapterInfoSize) == ERROR_SUCCESS )
    {
        IP_ADAPTER_INFO *ai = (IP_ADAPTER_INFO *)pAdapterInfo.data();

        do
        {
            if ((ai->Type == MIB_IF_TYPE_ETHERNET) 	// If type is etherent
                    || (ai->Type == IF_TYPE_IEEE80211))   // radio
            {
                ULONG ulPerAdapterInfoSize = sizeof(IP_PER_ADAPTER_INFO);
                QByteArray pPerAdapterInfo(ulPerAdapterInfoSize, Qt::Uninitialized);

                if( GetPerAdapterInfo(
                            ai->Index,
                            (IP_PER_ADAPTER_INFO*)pPerAdapterInfo.data(),
                            &ulPerAdapterInfoSize) == ERROR_BUFFER_OVERFLOW ) // out of buff
                {
                    pPerAdapterInfo.resize(ulPerAdapterInfoSize);
                }

                DWORD dwRet;
                if((dwRet = GetPerAdapterInfo(
                            ai->Index,
                            (IP_PER_ADAPTER_INFO*)pPerAdapterInfo.data(),
                            &ulPerAdapterInfoSize)) == ERROR_SUCCESS)
                {
                    IP_PER_ADAPTER_INFO *pai = (IP_PER_ADAPTER_INFO*)pPerAdapterInfo.data();
                    AdaptInfo tmpadpif;

                    pAddrStr   =   pai->DnsServerList.Next;

                    strncpy(tmpadpif.AdapterName, ai->AdapterName, sizeof(ai->AdapterName));
                    tmpadpif.NameSever = pai->DnsServerList.IpAddress.String;

                    //ignore TAP
                    if ( strstr(ai->Description, "TAP") == 0 )
                    {
                        qDebug() <<"pAdapterInfo->Description: " << ai->Description;

                        while(pAddrStr)
                        {
                            tmpadpif.NameSever += ",";
                            tmpadpif.NameSever += pAddrStr->IpAddress.String;
                            pAddrStr   =   pAddrStr->Next;
                        }
                        qDebug() << "AdapterName: " <<tmpadpif.AdapterName <<"DNS server: " <<tmpadpif.NameSever.c_str();

                        dns.insert( tmpadpif.AdapterName, QString::fromUtf8(tmpadpif.NameSever.c_str()));

                        qDebug()<<"RegSetDNS: " << regSetDNS( tmpadpif.AdapterName, newDns.toStdString().c_str());

                        qDebug()<<"NotifyIPChange: " << notifyIPChange( tmpadpif.AdapterName );
                    }
                }
            }
            ai = ai->Next;
        } while(ai);

        qDebug() << flushDNS();
    }

}

void DnsLeaks::restoreOldDns(QMap<QString, QVariant> &dns)
{
    QMap<QString, QVariant>::iterator i = dns.begin();
    while (i != dns.end()) {
        regSetDNS(i.key().toStdString().c_str(), i.value().toString().toStdString().c_str());
        notifyIPChange(i.key().toStdString().c_str());

        qDebug() <<"Adapter: " <<i.key() <<"DNS: " <<i.value();

        ++i;
    }
    dns.clear();

    flushDNS();
}

bool DnsLeaks::regSetDNS(const QString &lpszAdapterName, const QString &pDNS)
{
    QString strKeyName = "HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + lpszAdapterName;
    QString commandLine = "REG ADD " + strKeyName + " /v NameServer /t REG_SZ /d \"" + pDNS + "\" /f";

    quint32 exitCode;
    if (g_openVPNConnection->executeRootCommand(commandLine, &exitCode))
    {
        return exitCode == 0;
    }
    else
    {
        return false;
    }
}

bool DnsLeaks::notifyIPChange(const char *lpszAdapterName)
{
    typedef int (CALLBACK* DHCPNOTIFYPROC)(LPWSTR, LPWSTR, BOOL, DWORD, DWORD, DWORD, int);
    BOOL			bResult = FALSE;
    HINSTANCE		hDhcpDll;
    DHCPNOTIFYPROC	pDhcpNotifyProc;
    WCHAR wcAdapterName[256];

    MultiByteToWideChar(CP_ACP, 0, lpszAdapterName, -1, wcAdapterName,256);

    if((hDhcpDll = LoadLibrary("dhcpcsvc")) == NULL)
        return FALSE;

    if((pDhcpNotifyProc = (DHCPNOTIFYPROC)GetProcAddress(hDhcpDll, "DhcpNotifyConfigChange")) != NULL)
        if((pDhcpNotifyProc)(NULL, wcAdapterName, FALSE, 0, NULL,NULL, 0) == ERROR_SUCCESS)
            bResult = TRUE;

    FreeLibrary(hDhcpDll);
    return bResult;
}

bool DnsLeaks::flushDNS()
{
    typedef int (CALLBACK* DNSFLUSHPROC)();
    bool			bResult = FALSE;
    HINSTANCE		hDnsDll;
    DNSFLUSHPROC	pDnsFlushProc;

    if((hDnsDll = LoadLibrary("dnsapi")) == NULL) {
        return false;
    }

    if((pDnsFlushProc = (DNSFLUSHPROC)GetProcAddress(hDnsDll, "DnsFlushResolverCache")) != NULL)
    {
        if ( (pDnsFlushProc)() == ERROR_SUCCESS)
        {
            bResult = false;
        }
    }

    FreeLibrary(hDnsDll);
    return bResult;
}
#endif

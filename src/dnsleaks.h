#ifndef DNSLEAKS_H
#define DNSLEAKS_H
#include <QMap>
#include <QVariant>

class DnsLeaks
{
public:
    DnsLeaks();
    virtual ~DnsLeaks();
    void enable(bool bEnabled);
    bool isEnable();

private:
#if defined Q_OS_WIN
    void assignNewDns(const QString &newDns, QMap<QString, QVariant> &dns);
    void restoreOldDns(QMap<QString, QVariant> &dns);
    bool regSetDNS(const QString &lpszAdapterName, const QString &pDNS);
    bool notifyIPChange(const char *lpszAdapterName);
    bool flushDNS();
#endif
};

#endif // DNSLEAKS_H

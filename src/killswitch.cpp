#include "killswitch.h"

#include <QProcess>
#include <QFile>
#include <QTextCodec>
#include <QDebug>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

#if defined Q_OS_MAC
    #include "Mac/OpenVPNConnectorQt.h"
#elif defined Q_OS_WIN
    #include "Windows/OpenVPNConnectorQt.h"
#elif defined Q_OS_UNIX
    #include "../Linux/openvpnconnectorqt.h"
#endif

extern OpenVPNConnectorQt *g_openVPNConnection;

KillSwitch::KillSwitch(QObject *parent) : QObject(parent)
{
}

KillSwitch::~KillSwitch()
{
    if (isActive())
    {
        restoreInternet();
    }
}

bool KillSwitch::killInternet()
{
#if defined Q_OS_WIN
    QString cmd = "route DELETE 0.0.0.0";
    quint32 exitCode;
    bool b = g_openVPNConnection->executeRootCommand(cmd, &exitCode);
    qDebug() << cmd << " : executed = " << b << "; exitCode = " << exitCode;
#elif defined Q_OS_MAC
    quint32 exitCode;
    g_openVPNConnection->executeRootCommand("route delete default", &exitCode);
#endif

    QSettings settings;
    settings.setValue("killSwitchActive", true);

    return true;
}

bool KillSwitch::restoreInternet()
{
    if (!isActive())
    {
        return true;
    }

    QList<GatewayData> gw = loadGatewayFromSettings();

    foreach (GatewayData data, gw) {

#if defined Q_OS_WIN
        QString cmd = "route ADD 0.0.0.0 mask 0.0.0.0 " + data.host_;
        quint32 exitCode;
        bool b = g_openVPNConnection->executeRootCommand(cmd, &exitCode);
        qDebug() << cmd << " : executed = " << b << "; exitCode = " << exitCode;
#elif defined Q_OS_MAC
        // выполнить команду от рута route add default %data.host_%
        quint32 exitCode;
        g_openVPNConnection->executeRootCommand("route add default " + data.host_, &exitCode);
#endif
    }

    QSettings settings;
    settings.remove("savedGateway");
    settings.remove("killSwitchActive");
    return true;
}

bool KillSwitch::isActive()
{
    QSettings settings;
    return settings.contains("killSwitchActive");
}

void KillSwitch::saveDefaultGateway()
{
    QList<GatewayData> dumpGateway;
    QProcess process(this);
    QStringList params;

#if defined Q_OS_WIN
    params << "print" <<"0*";
    process.start("route", params);
#elif defined Q_OS_MAC
    params << "-nr";
    process.start("netstat", params);
#endif
    process.waitForFinished(2000);

    // read console output
#if defined Q_OS_WIN
    QTextCodec *codec = QTextCodec::codecForName("CP866");  //Windows-1251 CP1251 KOI8-R
    QString data = codec->toUnicode(process.readLine());
    while (!data.isEmpty()) {
        QStringList list = data.split(" ");
        list.removeAll("");
        if ((list[0] == "0.0.0.0") && (list.size() > 3)) {
            GatewayData gw;
            gw.host_ = list[2];
            gw.interface_ = list[3];
            dumpGateway.append( gw );
        }

        data = codec->toUnicode(process.readLine());
    }
#elif defined Q_OS_MAC
    QString data = process.readLine();
    while (!data.isEmpty()) {
        QStringList list = data.split(" ");
        list.removeAll("");
        list.removeAll("");
        if ((list[0] == "default") && (list.size() >= 6)) {
            GatewayData gw;
            gw.host_ = list[1];
            gw.interface_ = list[5];
            dumpGateway.append( gw );
        }

        data = process.readLine();
    }
#endif

    // save gateways to settings
    QSettings settings;
    QStringList list;
    foreach (const GatewayData &data, dumpGateway)
    {
        list << QString("%1 %2").arg(data.host_).arg(data.interface_);
    }
    settings.setValue("savedGateway", list);
}

QList<KillSwitch::GatewayData> KillSwitch::loadGatewayFromSettings()
{
    QList<GatewayData> gwList;

    QSettings settings;
    QStringList list = settings.value("savedGateway", QStringList()).toStringList();
    Q_FOREACH(const QString &str, list)
    {
        QStringList ls = str.split(" ");
        if (ls.size() == 2)
        {
            GatewayData gw;
            gw.host_ = ls[0];
            gw.interface_ = ls[1];
            gwList.append(gw);
        }
        else
        {
            Q_ASSERT(0);
        }
    }

    return gwList;
}

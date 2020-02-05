#include "mmainwindow.h"
#include "SingleApplication/singleinstance.h"
#include <QApplication>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QDir>

#include "Controllers/VPNConnection.h"
#include "Controllers/ServerSide.h"
#include "Controllers/Log.h"

#ifdef Q_OS_MAC
#	include "Mac/CrashlyticsImpl.h"
#	include "Mac/Updater.h"
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef Q_OS_MAC
	SetupCrashlytics();
	checkUpdates();
#endif

    QString log;
    if(argc > 1)
        log = argv[1];

    QApplication::setApplicationName(QStringLiteral("OneVPN"));
    QApplication::setOrganizationName(QStringLiteral("OneVPN"));

    const QString serverName = QApplication::organizationName() + QApplication::applicationName();
//    SingleInstance singleInstance(serverName);
//    if (singleInstance.isOtherInstanceRunning())
//        return 1;
	Log::Controller lgCtrl;
	VPNConnection::Controller connCtrl;
	const bool isInitialized = connCtrl.initialize();
    ServerSide   ::Controller sApiCtrl;
    

    MMainWindow w;
    w.show();

    return a.exec();
}

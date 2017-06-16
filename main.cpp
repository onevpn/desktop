#include "mmainwindow.h"
#include "SingleApplication/singleinstance.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QDir>

const QString gCurrDTString = QDateTime::currentDateTime().toString(Qt::ISODate).replace(":", "-");

void customLogger(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtInfoMsg:
        txt = QString("Info: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1 (%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);
        break;
    }

    QStringList locations =  QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    if(!locations.isEmpty())
    {
        QDir().mkpath(locations.front());
        QFile outFile(QString("%1/%2.log").arg(locations.front(), gCurrDTString));
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << txt << endl;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString log;
    if(argc > 1)
        log = argv[1];

    if(log != "nolog")
        qInstallMessageHandler(customLogger);

    QApplication::setApplicationName(QStringLiteral("OneVPN"));
    QApplication::setOrganizationName(QStringLiteral("OneVPN"));

    const QString serverName = QApplication::organizationName() + QApplication::applicationName();
//    SingleInstance singleInstance(serverName);
//    if (singleInstance.isOtherInstanceRunning())
//        return 1;

    MMainWindow w;
    w.show();

    return a.exec();
}

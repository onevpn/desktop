//---------------------------------------------------------------------------
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include "log.h"

QMutex ALog::mutex_;
QWidget *ALog::parent_;

//---------------------------------------------------------------------------
void ALog::Out(QString str)
{
	QMutexLocker locker(&mutex_);

    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(dirPath);
    if (!dir.mkpath(dirPath))
    {
        return;
    }

    QString fileName = dirPath + "/log.txt";
	QFile file(fileName);

    if (!file.exists())
    {
        file.open(QFile::Text | QFile::WriteOnly);
    }
    else if(file.open(QFile::Text | QFile::Append))
	{
	}
    else
    {
        QMessageBox::information(parent_, QApplication::applicationName(),
                                 QObject::tr("Can't create log file"));
        return;
    }
    QDateTime dt = QDateTime::currentDateTime();
    file.seek(file.size());
    QTextStream out(&file);
    out << dt.toString("[hh:mm:ss] ") << str << "\n";
    file.close();
}

//---------------------------------------------------------------------------
void ALog::Clear()
{
    QString pathToLog = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/log.txt";
    QFile::remove(pathToLog);
}


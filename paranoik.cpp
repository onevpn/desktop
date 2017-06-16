#include "paranoik.h"

#include <QProcess>
#include <QTimer>
#include <QDebug>

Paranoik::Paranoik(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(30000);
    connect(m_timer, SIGNAL(timeout()), SLOT(makeWork()));
}

Paranoik::~Paranoik()
{
}

void Paranoik::start()
{
#ifdef Q_OS_LINUX
    if(!m_timer->isActive())
        m_timer->start();
#endif
}

void Paranoik::stop()
{
    m_timer->stop();
}

void Paranoik::makeWork()
{
    QString value;

    char returnData[64];
    FILE* fp = popen("ethtool eth0 | grep \"Link detected:\"", "r");
    while(fgets(returnData, 64, fp) != NULL)
    {
        value = returnData;
    }
    pclose(fp);

    if(!value.isEmpty())
    {
         QStringList exp = value.split(": ");
         if(exp.size() > 1 )
         {
             QString answer = exp[1].simplified();
             answer.replace(" ", "");
             if(answer == "no")
                 emit shouldRestart();
         }
    }
}

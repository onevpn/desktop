#include "showlog.h"
#include <QFile>

ShowLog::ShowLog(QWidget *parent,const QStringList &strs)
    : QDialog(parent)
{
	ui.setupUi(this);

    setWindowFlags(this->windowFlags() & ~Qt::WindowMaximizeButtonHint
                   & ~Qt::WindowMaximizeButtonHint & ~ Qt::WindowContextHelpButtonHint);

    Q_FOREACH(const QString &str, strs)
    {
        ui.textBrowser->append(str);
    }
}

ShowLog::~ShowLog()
{

}

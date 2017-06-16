#ifndef SHOWLOG_H
#define SHOWLOG_H

#include <QDialog>
#include "ui_showlog.h"

class ShowLog : public QDialog
{
	Q_OBJECT

public:
    ShowLog(QWidget *parent, const QStringList &strs);
    virtual ~ShowLog();

private:
	Ui::ShowLog ui;
};

#endif // SHOWLOG_H

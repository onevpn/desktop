#pragma once

#include <QDialog>
#include "Scheduler.h"

class QTextBrowser;
class QPushButton;

class LogBrowserDialog : public QDialog
{
    Q_OBJECT

public:
    LogBrowserDialog(QWidget *parent = nullptr);

public slots:
    void outputMessage(const QString &);

protected slots:
    void save();

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    void updateText();

    QString m_text;
    QTextBrowser *m_browser;
    QPushButton  *m_clearButton;
    QPushButton  *m_saveButton;
    Util::Scheduler<LogBrowserDialog> m_scheduler;
};

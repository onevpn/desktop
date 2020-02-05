#include "LogBrowserDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QDir>
#include <QFile>
#include <QTextBlock>
#include <QTextCursor>
#include <QScrollBar>

LogBrowserDialog::LogBrowserDialog(QWidget *parent)
    : QDialog(parent)
    , m_scheduler(this, &LogBrowserDialog::updateText, 100)
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    m_browser = new QTextBrowser();
    layout->addWidget(m_browser);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(buttonLayout);

    buttonLayout->addStretch(10);

    m_clearButton = new QPushButton();
    m_clearButton->setText("clear");
    buttonLayout->addWidget(m_clearButton);
    connect(m_clearButton, SIGNAL (clicked()), m_browser, SLOT (clear()));

    m_saveButton = new QPushButton();
    m_saveButton->setText("save output");
    buttonLayout->addWidget(m_clearButton);
    connect(m_saveButton, SIGNAL (clicked()), this, SLOT (save()));
    m_clearButton->hide();

    resize(800, 400);
}

void LogBrowserDialog::outputMessage(const QString &msg)
{
    m_text = msg;
    m_scheduler.schedule();
}

void LogBrowserDialog::updateText()
{
    m_browser->append(m_text);
    QScrollBar *sb = m_browser->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogBrowserDialog::save()
{
    QString saveFileName = QFileDialog::getSaveFileName(this, tr("Save Log Output"), tr("%1/logfile.txt").arg(QDir::homePath()), tr("Text Files (*.txt);;All Files (*)"));
    if(saveFileName.isEmpty())
        return;
    QFile file(saveFileName);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("Error"), QString(tr("<nobr>File '%1'<br/>cannot be opened for writing.<br/><br/>The log output could <b>not</b> be saved!</nobr>")).arg(saveFileName));
        return;
    }

    QTextStream stream(&file);
    stream << m_browser->toPlainText();
    file.close();
}

void LogBrowserDialog::keyPressEvent(QKeyEvent *e)
{
    // ignore all keyboard events
    // protects against accidentally closing of the dialog
    // without asking the user
    e->ignore();
}

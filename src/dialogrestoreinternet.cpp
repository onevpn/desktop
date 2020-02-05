#include "dialogrestoreinternet.h"
#include "ui_dialogrestoreinternet.h"

DialogRestoreInternet::DialogRestoreInternet(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogRestoreInternet)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    connect(ui->btnClose, SIGNAL(clicked(bool)), SLOT(onClickClose()));
    connect(ui->btnRestore, SIGNAL(clicked(bool)), SLOT(onClickRestore()));
    //connect(ui->btnRestoreAndReconnect, SIGNAL(clicked(bool)), SLOT(onClickRestoreAndReconnect()));
}

DialogRestoreInternet::~DialogRestoreInternet()
{
    delete ui;
}

void DialogRestoreInternet::onClickClose()
{
    result_ = CLOSE;
    accept();
}

void DialogRestoreInternet::onClickRestore()
{
    result_ = RESTORE;
    accept();
}

void DialogRestoreInternet::onClickRestoreAndReconnect()
{
    result_ = RESTORE_AND_RECONNECT;
    accept();
}

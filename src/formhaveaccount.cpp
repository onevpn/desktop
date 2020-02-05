#include "formhaveaccount.h"
#include "ui_formhaveaccount.h"

#include <QFile>
#include <QDebug>

FormHaveAccount::FormHaveAccount(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FormHaveAccount)
{
    ui->setupUi(this);

    QFile qss(":/style/FormHaveAccount.qss");
    qss.open(QIODevice::ReadOnly);
    setStyleSheet(qss.readAll());

    connect(ui->tbYes, SIGNAL(clicked(bool)), SIGNAL(yes()));
    connect(ui->tbNo, SIGNAL(clicked(bool)), SIGNAL(no()));
}

FormHaveAccount::~FormHaveAccount()
{
}

#include "formloginwaitanimation.h"
#include "ui_formloginwaitanimation.h"
#include <QMovie>

FormLoginWaitAnimation::FormLoginWaitAnimation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormLoginWaitAnimation)
{
    ui->setupUi(this);
    setFixedSize(465, 490);

    QMovie *movie = new QMovie(":/MainWindow/Images/login_loader.gif");
    ui->lblWait->setMovie(movie);
    movie->start();
}

FormLoginWaitAnimation::~FormLoginWaitAnimation()
{
    delete ui;
}

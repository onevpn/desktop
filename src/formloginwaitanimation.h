#ifndef FORMLOGINWAITANIMATION_H
#define FORMLOGINWAITANIMATION_H

#include <QWidget>

namespace Ui {
class FormLoginWaitAnimation;
}

class FormLoginWaitAnimation : public QWidget
{
    Q_OBJECT

public:
    explicit FormLoginWaitAnimation(QWidget *parent = 0);
    ~FormLoginWaitAnimation();

private:
    Ui::FormLoginWaitAnimation *ui;
};

#endif // FORMLOGINWAITANIMATION_H

#ifndef FORMHAVEACCOUNT_H
#define FORMHAVEACCOUNT_H

#include <QFrame>
#include <QScopedPointer>

namespace Ui {
class FormHaveAccount;
}

class FormHaveAccount : public QFrame
{
    Q_OBJECT

public:
    explicit FormHaveAccount(QWidget * = Q_NULLPTR);
    ~FormHaveAccount();

signals:
    void yes();
    void no();

private:
    QScopedPointer<Ui::FormHaveAccount> ui;
};

#endif // FORMHAVEACCOUNT_H

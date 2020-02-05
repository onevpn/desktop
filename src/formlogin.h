#ifndef FORMLOGIN_H
#define FORMLOGIN_H

#include <QFrame>
#include <QScopedPointer>
#include <QPointer>

namespace Ui {
class FormLogin;
}

class QMovie;

class FormLogin : public QFrame
{
    Q_OBJECT

public:
    explicit FormLogin(QWidget * = Q_NULLPTR);
    ~FormLogin();

    void setErrorMessage(const QString &);
	void writeSettings();
    void gotoWaiting();
    void gotoFilling();
protected:
    void showEvent(QShowEvent*);

signals:
    void acceptClicked(QString const& username, QString const& password);

//    /*DOWN*/void acceptClicked();
private slots:
    void checkAccesibility();
    void signUp();
    void forgot();
    void doAccept();

public:
    QScopedPointer<Ui::FormLogin> ui;
    QPointer<QMovie> m_movie;
};

#endif // FORMLOGIN_H

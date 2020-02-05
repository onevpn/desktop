#include "formlogin.h"
#include "ui_formlogin.h"
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QStyle>
#include <QMovie>

#include <QDebug>

static const QString LAST_LOGIN   (QStringLiteral("saved_login"));
static const QString LAST_PASSWORD(QStringLiteral("saved_password"));
static const QString SAVE_PASSWORD(QStringLiteral("save_password"));
static const QString SAVE_USERNAME(QStringLiteral("save_username"));

FormLogin::FormLogin(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FormLogin)
{
    ui->setupUi(this);
    connect(ui->tbAccept, SIGNAL(clicked(bool)),         SLOT(doAccept()));
    connect(ui->tbSignUp, SIGNAL(clicked(bool)),         SLOT(signUp()));
    connect(ui->tbForgotPassword, SIGNAL(clicked(bool)), SLOT(forgot()));

    connect(ui->edUsername, SIGNAL(textEdited(QString)), this, SLOT(checkAccesibility()));
    connect(ui->edPassword, SIGNAL(textChanged(QString)), this, SLOT(checkAccesibility()));

#ifdef Q_OS_MAC
    ui->edUsername->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->edPassword->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    QPalette p = ui->edPassword->palette();
    p.setColor(QPalette::Mid, Qt::red);
    ui->edPassword->setPalette(p);
    ui->edUsername->setPalette(p);

    ui->failLabel->clear();
    ui->failLine->hide();
}

FormLogin::~FormLogin()
{
}

void FormLogin::signUp()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://onevpn.co/members/signup.php")));
}

void FormLogin::forgot()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://onevpn.co/members/forgot.php")));
}

void FormLogin::doAccept()
{
    emit acceptClicked(ui->edUsername->text(), ui->edPassword->text());
}

void FormLogin::writeSettings()
{
	QSettings settings;
	const bool savePassword = ui->savePassword->isChecked();
	const bool saveUsername = ui->saveUsername->isChecked();
	settings.setValue(SAVE_PASSWORD, savePassword);
	settings.setValue(SAVE_USERNAME, saveUsername);
	settings.setValue(LAST_PASSWORD, savePassword ? ui->edPassword->text() : "");
	settings.setValue(LAST_LOGIN   , saveUsername ? ui->edUsername->text() : "");
}

void FormLogin::showEvent(QShowEvent* event)
{
	QSettings settings;
	ui->edPassword->setText(settings.value(LAST_PASSWORD, "").toString());
	ui->edUsername->setText(settings.value(LAST_LOGIN   , "").toString());

	ui->savePassword->setChecked(settings.value(SAVE_PASSWORD, false).toBool());
	ui->saveUsername->setChecked(settings.value(SAVE_USERNAME, false).toBool());

	QWidget::showEvent(event);
}

void FormLogin::setErrorMessage(const QString &msg)
{
    if (!msg.isEmpty())
    {
        ui->failLabel->setText(msg);
        ui->failLine->show();
    }
    else
    {
        ui->failLabel->clear();
        ui->failLine->hide();
    }
}

void FormLogin::checkAccesibility()
{
    const bool username = !ui->edUsername->text().isEmpty();
    const bool password = !ui->edPassword->text().isEmpty();

    ui->tbAccept->setProperty("styleHint", username && password ? "accessible" : "");
    style()->unpolish(ui->tbAccept);
    style()->polish(ui->tbAccept);
}

void FormLogin::gotoWaiting()
{
    ui->failLabel->clear();
    ui->failLine->hide();
    ui->stackedWidget->setCurrentWidget(ui->waiting);
    if(!m_movie)
    {
        m_movie = new QMovie(QStringLiteral(":/FormLogin/load.gif"), QByteArray(), this);
        ui->waitLabel->setMovie(m_movie);
    }
    m_movie->start();
}

void FormLogin::gotoFilling()
{
    if(m_movie)
        m_movie->stop();
    ui->stackedWidget->setCurrentWidget(ui->invitation);
}

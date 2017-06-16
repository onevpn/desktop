#include "connectwindow.h"
#include "log.h"
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include "mainwindow.h"
#include "showlog.h"
#include "downloadfile.h"
#include <QStandardPaths>

ConnectWindow::ConnectWindow(QWidget *parent)
	: QWidget(parent), state_(STATE_DISCONNECTED), connection_(this)
{
	ui.setupUi(this);

    mainWindow_ = (MainWindow *)parent;
    ALog::parent_ = this;

    // delete log file
    ALog::Clear();

    if (!connection_.initialize())
	{
		QMessageBox::information(this, QApplication::applicationName(), tr("OpenVPN initialize error"));
		QTimer::singleShot(1, parent, SLOT(close()));
		return;
	}

	if (!connection_.tapInstalled())
	{
		QMessageBox::question(this, QApplication::applicationName(), tr("TAP drivers not installed."));
		QTimer::singleShot(1, this, SLOT(close()));
		return;
	}

	connect(ui.btnConnect, SIGNAL(clicked()), SLOT(onClickConnect()));

	connect(&connection_, SIGNAL(connected()), SLOT(onConnected()));
	connect(&connection_, SIGNAL(disconnected(bool)), SLOT(onDisconnected(bool)));
	connect(&connection_, SIGNAL(error(QString)), SLOT(onConnectionError(QString)));
	connect(&connection_, SIGNAL(statisticsChanged(double, double)), SLOT(onStatisticsChanged(double, double)));

	connect(ui.cbServer, SIGNAL(currentIndexChanged(const QString &)), SLOT(onServerChanged(const QString &)));
    connect(ui.cbCountry, SIGNAL(currentIndexChanged(const QString &)), SLOT(onCountryChanged(const QString &)));
    connect(ui.chShowPass, SIGNAL(stateChanged(int)), SLOT(onShowPasssword(int)));

    connect(ui.lbForgotPass, SIGNAL(linkActivated(const QString &)),
            SLOT(onForgotLink(const QString &)));
    connect(ui.lbFaq, SIGNAL(linkActivated(const QString &)),
            SLOT(onFaqLink(const QString &)));
}

ConnectWindow::~ConnectWindow()
{
	connection_.disconnect(false);
}


void ConnectWindow::onClickConnect()
{
    QString login;
    QString password;

    if ((ui.edLogin->text().isEmpty() || ui.edPassword->text().isEmpty())
        && (state_ == STATE_DISCONNECTED)) {
        QMessageBox::information(this, QApplication::applicationName(),
                                 tr("You must enter username and password to sign in"));
        return;
    }


	if (state_ == STATE_DISCONNECTED)
	{
        QSettings settings;
        if (ui.cbSavePassword->isChecked()) {
            settings.setValue("login", ui.edLogin->text());
            settings.setValue("password", ui.edPassword->text());
        }

        login = ui.edLogin->text();
        password = ui.edPassword->text();

        settings.setValue("savePass", ui.cbSavePassword->isChecked());

        ALog::Out("Start connection");
		if (connection_.isConnected())
		{
            ALog::Out("connection_.isConnected()");
			return;
		}

        PROTOCOL_TYPE protocol;
		if (ui.cbProtocol->currentText() == tr("PPTP"))
		{
			protocol = PPTP;
		}
		else if (ui.cbProtocol->currentText() == tr("L2TP"))
		{
			protocol = L2TP;
		}
		else
		{
			protocol = OpenVPN;
            ServerInfo si = servers_[ui.cbServer->currentText()];
            makeOVPNFile(settings.value("openVPNPort", "UDP 1194").toString(), servers_[ui.cbServer->currentText()].ip_, configOVPN_);
        }

        ui.btnConnect->setText("Connecting...");
        setEnabled(false);

		state_ = STATE_CONNECTING;
		timerImageNum_ = 1;
         connection_.connect(protocol, servers_[ui.cbServer->currentText()].ip_, login, password, QString(), servers_[ui.cbServer->currentText()].l2tpKey_, servers_[ui.cbServer->currentText()].dns_);
	}
	else if (state_ == STATE_CONNECTED)
    {
		connection_.disconnect(true);
	}
	else if (state_ == STATE_CONNECTING)
	{
		connection_.disconnect(true);
	}
}

void ConnectWindow::onConnected()
{	
    ui.btnConnect->setText("Disconnect");
    ui.lbStatus->setText("VPN Connected");
	state_ = STATE_CONNECTED;    
    emit connected();
}

void ConnectWindow::onDisconnected(bool withError)
{	
    setEnabled(true);
    ui.btnConnect->setText("Connect");
    ui.lbStatus->setText("VPN Disconnected");

	state_ = STATE_DISCONNECTED;

	if (withError)
	{

		QSettings settings;
		if (settings.value("reconnectAutomatically").toString() == "true")
		{
			onClickConnect();
        }
	}   
}

void ConnectWindow::onConnectionError(QString msg)
{
    if (!msg.isEmpty())
    {
        ALog::Out("onConnectionError()");
        QMessageBox::information(this, QApplication::applicationName(), msg);
    }

    setEnabled(true);
    ui.btnConnect->setText("Connect");
    ui.lbStatus->setText("VPN Disconnected");
    state_ = STATE_DISCONNECTED;    
}

void ConnectWindow::setEnabled(bool enabled)
{
    ui.edLogin->setEnabled( enabled );
    ui.edPassword->setEnabled( enabled );
    ui.chShowPass->setEnabled( enabled );
    ui.cbSavePassword->setEnabled( enabled );
    ui.cbCountry->setEnabled( enabled );
    ui.cbProtocol->setEnabled( enabled );
    ui.cbServer->setEnabled( enabled );
}

void ConnectWindow::setServers(const QVector<ServerInfo> &servers)
{
    QStringList countryes;

	ui.cbServer->clear();
	servers_.clear();
	foreach(const ServerInfo &si, servers)
	{
		servers_[si.description_] = si;
        if (!countryes.contains(si.coutry_))
            countryes <<si.coutry_;
	}

    ui.cbCountry->clear();
    foreach (QString contry, countryes) {
        ui.cbCountry->addItem(contry);
    }
    ui.cbCountry->setCurrentIndex(0);
}

void ConnectWindow::onCountryChanged(const QString &country)
{
    ui.cbServer->clear();
    for (QMap<QString, ServerInfo>::iterator it = servers_.begin(); it != servers_.end(); ++it)
    {
        if (country == (it.value()).coutry_)
            ui.cbServer->addItem(it.key());
    }

    ui.cbServer->setCurrentIndex(0);
}

void ConnectWindow::onServerChanged(const QString &server)
{
	QString curProtocol = ui.cbProtocol->currentText();

	ui.cbProtocol->clear();
	const ServerInfo &si = servers_[server];
	if (si.pptp_)
	{
		ui.cbProtocol->addItem("PPTP");
	}
	if (si.l2tp_)
	{
		ui.cbProtocol->addItem("L2TP");
    }
	if (si.openvpn_)
	{
		ui.cbProtocol->addItem("OpenVPN");
	}

	int ind = ui.cbProtocol->findText(curProtocol);
	if (ind != -1)
		ui.cbProtocol->setCurrentIndex(ind);
	else
		ui.cbProtocol->setCurrentIndex(0);
}

void ConnectWindow::makeOVPNFile(QString protocolPort, QString server, QByteArray configData)
{
    QString appPath = QApplication::applicationDirPath();

#if defined Q_OS_WIN
    QString OVPNPath = appPath + "/config.ovpn";
#elif defined Q_OS_MAC
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(dirPath);
    if (!dir.mkpath(dirPath))
    {
        QMessageBox::information(this, QApplication::applicationName(),
                                 tr("Can't create path ") + dirPath);
        return;
    }
    QString OVPNPath = dirPath + "/config.ovpn";
#endif
    QString OVPNorig = ":/MainWindow/ovpnconfs/config.ovpn";

    QStringList strs = protocolPort.split(" ");
    QStringList ports;
    QString protocol;
    if (strs.count() == 2) {
        protocol = strs[0];

        ports = strs[1].split(",");
    } else {
        Q_ASSERT(false);
        return;
    }

    QFile configOrigFile(OVPNorig);
    if (!configOrigFile.open(QIODevice::ReadOnly)){
        Q_ASSERT(false);
        return;
    }

    QFile configFile(OVPNPath);
    if (configFile.open(QIODevice::WriteOnly))
    {
        configFile.write( configOrigFile.readAll() );
        // protocol
        if (protocol.contains("TCP", Qt::CaseInsensitive)) {
            configFile.write("proto tcp\r\n");
        } else {
            configFile.write("proto udp\r\n");
        }

        // server
        QString str;
        QStringList::const_iterator constIterator;
        for (constIterator = ports.constBegin(); constIterator != ports.constEnd(); ++constIterator)
            str += "remote " + server + " " +(*constIterator).toLocal8Bit().constData() +"\r\n";
        configFile.write(str.toLocal8Bit());

        configFile.close();
        configOrigFile.close();
    }
    else
    {
        QMessageBox::information(this, QApplication::applicationName(),
                                 tr("Can't create config.ovpn file ") + OVPNPath);
    }
}


void ConnectWindow::onStatisticsChanged(double download, double upload)
{

}

void ConnectWindow::onShowPasssword(int state)
{
    if (state!=0)
        ui.edPassword->setEchoMode( QLineEdit::Normal );
    else
        ui.edPassword->setEchoMode( QLineEdit::Password );
}

void ConnectWindow::onForgotLink(const QString &str)
{
    QString link = "https://vpnvantage.com/signup";
    QDesktopServices::openUrl(QUrl(link));
}

void ConnectWindow::onFaqLink(const QString &str)
{
    QString link = "http://vpnvantage.com/faq";
    QDesktopServices::openUrl(QUrl(link));
}

void ConnectWindow::saveSettings()
{
	QSettings settings;
	settings.setValue("Server", ui.cbServer->currentText());
	settings.setValue("Protocol", ui.cbProtocol->currentText());
}

void ConnectWindow::loadSettings()
{
	QSettings settings;
	QString server = settings.value("Server").toString();
	QString protocol = settings.value("Protocol").toString();

	int ind = ui.cbServer->findText(server);
	if (ind != -1)
		ui.cbServer->setCurrentIndex(ind);

	ind = ui.cbProtocol->findText(protocol);
	if (ind != -1)
		ui.cbProtocol->setCurrentIndex(ind);

    if (settings.value("savePass", "true").toString() == "true")
    {
        QString username = settings.value("login", "").toString();
        QString password = settings.value("password", "").toString();
        ui.edLogin->setText( username );
        ui.edPassword->setText( password );
        ui.cbSavePassword->setChecked( true );
        ui.chShowPass->setChecked( false );
    }
}

void ConnectWindow::autoConnect()
{
	onClickConnect();
}

bool ConnectWindow::isConnected()
{
    return (state_ == STATE_CONNECTED);
}

void ConnectWindow::onShowLog()
{
    QScopedPointer<ShowLog> showLog(new ShowLog(this));
    showLog->exec();
}


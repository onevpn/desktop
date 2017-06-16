#ifndef CONNECTWINDOW_H
#define CONNECTWINDOW_H

#include <QWidget>
#include <QItemDelegate>
#include "ui_connectwindow.h"

#if defined Q_OS_WIN
#include "Windows/connection.h"
#elif defined Q_OS_MAC
#include "Mac/connection.h"
#elif defined Q_OS_UNIX
#include "Unix/connection.h"
#endif

#include "getservers.h"

class MainWindow;

class RowHeightDelegate : public QItemDelegate
{
	Q_OBJECT
public:
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		return QSize(1, 40);
	}
};

class ConnectWindow : public QWidget
{
	Q_OBJECT
public:
	ConnectWindow(QWidget *parent = 0);
	~ConnectWindow();

    void setServers(const QVector<ServerInfo> &servers);
    void setOVPNConfig(QByteArray data) { configOVPN_ = data; }
	void loadSettings();
	void saveSettings();
	void autoConnect();
    bool isConnected();

signals:
	void settings();
    void connected();

private slots:
	void onClickConnect();
	void onConnected();
	void onDisconnected(bool withError);
	void onConnectionError(QString msg);
	void onServerChanged(const QString & server);
    void onCountryChanged(const QString & country);
	void onStatisticsChanged(double download, double upload);
    void onShowLog();
    void onShowPasssword(int state);
    void onForgotLink(const QString &str);
    void onFaqLink(const QString &str);

private:
	Ui::ConnectWindow ui;
	enum {STATE_CONNECTED, STATE_DISCONNECTED, STATE_CONNECTING};
	int state_;
	Connection connection_;
	QMap<QString, ServerInfo>  servers_;
    QByteArray configOVPN_;
	int timerImageNum_;

    MainWindow *mainWindow_;

	void setEnabled(bool enabled);
    void makeOVPNFile(QString protocolPort, QString server, QByteArray configData);
};
#endif // CONNECTWINDOW_H

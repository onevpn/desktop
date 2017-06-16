#ifndef CONNECTION_H
#define CONNECTION_H

#include "../utils.h"
#include "openvpnconnectorqt.h"

class Connection : public QObject
{
	Q_OBJECT

public:
	Connection(QObject *parent);
	~Connection();

	bool initialize();
    bool connect(PROTOCOL protocol, QString serverIP, QString username, QString password, QString ovpnFile, QString l2tpKey, QStringList dns);
    void disconnect();
	bool tapInstalled();

    bool isConnected() const { return m_isConnected; }
	
signals:
	void connected();
    void disconnected();
    void error(const QString &error);
    void log(const QString &logStr);
    void statisticsChanged(quint64 download, quint64 upload);

protected:
    //void customEvent(QEvent *event);
    //void timerEvent(QTimerEvent *event);

private slots:
    void onOpenVPNConnected();
    void onOpenVPNDisconnected();
    void onOpenVPNError(OPENVPN_ERROR err);
    void onOpenVPNLog(const QString &logStr);

private:
	enum {UM_CONNECTED = 1001, UM_ERROR = 1002};
    bool m_isConnected;

    /*HRASCONN connHandle_;
	int	timerId_;
	bool bFirstCalcStat_;
	DWORD prevBytesRcved_;
	DWORD prevBytesXmited_;

	static const wchar_t *g_szConnName; 
    static Connection *g_Connection;
	static QString g_str;
    static void CALLBACK rasDialFunc(UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError);*/
};

#endif // CONNECTION_H

#ifndef CONNECTION_H
#define CONNECTION_H

#include "utils.h"
#include "openvpnconnectorqt.h"
#include "Interfaces/AbstractConnection.h"

class Connection : public AbstractConnection
{
	Q_OBJECT

public:
    Connection(QObject *parent = nullptr);
	~Connection();

    bool initialize() override;
    bool connect(PROTOCOL protocol
                 , QString serverIP
                 , QString username
                 , QString password
                 , QString ovpnFile
                 , QString l2tpKey) override;

    void disconnect() override;
    bool tapInstalled() override;
    bool isConnected() const override { return m_isConnected; }
	
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
    bool m_isConnected = false;

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

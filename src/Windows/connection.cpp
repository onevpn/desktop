#include "connection.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

const wchar_t *Connection::g_szConnName = L"OneVPN";
Connection *Connection::g_Connection = NULL;
QString Connection::g_str;

OpenVPNConnectorQt *g_openVPNConnection = NULL;

#pragma comment(lib, "rasapi32.lib")

Connection::Connection(QObject *parent)
    : AbstractConnection(parent), connHandle_(NULL)
{
	g_Connection = this;

    qRegisterMetaType<OPENVPN_ERROR>("OPENVPN_ERROR");

    g_openVPNConnection = new OpenVPNConnectorQt(this);

    QObject::connect(g_openVPNConnection, SIGNAL(connected()), SLOT(onOpenVPNConnected()));
    QObject::connect(g_openVPNConnection, SIGNAL(disconnected()), SLOT(onOpenVPNDisconnected()));
    QObject::connect(g_openVPNConnection, SIGNAL(error(OPENVPN_ERROR)), SLOT(onOpenVPNError(OPENVPN_ERROR)));
    QObject::connect(g_openVPNConnection, SIGNAL(log(QString)), SLOT(onOpenVPNLog(QString)));
    QObject::connect(g_openVPNConnection, SIGNAL(statisticsUpdated(quint64,quint64)), SIGNAL(statisticsChanged(quint64,quint64)));
}

Connection::~Connection()
{
    delete g_openVPNConnection;
}

void CALLBACK Connection::rasDialFunc( UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError )
{
	if (rasconnstate == RASCS_OpenPort) {
		g_str = "OpenPort";
	}
	else if (rasconnstate == RASCS_PortOpened) {
		g_str = "PortOpened";
	}
	else if (rasconnstate == RASCS_ConnectDevice) {
		g_str = "ConnectDevice";
	}
	else if (rasconnstate == RASCS_DeviceConnected) {
		g_str = "DeviceConnected";
	}
	else if (rasconnstate == RASCS_AllDevicesConnected) {
		g_str = "AllDevicesConnected";
	}
	else if (rasconnstate == RASCS_Authenticate) {
		g_str = "Authenticate";
	}
	else if (rasconnstate == RASCS_AuthNotify) {
		g_str = "AuthNotify";
	}
	else if (rasconnstate == RASCS_AuthRetry) {
		g_str = "AuthRetry";
	}
	else if (rasconnstate == RASCS_AuthCallback) {
		g_str = "AuthCallback";
	}
	else if (rasconnstate == RASCS_AuthChangePassword) {
		g_str = "AuthChangePassword";
	}
	else if (rasconnstate == RASCS_AuthProject) {
		g_str = "AuthProject";
	}
	else if (rasconnstate == RASCS_AuthLinkSpeed) {
		g_str = "AuthLinkSpeed";
	}
	else if (rasconnstate == RASCS_AuthAck) {
		g_str = "AuthAck";
	}
	else if (rasconnstate == RASCS_ReAuthenticate) {
		g_str = "ReAuthenticate";
	}
	else if (rasconnstate == RASCS_Authenticated) {
		g_str = "Authenticated";
	}
	else if (rasconnstate == RASCS_PrepareForCallback) {
		g_str = "PrepareForCallback";
	}
	else if (rasconnstate == RASCS_WaitForModemReset) {
		g_str = "WaitForModemReset";
	}
	else if (rasconnstate == RASCS_WaitForCallback) {
		g_str = "WaitForCallback";
	}
	else if (rasconnstate == RASCS_Projected) {
		g_str = "Projected";
	}
	else if (rasconnstate == RASCS_StartAuthentication) {
		g_str = "StartAuthentication";
	}
	else if (rasconnstate == RASCS_CallbackComplete) {
		g_str = "CallbackComplete";
	}
	else if (rasconnstate == RASCS_LogonNetwork) {
		g_str = "LogonNetwork";
	}
	else if (rasconnstate == RASCS_SubEntryConnected) {
		g_str = "SubEntryConnected";
	}
	else if (rasconnstate == RASCS_SubEntryDisconnected) {
		g_str = "SubEntryDisconnected";
	}
	else if (rasconnstate == RASCS_Interactive) {
		g_str = "Interactive";
	}
	else if (rasconnstate == RASCS_RetryAuthentication) {
		g_str = "RetryAuthentication";
	}
	else if (rasconnstate == RASCS_CallbackSetByCaller) {
		g_str = "CallbackSetByCaller";
	}
	else if (rasconnstate == RASCS_PasswordExpired) {
		g_str = "PasswordExpired";
	}
	else if (rasconnstate == RASCS_InvokeEapUI) {
		g_str = "InvokeEapUI";
	}
	else if (rasconnstate == RASCS_Connected) {
		g_str = "Connected";
		if (dwError == 0)
		{
			QApplication::postEvent(g_Connection,new QEvent(QEvent::Type(UM_CONNECTED)));
		}
	}
	else if (rasconnstate == RASCS_Disconnected) {
		g_str = "Disconnected";
	}

	if (dwError == 0) {
		g_str += ": Ok";
	}
	else {
		wchar_t strErr[1024];
		RasGetErrorString(dwError, strErr, 1024);
		g_str += ": " + QString::fromWCharArray(strErr);

		QApplication::postEvent(g_Connection,new QEvent(QEvent::Type(UM_ERROR)));
	}

    emit g_Connection->log(g_str);
}

void Connection::customEvent( QEvent *event )
{
	if (event->type() == (QEvent::Type)UM_CONNECTED)
	{
		emit connected();
		bFirstCalcStat_ = true;
		prevBytesRcved_ = 0;
		prevBytesXmited_ = 0;
		timerId_ = startTimer(1000);
	}
	else if (event->type() == (QEvent::Type)UM_ERROR)
	{
        Connection::disconnect();
		emit error(g_str);
	}
}

bool Connection::connect( PROTOCOL protocol, QString serverIP, QString username, QString password , QString ovpnFile, QString l2tpKey)
{
    if (protocol == OPENVPN)
	{
        emit log("Do connect to OpenVPN");
        g_openVPNConnection->connect(ovpnFile, username, password, "", "");
        return true;
    }

	RASDEVINFO *devinfo;
	DWORD dwSize;
	DWORD dwNumberDevices = 0;

	dwSize = 0;

	RasEnumDevices(0, &dwSize, &dwNumberDevices);
	devinfo = (RASDEVINFO*)malloc(dwSize);
	devinfo[0].dwSize = sizeof(RASDEVINFO);

	DWORD err = RasEnumDevices(devinfo, &dwSize, &dwNumberDevices);
	if (err != 0) 
	{
		QString errMsg = "RasEnumDevices return error code: " + err;
        emit log(errMsg);
		free(devinfo);
		emit error("RasEnumDevices return error code");
        return false;
	}

	int DeviceInd = -1;
	QString selProtocol;
	if (protocol == PPTP)
	{
		selProtocol = "pptp";
        emit log("Do connect to PPTP");
	}
    else if (protocol == L2TP)
	{
		selProtocol = "l2tp";
        emit log("Do connect to L2TP");
	}
    else if (protocol == SSTP)
    {
        selProtocol = "sstp";
        emit log("Do connect to SSTP");
    }

	for (DWORD i = 0; i < dwNumberDevices; i++) 
	{
		QString devtype = QString::fromUtf16((const ushort *)devinfo[i].szDeviceType);
		QString devname = QString::fromUtf16((const ushort *)devinfo[i].szDeviceName);
		devtype = devtype.toLower();
		devname = devname.toLower();

		if (devtype.contains("vpn") && devname.contains(selProtocol)) 
		{
			DeviceInd = i;
            break;
		}
	}

	if (DeviceInd == -1) 
	{
        emit log("Unable to find device with VPN PPTP");
		free(devinfo);
		emit error("Unable to find device with VPN PPTP");
        return false;
	}

	RASENTRY re;
	DWORD dwEntrySize;

	memset(&re, 0, sizeof(re));
	re.dwSize = sizeof(re);
	dwEntrySize = sizeof(re);

    wcscpy(re.szLocalPhoneNumber, (wchar_t*)serverIP.utf16());
	wcscpy(re.szDeviceName, devinfo[DeviceInd].szDeviceName);
	wcscpy(re.szDeviceType, devinfo[DeviceInd].szDeviceType);

	if (selProtocol == "pptp")
	{
		re.dwfOptions = 0x20011C10;
		re.dwFramingProtocol = RASFP_Ppp;
		re.dwType = RASET_Vpn;
		re.dwVpnStrategy = VS_PptpOnly;
		re.dwfNetProtocols = RASNP_Ip;
		re.dwEncryptionType = ET_RequireMax;
		re.dwRedialCount = 99;
		re.dwRedialPause = 60;
		re.dwfOptions2 = 263;
	}
	else if (selProtocol == "l2tp")
	{
		re.dwfOptions = 0x20011C10;
		re.dwFramingProtocol = RASFP_Ppp;
		re.dwType = RASET_Vpn;
		re.dwVpnStrategy = VS_L2tpOnly;
		re.dwfNetProtocols = RASNP_Ip;
        re.dwEncryptionType = ET_RequireMax;
		re.dwRedialCount = 99;
		re.dwRedialPause = 60;
        re.dwfOptions2 = 279;
	}
    else if (selProtocol == "sstp")
    {
        re.dwfOptions = 0x20011C10;
        re.dwFramingProtocol = RASFP_Ppp;
        re.dwType = RASET_Vpn;
        re.dwVpnStrategy = 5;//VS_SstpOnly;
        re.dwfNetProtocols = RASNP_Ip;
        re.dwEncryptionType = ET_RequireMax;
        re.dwRedialCount = 99;
        re.dwRedialPause = 60;
        re.dwfOptions2 = 263;
    }

	
	err = RasSetEntryProperties(NULL, g_szConnName, &re, dwEntrySize, NULL, NULL);
	if (err != 0) 
	{
		free(devinfo);
		emit error("RasSetEntryProperties return error code");
        return false;
	}

	if (selProtocol == "l2tp")
	{
		RASCREDENTIALS ras_cre_psk = {0};
		ras_cre_psk.dwSize = sizeof(ras_cre_psk);
        ras_cre_psk.dwMask = RASCM_PreSharedKey; //0x00000010; //RASCM_PreSharedKey;
		wcscpy(ras_cre_psk.szPassword, (wchar_t *)l2tpKey.utf16());
        err = RasSetCredentials(NULL, g_szConnName, &ras_cre_psk, FALSE);        
        if (err != 0)
        {
            free(devinfo);
            emit error("RasSetCredentials return error code");
            return false;
        }
	}

	RASDIALPARAMS dialparams;

	memset(&dialparams, 0, sizeof(dialparams));
	dialparams.dwSize = sizeof(dialparams);
	wcscpy(dialparams.szEntryName, g_szConnName);

    wcscpy(dialparams.szUserName, (wchar_t *)username.utf16());
    wcscpy(dialparams.szPassword, (wchar_t *)password.utf16());

	err = RasSetEntryDialParams(NULL, &dialparams, FALSE);
	if (err != 0) 
	{
		QString errMsg = "RasSetEntryDialParams return error code: " + err;
        emit log(errMsg);
		emit error("RasSetEntryDialParams return error code");
        return false;
	}

	// Connect
    err = RasDial(NULL, NULL, &dialparams, 0, (void *)rasDialFunc, &connHandle_);
	if (err != 0) 
	{
		QString errMsg = "RasDial return error code: " + err;
        emit log(errMsg);
		emit error("RasDial return error code");
		connHandle_ = NULL;
        return false;
	}

	free(devinfo);
    return true;
}

void Connection::disconnect()
{
	if (connHandle_) 
	{
		DWORD err = RasHangUp(connHandle_);

		err = 0;
		while (err != ERROR_INVALID_HANDLE) {
			RASCONNSTATUS status;
			memset(&status, 0, sizeof(status));
			status.dwSize = sizeof(status);
			err = RasGetConnectStatus(connHandle_, &status);
			Sleep(1);
		}
		killTimer(timerId_);
		connHandle_ = NULL;
        emit disconnected();
	}
    g_openVPNConnection->disconnect();
}

void Connection::timerEvent( QTimerEvent *event )
{
	if (connHandle_) 
	{
		RASCONNSTATUS status;
		memset(&status, 0, sizeof(status));
		status.dwSize = sizeof(status);
		DWORD err = RasGetConnectStatus(connHandle_, &status);
		if (err == ERROR_INVALID_HANDLE)
		{
			connHandle_ = NULL;
			killTimer(timerId_);
            emit disconnected();
		}
		else
		{
			RAS_STATS stats;
			stats.dwSize = sizeof(RAS_STATS);
			if (RasGetLinkStatistics(connHandle_, 1, &stats) == ERROR_SUCCESS)
			{
                emit statisticsChanged(stats.dwBytesRcved, stats.dwBytesXmited);
			}
		}
    }
}

void Connection::onOpenVPNConnected()
{
    m_isConnected = true;
    emit connected();
}

void Connection::onOpenVPNDisconnected()
{
    m_isConnected = false;
    emit disconnected();
}

void Connection::onOpenVPNError(OPENVPN_ERROR err)
{
    if (err == AUTH_ERROR)
    {
        emit error(tr("Incorrect username or password"));
    }
    else if (err == CANNOT_ALLOCATE_TUN_TAP)
    {
        emit error(tr("There are no TAP-Windows adapters on this system. Please install."));
    }
    else
    {
        emit error(tr("Error during connection (") + QString::number(err) + tr(")"));
    }
}

void Connection::onOpenVPNLog(const QString &logStr)
{
    emit log(logStr);
}

bool Connection::initialize()
{
    return g_openVPNConnection->installHelper("OneVPNService");
}

bool Connection::tapInstalled()
{
    return true;
}

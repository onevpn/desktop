#include <windows.h>
#include <vector>
#include <string>

#define SERVICE_NAME  ("OneVPNService")
#define SERVICE_PIPE_NAME  ("\\\\.\\pipe\\OneVPNService")

SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI serviceMain (DWORD argc, LPTSTR *argv);
VOID WINAPI serviceCtrlHandler (DWORD);
DWORD WINAPI serviceWorkerThread (LPVOID lpParam);

#pragma pack(push, 1)
struct MessagePacket
{
    bool blocking;
    wchar_t szCommandLine[MAX_PATH*3];
};

struct MessagePacketResult
{
    bool success;
    DWORD exitCode;
};
#pragma pack(pop)

static std::string utf16ToUTF8(const std::wstring &s)
{
	const int size = ::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, NULL, 0, 0, NULL);

	std::vector<char> buf(size);
	::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, &buf[0], size, 0, NULL);

	return std::string(&buf[0]);
}

int main(int argc, char *argv[])
{
    SERVICE_TABLE_ENTRY serviceTable[] =
    {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) serviceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(serviceTable) == FALSE)
    {
        return GetLastError ();
    }

    return 0;
}

VOID WINAPI serviceMain (DWORD argc, LPTSTR *argv)
{
    DWORD status = E_FAIL;

    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler (SERVICE_NAME, serviceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        goto EXIT;
    }

    // Tell the service controller we are starting
    ZeroMemory (&g_ServiceStatus, sizeof (g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle , &g_ServiceStatus) == FALSE)
    {
        //OutputDebugString(_T(
        //  "My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    //
    // Perform tasks necessary to start the service here
    //

    // Create a service stop event to wait on later
    g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        // Error creating event
        // Tell service controller we are stopped and exit
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            //OutputDebugString(_T(
            //"My Sample Service: ServiceMain: SetServiceStatus returned error"));
        }
        goto EXIT;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        //OutputDebugString(_T(
          //"My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    // Start a thread that will perform the main task of the service
    HANDLE hThread = CreateThread (NULL, 0, serviceWorkerThread, NULL, 0, NULL);

    // Wait until our worker thread exits signaling that the service needs to stop
    WaitForSingleObject (hThread, INFINITE);


    //
    // Perform any cleanup tasks
    //

    CloseHandle (g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        //OutputDebugString(_T(
          //"My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

EXIT:
    return;
}

VOID WINAPI serviceCtrlHandler (DWORD CtrlCode)
{
    switch (CtrlCode)
    {
     case SERVICE_CONTROL_STOP :

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
           break;

        //
        // Perform tasks necessary to stop the service here
        //

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            //OutputDebugString(_T(
              //"My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent (g_ServiceStopEvent);

        break;

     default:
         break;
    }
}

HANDLE сreatePipe()
{
    SECURITY_ATTRIBUTES sa;
    sa.lpSecurityDescriptor =
      (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (!InitializeSecurityDescriptor(sa.lpSecurityDescriptor,
         SECURITY_DESCRIPTOR_REVISION))
    {
        return INVALID_HANDLE_VALUE;
    }
    if (!SetSecurityDescriptorDacl(sa.lpSecurityDescriptor,
                                   TRUE, (PACL)0, FALSE))
    {
        return INVALID_HANDLE_VALUE;
    }
    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;

    HANDLE hPipe = ::CreateNamedPipe(SERVICE_PIPE_NAME,
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE | PIPE_WAIT,
                    PIPE_UNLIMITED_INSTANCES, sizeof(MessagePacket),
                    sizeof(MessagePacket), NMPWAIT_USE_DEFAULT_WAIT, &sa);
    return hPipe;
}

DWORD WINAPI serviceWorkerThread (LPVOID lpParam)
{
    HANDLE hPipe = сreatePipe();
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (hEvent == NULL)
    {
        return 0;
    }
    OVERLAPPED overlapped;
    overlapped.hEvent = hEvent;

    HANDLE hEvents[2];

    hEvents[0] = g_ServiceStopEvent;
    hEvents[1] = hEvent;

    while (true)
    {
        ::ConnectNamedPipe(hPipe, &overlapped);

        DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (dwWait == WAIT_OBJECT_0)
        {
            break;
        }
        else if (dwWait == (WAIT_OBJECT_0 + 1))
        {
            MessagePacket packet;
            DWORD read = 0;

            if (!(::ReadFile(hPipe, &packet, sizeof(packet), &read, 0)))
            {
                unsigned int error = GetLastError();
            }
            else
            {
                // The processing of the received data
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                si.cb = sizeof(si);
                ZeroMemory( &pi, sizeof(pi) );

				std::string szCommandLine(utf16ToUTF8(packet.szCommandLine));
				LPSTR lpStr = (LPSTR)szCommandLine.c_str();
                if (CreateProcess(NULL, lpStr, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                {
                    DWORD exitCode;
                    if (packet.blocking)
                    {
                        WaitForSingleObject(pi.hProcess, INFINITE);
                        GetExitCodeProcess(pi.hProcess, &exitCode);
                    }

                    CloseHandle( pi.hProcess );
                    CloseHandle( pi.hThread );

                    MessagePacketResult mpr;
                    mpr.success = true;
                    mpr.exitCode = exitCode;
                    DWORD dwWrite = 0;
                    ::WriteFile(hPipe, &mpr, sizeof(mpr), &dwWrite, 0);
                }
                else
                {
                    MessagePacketResult mpr;
                    mpr.success = false;
                    mpr.exitCode = 0;
                    DWORD dwWrite = 0;
                    ::WriteFile(hPipe, &mpr, sizeof(mpr), &dwWrite, 0);
                }
            }
            ::FlushFileBuffers(hPipe);
            ::DisconnectNamedPipe(hPipe);
        }
    }

    CloseHandle(hEvent);
    CloseHandle(hPipe);

    return ERROR_SUCCESS;
}

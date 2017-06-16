/*
 
    File: OVPNHelper.c
*/

#include <syslog.h>
#include <xpc/xpc.h>

// fork a child process, execute vlc, and return it's pid.
// returns -1 if fork failed.

static pid_t runProcess(const char *szCommand)
{
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    }
    
    // when you call fork(), it creates two copies of your program:
    // a parent, and a child. you can tell them apart by the return
    // value from fork().  If fork() returns 0, this is is the child
    // process.  If fork() returns non-zero, we are the parent and the
    // return value is the PID of the child process.
    if (pid == 0) {
        // this is the child process.  now we can call one of the exec
        // family of functions to execute VLC.  when you call exec,
        // it replaces the currently running process (the child process)
        // with whatever we pass to exec.  So our child process will now
        // be running VLC.  exec() will never return except in an error
        // case, since it is now running the VLC code and not our code.
        system(szCommand);
        //execlp("vlc", "vlc", (char*)NULL);
        //perror("vlc");
        exit(0);
        
    } else {
        // parent, return the child's PID back to main.
        return pid;
    }
}

static void __XPC_Peer_Event_Handler(xpc_connection_t connection, xpc_object_t event)
{
    syslog(LOG_NOTICE, "Received event in helper.");
    
	xpc_type_t type = xpc_get_type(event);
    
	if (type == XPC_TYPE_ERROR)
    {
		if (event == XPC_ERROR_CONNECTION_INVALID)
        {
			// The client process on the other end of the connection has either
			// crashed or cancelled the connection. After receiving this error,
			// the connection is in an invalid state, and you do not need to
			// call xpc_connection_cancel(). Just tear down any associated state
			// here.
            
		}
        else if (event == XPC_ERROR_TERMINATION_IMMINENT)
        {
			// Handle per-connection termination cleanup.
		}
	}
    else
    {
        xpc_connection_t remote = xpc_dictionary_get_remote_connection(event);

        xpc_object_t reply = xpc_dictionary_create_reply(event);

        const char* cmd = xpc_dictionary_get_string(event, "cmd");
        
        if (strcmp(cmd, "openvpn") == 0)
        {
        
            const char* path_tun = xpc_dictionary_get_string(event, "path_tun");
            const char* path_tap = xpc_dictionary_get_string(event, "path_tap");
            const char* openvpncmd = xpc_dictionary_get_string(event, "openvpncmd");
        
        
            char szBuf[PATH_MAX + 100];
        
            system("mkdir -p /var/run/OVPNOneVPNHelper");
            // install tun driver
            if (path_tun != NULL)
            {
                //copy command
                strcpy(szBuf, "cp -R ");
                strcat(szBuf, path_tun);
                strcat(szBuf, " /var/run/OVPNOneVPNHelper");
                system(szBuf);
            
                //chown command
                system("chown -R root:wheel /var/run/OVPNOneVPNHelper/tun.kext");
            
                //kextload command
                strcpy(szBuf, "/sbin/kextload ");
                strcat(szBuf, "/var/run/OVPNOneVPNHelper/tun.kext");
                system(szBuf);
            }
            // install tap driver
            if (path_tap != NULL)
            {
                //copy command
                strcpy(szBuf, "cp -R ");
                strcat(szBuf, path_tap);
                strcat(szBuf, " /var/run/OVPNOneVPNHelper");
                system(szBuf);
            
                //chown command
                system("chown -R root:wheel /var/run/OVPNOneVPNHelper/tap.kext");
            
                //kextload command
                strcpy(szBuf, "/sbin/kextload ");
                strcat(szBuf, "/var/run/OVPNOneVPNHelper/tap.kext");
                system(szBuf);
            }
            // run openvpn command
            if (openvpncmd != NULL)
            {
                //__int32_t pid = runProcess(openvpncmd);
                //char bf[64];
                //itoa(pid, bf, 10);
                //sprintf(bf, "pid: %d", pid);
                system(openvpncmd);
                xpc_dictionary_set_string(reply, "reply", "openvpn command executed (ver 3)");
                //xpc_dictionary_set_string(reply, "str", bf);
                //xpc_dictionary_set_int64(reply, "pid", pid);
            }
        
            xpc_connection_send_message(remote, reply);
        }
        else if (strcmp(cmd, "anycmd") == 0)
        {
            const char* command_line = xpc_dictionary_get_string(event, "command_line");
            
            //__int32_t pid = runProcess(command_line);
            system(command_line);
            xpc_dictionary_set_string(reply, "reply", "anycmd command executed (ver 3)");
            //xpc_dictionary_set_int64(reply, "pid", pid);
            xpc_connection_send_message(remote, reply);
            
        }
        xpc_release(reply);
	}
}

static void __XPC_Connection_Handler(xpc_connection_t connection)
{
    syslog(LOG_NOTICE, "Configuring message event handler for helper.");
    
	xpc_connection_set_event_handler(connection, ^(xpc_object_t event)
    {
		__XPC_Peer_Event_Handler(connection, event);
	});
	
	xpc_connection_resume(connection);
}

int main(int argc, const char *argv[])
{
    xpc_connection_t service = xpc_connection_create_mach_service("com.aaa.onevpn.OVPNHelper",
                                                                  dispatch_get_main_queue(),
                                                                  XPC_CONNECTION_MACH_SERVICE_LISTENER);
    
    if (!service)
    {
        syslog(LOG_NOTICE, "Failed to create service.");
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_NOTICE, "Configuring connection event handler for helper");
    xpc_connection_set_event_handler(service, ^(xpc_object_t connection)
    {
        __XPC_Connection_Handler(connection);
    });
    
    xpc_connection_resume(service);
    
    dispatch_main();
    
    xpc_release(service);
    
    return EXIT_SUCCESS;
}


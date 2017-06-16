#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) 
{
	if (argc != 3)
	    	return 0;
	
	char *configPath;
	char *port;
	char cmd[16000];
   

    	configPath = argv[1];
    	port = argv[2];
        
    	setuid(0);
    	clearenv();
	system("iptables-save > /tmp/onevpniptablesConfig.conf");
    	sprintf(cmd, "openvpn --config %s --management 127.0.0.1 %s --management-query-passwords", configPath, port);
    	system(cmd);
}

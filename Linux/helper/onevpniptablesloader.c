#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) 
{
    	char *configPath = argv[1];
    	setuid(0);
    	clearenv();
	system("iptables -P INPUT ACCEPT;"
		"iptables -P FORWARD ACCEPT;"
		"iptables -P OUTPUT ACCEPT;"
		"iptables -F;"
		"iptables -X;"
		"iptables -t nat -F;"
		"iptables -t nat -X;"
		"iptables -t mangle -F;"
		"iptables -t mangle -X;"
		"iptables -t raw -F;"
		"iptables -t raw -X");
	char cmd[1600];
	sprintf(cmd, "iptables-restore < %s", configPath);
    	system(cmd);
}

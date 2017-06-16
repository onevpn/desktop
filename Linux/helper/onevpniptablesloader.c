#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) 
{
    	char *configPath = argv[1];
    	setuid(0);
    	clearenv();
	char cmd[1600];
	sprintf(cmd, "iptables-restore < %s", configPath);
    	system(cmd);
}

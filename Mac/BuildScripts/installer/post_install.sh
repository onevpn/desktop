#!/bin/sh -e

AGENTSDIR="/Library/LaunchDaemons/"

installAgent()
{
	if [ ! -d "$AGENTSDIR" ]; then
		mkdir -p "$AGENTSDIR"
		chown root "$AGENTSDIR"
	fi

	name=$1
	agentFile="$AGENTSDIR/$name"
	if [ -e "$agentFile" ]; then
		sudo launchctl unload -w "$agentFile"
		rm "$agentFile"
	fi
	cp "$name" "$AGENTSDIR"
	#sed -i '' "s/@USER@/$USER/" "$agentFile"
	#chmod -x "$agentFile"
	#chown "$USER" "$agentFile"

	sudo launchctl load -F "$agentFile"
}

runAgent()
{
	name=$1
	sudo launchctl start "$name"
}

disableAgent()
{
 	local path="$AGENTSDIR/$1"
    if [ -e "$path" ]; then
        sudo launchctl unload -w "$path"
    fi
}

sudo installer -pkg 'tuntap.pkg' -target /

disableAgent "com.aaa.onevpn.OVPNHelper.plist"

AGENTEXECDIR="/Library/PrivilegedHelperTools/"
if [ ! -d "$AGENTEXECDIR" ]; then
	mkdir -p "$AGENTEXECDIR"
	chown root "$AGENTSDIR"
fi

sudo cp "com.aaa.onevpn.OVPNHelper" "$AGENTEXECDIR"
sudo cp "com.aaa.OneVPN.starter.plist" "$AGENTEXECDIR"

installAgent "com.aaa.OneVPN.starter.plist"
installAgent "com.aaa.onevpn.OVPNHelper.plist"

runAgent "com.aaa.onevpn.OVPNHelper"

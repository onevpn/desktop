#include "Updater.h"

#include <Sparkle/Sparkle.h>
#include <Sparkle/SUUpdater.h>

static SUUpdater* updater = [[SUUpdater sharedUpdater] retain];

void checkUpdates()
{
	[updater setFeedURL: [NSURL URLWithString: @"https://onevpn.co/appcast.xml"]];
	[updater setUpdateCheckInterval: 3600];
	[updater checkForUpdatesInBackground];
}

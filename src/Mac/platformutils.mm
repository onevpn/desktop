#include "../platformutils.h"

#import <Cocoa/Cocoa.h>

void hideFromDock()
{
    [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];
}

void showInDock()
{
    [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
}

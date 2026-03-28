#import <Cocoa/Cocoa.h>
#include "StellarrPlatform.h"

void stellarrSetDarkAppearance()
{
    [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
}

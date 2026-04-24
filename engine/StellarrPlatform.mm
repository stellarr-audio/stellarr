#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "StellarrPlatform.h"

void stellarrSetDarkAppearance()
{
    [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
}

static WKWebView* findWebView(NSView* view)
{
    if ([view isKindOfClass:[WKWebView class]])
        return (WKWebView*)view;

    for (NSView* subview in view.subviews)
    {
        if (auto* found = findWebView(subview))
            return found;
    }

    return nil;
}

void stellarrMakeWebViewInspectable(void* nativeView)
{
    if (nativeView == nullptr)
        return;

    NSView* view = (__bridge NSView*)nativeView;
    WKWebView* webView = findWebView(view);

    if (webView != nil)
    {
        if ([webView respondsToSelector:@selector(setInspectable:)])
            [webView setInspectable:YES];

        // Prevent white flash while page loads
        [webView setValue:@NO forKey:@"drawsBackground"];
        webView.enclosingScrollView.backgroundColor = [NSColor colorWithRed:0.05 green:0.04 blue:0.1 alpha:1.0];
    }
}

juce::File stellarrGetBundleResource(const juce::String& subpath)
{
    return juce::File::getSpecialLocation(juce::File::currentApplicationFile)
        .getChildFile("Contents/Resources")
        .getChildFile(subpath);
}

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

    if (webView != nil && [webView respondsToSelector:@selector(setInspectable:)])
        [webView setInspectable:YES];
}

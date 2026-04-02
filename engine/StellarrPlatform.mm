#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <mach/mach.h>
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

double stellarrGetProcessMemoryMB()
{
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_VM_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS)
    {
        return static_cast<double>(info.phys_footprint) / (1024.0 * 1024.0);
    }

    return 0.0;
}

double stellarrGetTotalMemoryMB()
{
    return static_cast<double>([NSProcessInfo processInfo].physicalMemory) / (1024.0 * 1024.0);
}

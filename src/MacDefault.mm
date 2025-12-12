#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>

extern "C" void setDefaultURLHandlerForHttpHttpsMac() {
    CFStringRef current = LSCopyDefaultHandlerForURLScheme(CFSTR("http"));
    NSString *bundleId = [[NSBundle mainBundle] bundleIdentifier];
    if (current && CFStringCompare(current, (__bridge CFStringRef)bundleId, 0) == kCFCompareEqualTo) {
        // Already default, do nothing
        CFRelease(current);
        return;
    }
    CFRelease(current);
    if (!bundleId) return;
    LSSetDefaultHandlerForURLScheme(CFSTR("http"), (__bridge CFStringRef)bundleId);
    LSSetDefaultHandlerForURLScheme(CFSTR("https"), (__bridge CFStringRef)bundleId);
}

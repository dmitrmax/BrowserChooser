#ifdef Q_OS_MAC
#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>

extern "C" void setDefaultURLHandlerForHttpHttpsMac() {
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
    if (!bundleId) return;
    LSSetDefaultHandlerForURLScheme(CFSTR("http"), (__bridge CFStringRef)bundleId);
    LSSetDefaultHandlerForURLScheme(CFSTR("https"), (__bridge CFStringRef)bundleId);
}
#endif

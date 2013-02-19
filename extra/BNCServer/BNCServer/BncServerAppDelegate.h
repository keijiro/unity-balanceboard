// BncServerAppDelegate.h
// Based on MacOS Communications Driver written by Ian Rickard
// http://alumni.soe.ucsc.edu/~inio/wii.html

#import <Cocoa/Cocoa.h>
#import "Wiimote.h"

@interface BncServerAppDelegate : NSObject <NSApplicationDelegate>
{
    char basenameDisplay[128];
    char basename[104];
}

@property NSMutableArray *wiimotes;
@property Wiimote *connecting;
@property IOBluetoothDeviceInquiry *inquiry;

@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet NSButton *disconnectButton;
@property (assign) IBOutlet NSButton *syncButton;
@property (assign) IBOutlet NSProgressIndicator *syncIndicator;
@property (assign) IBOutlet NSTableView *wiiList;
@property (assign) IBOutlet NSTextField *statusLine;

- (IBAction)disconect:(id)sender;
- (IBAction)sync:(id)sender;

@end

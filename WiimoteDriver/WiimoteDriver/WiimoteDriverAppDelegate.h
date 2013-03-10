// WiimoteDriverAppDelegate.h
// Based on MacOS Communications Driver written by Ian Rickard
// http://alumni.soe.ucsc.edu/~inio/wii.html

#import <Cocoa/Cocoa.h>
#import "Wiimote.h"

@interface WiimoteDriverAppDelegate : NSObject <NSApplicationDelegate>

@property NSMutableArray *wiimotes;
@property IOBluetoothDeviceInquiry *inquiry;
@property IOBluetoothDevice *connecting;

@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet NSButton *disconnectButton;
@property (assign) IBOutlet NSButton *syncButton;
@property (assign) IBOutlet NSProgressIndicator *syncIndicator;
@property (assign) IBOutlet NSTableView *deviceTable;
@property (assign) IBOutlet NSTextField *statusLine;

- (IBAction)disconect:(id)sender;
- (IBAction)sync:(id)sender;

@end

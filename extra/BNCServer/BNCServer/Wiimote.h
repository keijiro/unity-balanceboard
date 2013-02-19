// Wiimote.h
// Based on MacOS Communications Driver written by Ian Rickard
// http://alumni.soe.ucsc.edu/~inio/wii.html

#import <Foundation/Foundation.h>

#import <IOBluetooth/objc/IOBluetoothDevice.h>
#import <IOBluetooth/objc/IOBluetoothDeviceInquiry.h>
#import <IOBluetooth/objc/IOBluetoothL2CAPChannel.h>

#include <sys/un.h>

#define WIIMOTE_USE_INET 0

@interface Wiimote : NSObject
{
@public
	int sock, stream;
#if WIIMOTE_USE_INET
    struct sockaddr_in addr;
#else
	struct sockaddr_un addr;
#endif
}

@property (assign) NSInteger index;
@property (copy) NSString *displayName;

@property NSLock *streamLock;
@property NSLock *deviceLock;

@property IOBluetoothDevice *device;
@property IOBluetoothL2CAPChannel *ichan;
@property IOBluetoothL2CAPChannel *cchan;
@property IOBluetoothUserNotification *disconNote;
@property IOBluetoothUserNotification *ichanNote;
@property IOBluetoothUserNotification *cchanNote;

- (id)initWithDevice:(IOBluetoothDevice *)aDevice;
- (void)reinitialize;
- (void)disconnect;

@end

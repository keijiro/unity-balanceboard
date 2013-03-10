// Wiimote.h
// Based on MacOS Communications Driver written by Ian Rickard
// http://alumni.soe.ucsc.edu/~inio/wii.html

#import <Foundation/Foundation.h>

#import <IOBluetooth/objc/IOBluetoothDevice.h>
#import <IOBluetooth/objc/IOBluetoothDeviceInquiry.h>
#import <IOBluetooth/objc/IOBluetoothL2CAPChannel.h>

#include <netinet/in.h>

@interface Wiimote : NSObject
{
@public
	int sock, stream;
    struct sockaddr_in addr;
}

@property (assign) NSInteger deviceIndex;
@property (copy) NSString *displayName;

@property NSLock *streamLock;

@property IOBluetoothDevice *device;
@property IOBluetoothL2CAPChannel *ichan;
@property IOBluetoothL2CAPChannel *cchan;
@property IOBluetoothUserNotification *disconNote;
@property IOBluetoothUserNotification *ichanNote;
@property IOBluetoothUserNotification *cchanNote;

- (id)initWithDevice:(IOBluetoothDevice *)aDevice;
- (void)disconnect;

@end

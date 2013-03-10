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
    NSInteger deviceIndex_;
    struct sockaddr_in addr_;
	int sock_;
    int stream_;
}

@property (readonly) NSInteger deviceIndex;
@property (readonly) NSString *displayName;

@property NSLock *streamLock;

@property IOBluetoothDevice *device;
@property IOBluetoothL2CAPChannel *cchan;
@property IOBluetoothL2CAPChannel *ichan;
@property IOBluetoothUserNotification *disconNote;
@property IOBluetoothUserNotification *ichanNote;
@property IOBluetoothUserNotification *cchanNote;

- (id)initWithDevice:(IOBluetoothDevice *)device index:(NSInteger)index;
- (void)disconnect;

- (BOOL)openCChanWithObserver:(id)observer closeNotification:(SEL)closeSelector;
- (BOOL)openIChanWithObserver:(id)observer closeNotification:(SEL)closeSelector;
- (BOOL)openSocket;

- (BOOL)acceptStreamConnection;
- (void)doStreamReaderLoop;

- (void)processReceivedData:(void *)dataPointer length:(size_t)length;

@end

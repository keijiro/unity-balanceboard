// WiimoteDriverAppDelegate.m
// Based on MacOS Communications Driver written by Ian Rickard
// http://alumni.soe.ucsc.edu/~inio/wii.html

#import "WiimoteDriverAppDelegate.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netdb.h>

@implementation WiimoteDriverAppDelegate

#pragma mark Application lifecycle

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	self.wiimotes = [[NSMutableArray alloc] initWithCapacity:16];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(incomingConnection:) name:NSFileHandleConnectionAcceptedNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(dataAvailable:) name:NSFileHandleDataAvailableNotification object:nil];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    for (Wiimote *wiimote in self.wiimotes) {
        [wiimote disconnect];
    }

    self.wiimotes = nil;
    self.inquiry = nil;
    self.connecting = nil;
}

#pragma mark Private methods

- (void)removeWiimote:(Wiimote *)wiimote
{
    [wiimote disconnect];
    [self.wiimotes removeObject:wiimote];
	[self.deviceTable noteNumberOfRowsChanged];
}

- (Wiimote *)wiimoteForDevice:(IOBluetoothDevice *)device {
    for (Wiimote *wiimote in self.wiimotes) {
        if ([device isEqual:wiimote.device]) return wiimote;
    }
    return nil;
}

- (NSInteger)searchUnusedDeviceIndex {
    for (NSInteger index = 1; ; index++) {
        BOOL used = NO;
        for (Wiimote *wiimote in self.wiimotes) {
            if (wiimote.deviceIndex == index) {
                used = YES;
                break;
            }
        }
        if (!used) return index;
    }
}

#pragma mark Actions

- (IBAction)disconect:(id)sender
{
    if (self.deviceTable.selectedRow >= 0) {
        [self removeWiimote:self.wiimotes[self.deviceTable.selectedRow]];
    }
}

- (IBAction)sync:(id)sender
{
    NSAssert(self.inquiry == nil , @"Already syncing.");
    NSAssert(self.connecting == nil, @"Can't sync while a connection is establishing.");

    // Create an inquiry and start it.
 	self.inquiry = [IOBluetoothDeviceInquiry inquiryWithDelegate:self];
	[self.inquiry clearFoundDevices];
	IOReturn ret = [self.inquiry start];

	if (ret == kIOReturnSuccess){
		self.syncButton.enabled = NO;
		[self.syncIndicator startAnimation:self];
	} else {
		self.statusLine.stringValue = @"Failed to start an inquiry.";
        self.inquiry = nil;
	}
}

#pragma mark Table View Data Soruce methods

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.wiimotes.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
	if (row < 0 || row >= self.wiimotes.count) return @"";
	
	Wiimote *wiimote = self.wiimotes[row];
	
	switch ([[tableColumn identifier] intValue]) {
		case 1:
			return wiimote.displayName;
		case 2:
			return (wiimote->stream_ != -1) ? @"Yes" : @"No";
		case 3:
			return wiimote.device.addressString;
		default:
			return @"---";
	}
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    self.disconnectButton.enabled = ([self.deviceTable numberOfSelectedRows] > 0);
}

#pragma mark Bluetooth Device Inquiry delegates

- (void)deviceInquiryStarted:(IOBluetoothDeviceInquiry *)sender {
	self.statusLine.stringValue = @"Searching. Press sync button on a device.";
}

- (void)deviceInquiryComplete:(IOBluetoothDeviceInquiry *)sender error:(IOReturn)error aborted:(BOOL)aborted {
    self.inquiry = nil;
    
    self.syncButton.enabled = (self.connecting == nil);
	[self.syncIndicator stopAnimation:self];
	
	if (error != kIOReturnSuccess) {
        self.statusLine.stringValue = @"Inquiry ended with error.";
	} else {
        self.statusLine.stringValue = @"";
	}
}

- (void)deviceInquiryDeviceNameUpdated:(IOBluetoothDeviceInquiry *)sender device:(IOBluetoothDevice *)device devicesRemaining:(uint32_t)devicesRemaining {
	[self checkFoundDevice:device];
}

- (void)deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry *)sender device:(IOBluetoothDevice *)device {
	[self checkFoundDevice:device];
}

- (void)checkFoundDevice:(IOBluetoothDevice *)device {
    NSAssert(self.connecting == nil, @"Can't check a device while connecting.");
    
    // Check if the device is a sort of Wii Remote.
	if (![device.name hasPrefix:@"Nintendo RVL-"]) return;
    
    // Check if the device is already in our list.
    if ([self wiimoteForDevice:device] != nil) return;
    
    // We can stop the inquiry at this point.
    [self.inquiry stop];
    
    IOReturn ret = [device openConnection:self];
    
    if (ret == kIOReturnSuccess) {
        self.connecting = device;
        self.statusLine.stringValue = @"Connecting...";
    } else {
        self.syncButton.enabled = YES;
        [self.syncIndicator stopAnimation:self];
        self.statusLine.stringValue= @"Failed to start a connection to Wii Remote.";
    }
    
    // We don't need this inquiry anymore.
    self.inquiry = nil;
}

#pragma Connection process

- (void)connectionComplete:(IOBluetoothDevice *)device status:(IOReturn)status
{
    NSAssert([device isEqual:self.connecting], @"Wrong device");
    
    NSLog(@"Connection completed.");

    // Create a Wiimote for this device temporary.
    Wiimote *wiimote = [[Wiimote alloc] initWithDevice:device index:[self searchUnusedDeviceIndex]];
    [self.wiimotes addObject:wiimote];

    // No longer connecting.
    self.connecting = nil;
    
    // Change the UI status.
    self.syncButton.enabled = YES;
    [self.syncIndicator stopAnimation:self];
	self.statusLine.stringValue = @"";
	[self.deviceTable noteNumberOfRowsChanged];
    
	if (status != kIOReturnSuccess) {
        // The connection is failed.
        self.statusLine.stringValue = @"Failed on connecting to the controller.";
        NSLog(@"Error on connectionComplete (%08X)", status);
        [self removeWiimote:wiimote];
		return;
	}
    
    // It seems the connection is succeeded, so we want to get notified on disconnection.
    wiimote.disconNote = [device registerForDisconnectNotification:self selector:@selector(disconnected:fromDevice:)];
    
    // Open pipes.
    if (![wiimote openCChanWithObserver:self closeNotification:@selector(channelClosed:channel:)]) {
		self.statusLine.stringValue = @"Failed to open a control pipe.";
        [self removeWiimote:wiimote];
		return;
    }
    
    if (![wiimote openIChanWithObserver:self closeNotification:@selector(channelClosed:channel:)]) {
		self.statusLine.stringValue = @"Failed to open a data pipe.";
        [self removeWiimote:wiimote];
		return;
    }
    
    if (![wiimote openSocket]) {
        self.statusLine.stringValue = @"Failed to bind a socket.";
        [self removeWiimote:wiimote];
        return;
    }
    
	// Start the server thread.
	[NSThread detachNewThreadSelector:@selector(serverThread:) toTarget:self withObject:wiimote];
}

- (void)disconnected:(IOBluetoothUserNotification *)note fromDevice:(IOBluetoothDevice *)device
{
    Wiimote *wiimote = [self wiimoteForDevice:device];
    NSAssert(wiimote != nil, @"An unknown device has been disconnected.");
    
    NSLog(@"%@ has been disconnected.", wiimote.displayName);
    
    [self removeWiimote:wiimote];
    self.statusLine.stringValue = [NSString stringWithFormat:@"%@ has been disconnected.", wiimote.displayName];
}

- (void)channelClosed:(IOBluetoothUserNotification *)note channel:(IOBluetoothL2CAPChannel *)channel
{
    Wiimote *wiimote = [self wiimoteForDevice:channel.device];
    NSAssert(wiimote != nil, @"An unknown device has been disconnected.");
    
    NSLog(@"%@ closed channel %d.", wiimote.displayName, channel.remoteChannelID);

    [self removeWiimote:wiimote];
    self.statusLine.stringValue = [NSString stringWithFormat:@"%@ has been disconnected.", wiimote.displayName];
}

#pragma mark Interprocess communication

- (void)serverThread:(Wiimote*)wiimote {
	while (wiimote.device != nil && [wiimote.device isConnected]) {
		wiimote->stream_ = -1;
		[self.deviceTable performSelectorOnMainThread:@selector(reloadData) withObject:nil waitUntilDone:NO];

        struct sockaddr_in far;
		socklen_t farlen = sizeof(far);
		wiimote->stream_ = accept(wiimote->sock_, (struct sockaddr*)&far, &farlen);
		
		[wiimote.streamLock lock];
		int value = 1;
		setsockopt(wiimote->stream_, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
		[wiimote.streamLock unlock];
        
		if (wiimote->stream_ < 0) break;
        NSLog(@"Accepted connection on fd %d", wiimote->stream_);

		[self.deviceTable performSelectorOnMainThread:@selector(reloadData) withObject:nil waitUntilDone:NO];
		
		unsigned char buffer[256];
		ssize_t length;
		int stream = wiimote->stream_;
        
		while (wiimote->stream_ != -1) {
			ssize_t read = recv(stream, buffer, 1, MSG_WAITALL);
			if (read != 1) {
				NSLog(@"Read packet length failed (%ld)", read);
				break;
			}
			
			if (wiimote->stream_ != stream) break;
			
			length = buffer[0];
			
			read = recv(stream, buffer, length, MSG_WAITALL);
			if (read != length) {
				NSLog(@"Read packet payload failed (%ld != %ld)\n", read, length);
				break;
			}
            
            if (buffer[0] == 0xa2) {
                [wiimote.cchan writeSync:buffer length:length];
            }
		}
        
		NSLog(@"%@ Exited read loop", wiimote.displayName);

		[wiimote.streamLock lock];
		if (wiimote->stream_ != -1) {
			close(wiimote->stream_);
			wiimote->stream_ = -1;
		}
		[wiimote.streamLock unlock];
		
        NSLog(@"%@ Stream closed", wiimote.displayName);
	}
}

- (void)l2capChannelData:(IOBluetoothL2CAPChannel *)l2capChannel data:(void *)dataPointer length:(size_t)length{
	IOBluetoothDevice *sender = l2capChannel.device;
    Wiimote *wiimote = [self wiimoteForDevice:sender];
	
	if (wiimote == nil) {
		NSLog(@"Received data for unknown wiimote!");
		return;
	}
	
	[wiimote.streamLock lock];
	
	if (wiimote->stream_ != -1) {
		unsigned char header[2];
		header[0] = length + 1;
		header[1] = l2capChannel.localChannelID;
        
        BOOL error = NO;

		if (write(wiimote->stream_, header, 2) != 2) {
            NSLog(@"Write header failed.");
            error = YES;
        }
		
		if (write(wiimote->stream_, dataPointer, length) != length) {
            NSLog(@"Write payload failed.");
            error = YES;
        }
        
		if (error) {
			close(wiimote->stream_);
			wiimote->stream_ = -1;
		}
	}

	[wiimote.streamLock unlock];
}

@end

// Wiimote.m
// Based on MacOS Communications Driver written by Ian Rickard
// http://alumni.soe.ucsc.edu/~inio/wii.html

#import "Wiimote.h"

@implementation Wiimote

- (id)initWithDevice:(IOBluetoothDevice *)device index:(NSInteger)index
{
    self = [super init];
    if (self) {
        stream_ = -1;
        sock_ = -1;
        deviceIndex_ = index;
        self.device = device;
		self.streamLock = [[NSLock alloc] init];
    }
    return self;
}

- (void)disconnect
{
    if (self.disconNote != nil) {
        [self.disconNote unregister];
        self.disconNote = nil;
    }

    if (self.ichanNote != nil) {
        [self.ichanNote unregister];
        self.ichanNote = nil;
    }
    
    if (self.cchanNote != nil) {
        [self.cchanNote unregister];
        self.cchanNote = nil;
    }
    
    if (self.device != nil) {
        if ([self.device isConnected]) {
            if (self.cchan != nil) {
                [self.cchan closeChannel];
            }

            if (self.ichan != nil) {
                [self.ichan closeChannel];
            }
            
            [self.device closeConnection];
        }
    }
    
    self.cchan = nil;
    self.cchan = nil;
    self.device = nil;
	
    if (stream_ >= 0) close(stream_);
	if (sock_ >= 0) close(sock_);
}

- (NSInteger)deviceIndex
{
    return deviceIndex_;
}

- (NSString*)displayName
{
    return [NSString stringWithFormat:@"wii%ld", self.deviceIndex];
}

- (BOOL)openCChanWithObserver:(id)observer closeNotification:(SEL)closeSelector
{
    NSLog(@"Open a control pipe (L2CAP ch. 17)");
    
    IOBluetoothL2CAPChannel *cchan = nil;
	IOReturn ret = [self.device openL2CAPChannelSync:&cchan withPSM:17 delegate:observer];
	
    if (ret != kIOReturnSuccess) {
		NSLog(@"Error on openL2CAPChannelSync 17 (%08X)", ret);
		return NO;
	}
    
    self.cchan = cchan;
    self.cchanNote = [cchan registerForChannelCloseNotification:observer selector:closeSelector];
    
    return YES;
}

- (BOOL)openIChanWithObserver:(id)observer closeNotification:(SEL)closeSelector
{
    NSLog(@"Open a data pipe (L2CAP ch. 19)");
    
    IOBluetoothL2CAPChannel *ichan = nil;
	IOReturn ret = [self.device openL2CAPChannelSync:&ichan withPSM:19 delegate:observer];
    
	if (ret != kIOReturnSuccess) {
		NSLog(@"Error on openL2CAPChannelSync 19 (%08X)", ret);
        return NO;
	}
    
    self.ichan = ichan;
	self.ichanNote = [ichan registerForChannelCloseNotification:observer selector:closeSelector];
    
    return YES;
}

- (BOOL)openSocket
{
    NSLog(@"Open socket.");
    
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        NSLog(@"Error on creating a socket.");
        return NO;
    }
    
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(8000 + deviceIndex_);
    
	if (bind(sock_, (void*)&addr_, sizeof(addr_)) < 0) {
        NSLog(@"Error on socket binding.");
		return NO;
    }
    
	listen(sock_, 0);
	    
    return YES;
}

- (BOOL)acceptStreamConnection
{
    struct sockaddr_in far;
    socklen_t farlen = sizeof(far);
    int newStream = accept(sock_, (struct sockaddr*)&far, &farlen);

    if (newStream < 0) return NO;

    int value = 1;
    setsockopt(newStream, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));

    [self.streamLock lock];
    stream_ = newStream;
    [self.streamLock unlock];

    NSLog(@"Accepted a connection on fd %d.", newStream);
    
    return YES;
}

- (void)doStreamReaderLoop
{
    while (stream_ >= 0) {
        unsigned char buffer[256];
        ssize_t length;
        
        ssize_t read = recv(stream_, buffer, 1, MSG_WAITALL);
        if (read != 1) {
            NSLog(@"Failed to read packet length (%ld)", read);
            break;
        }
        length = buffer[0];
        
        // Check if the stream is still valid.
        if (stream_ < 0) break;
        
        read = recv(stream_, buffer, length, MSG_WAITALL);
        if (read != length) {
            NSLog(@"Failed to read packet payload failed (%ld)", length);
            break;
        }
        
        // Send control data.
        if (buffer[0] == 0xa2) {
            [self.cchan writeSync:buffer length:length];
        }
    }
    
    NSLog(@"A reader loop on %@ has been exited.", self.displayName);
    
    [self.streamLock lock];
    if (stream_ >= 0) {
        close(stream_);
        stream_ = -1;
    }
    [self.streamLock unlock];
}

- (void)processReceivedData:(void *)dataPointer length:(size_t)length
{
    // Do nothing with nonactive stream.
    if (stream_ < 0) return;
    
	[self.streamLock lock];
	
    unsigned char header[2];
    header[0] = length + 1;
    header[1] = self.ichan.localChannelID;
    
    BOOL error = NO;
    
    if (write(stream_, header, 2) != 2) {
        NSLog(@"Failed to write header.");
        error = YES;
    }
    
    if (!error && write(stream_, dataPointer, length) != length) {
        NSLog(@"Failed to write payload.");
        error = YES;
    }
    
    // Close the stream on errors.
    if (error) {
        close(stream_);
        stream_ = -1;
    }
    
	[self.streamLock unlock];
}

@end

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
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(8000 + deviceIndex_);
    
	if (bind(sock, (void*)&addr, sizeof(addr)) < 0) {
        NSLog(@"Error on socket binding.");
		return NO;
    }
    
	listen(sock_, 0);
	
    sock_ = sock;
    addr_ = addr;
    
    return YES;
}

@end

// Wiimote.m
// Based on MacOS Communications Driver written by Ian Rickard
// http://alumni.soe.ucsc.edu/~inio/wii.html

#import "Wiimote.h"

@implementation Wiimote

- (id)initWithDevice:(Wiimote *)aDevice
{
    self = [super init];
    if (self) {
        stream = -1;
        sock = -1;
		self.streamLock = [[NSLock alloc] init];
		self.deviceLock = [[NSLock alloc] init];
    }
    return self;
}
        
- (void)sendInitializeCode
{
	[self.cchan writeSync:(unsigned char[]){0xa2, 0x11, 0xF0} length:3];
	[self.cchan writeSync:(unsigned char[]){0xa2, 0x12, 0x00, 0x30} length:4];
	[self.cchan writeSync:(unsigned char[]){0xa2, 0x13, 0x00} length:3];
	[self.cchan writeSync:(unsigned char[]){0xa2, 0x1A, 0x00} length:3];
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
	
    if (stream >= 0) close(stream);
	if (sock >= 0) close(sock);
}

@end

#ifndef WIIDEV_H
#define WIIDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#define WIIMOTE_USE_INET 1

typedef unsigned char WiiDevData[];
typedef void (*WiiDevMemReadCallback)(int address, const WiiDevData data, int length);
typedef void (*WiiDevDataCallback)(WiiDevData data, int length);

typedef struct WiiDev {
	int sock;
	WiiDevDataCallback dataCallback;
	WiiDevMemReadCallback memCallback;
} *WiiDevRef;

#if WIIMOTE_USE_INET
WiiDevRef WiiDevNewForPort(int port);
#else
WiiDevRef WiiDevNewFromName(const char *name);
#endif

void WiiDevRelease(WiiDevRef ref);

int WiiDevUpdate(WiiDevRef ref);

void WiiDevSetMemReadCallback(WiiDevRef ref, WiiDevMemReadCallback callback);
void WiiDevSetDataReceivedCallback(WiiDevRef ref, WiiDevDataCallback callback);

void WiiDevReadMem(WiiDevRef ref, int address, int length);
void WiiDevWriteMem(WiiDevRef ref, int address, WiiDevData data, int length);

void WiiDevSend(WiiDevRef ref, WiiDevData data, int length);

#ifdef __cplusplus
}
#endif


#endif
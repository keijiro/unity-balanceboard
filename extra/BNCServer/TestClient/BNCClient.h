#ifndef WIIDEV_H
#define WIIDEV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char WiiDevData[];
typedef void (*WiiDevMemReadCallback)(int address, const WiiDevData data, int length);
typedef void (*WiiDevDataCallback)(WiiDevData data, int length);

typedef struct WiiDev {
	int sock;
	WiiDevDataCallback dataCallback;
	WiiDevMemReadCallback memCallback;
} *WiiDevRef;

WiiDevRef WiiDevNewFromName(const char *name);
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
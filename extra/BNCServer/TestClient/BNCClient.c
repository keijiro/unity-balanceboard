#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "BNCClient.h"

WiiDevRef WiiDevNewFromName(const char *name) {
	WiiDevRef out = calloc(sizeof(struct WiiDev), 1);
	
	out->sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (!out->sock) {
		free(out);
		return NULL;
	}

    struct sockaddr_un addr;
    bzero(&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/Library/Wii Remotes/%s", getenv("HOME"), name);

	if (connect(out->sock, (struct sockaddr*)&addr, sizeof(addr))) {
		free(out);
		return NULL;
	}
	
	out->dataCallback = NULL;
	out->memCallback = NULL;
	
	return out;
}

void WiiDevRelease(WiiDevRef ref) {
	close(ref->sock);
	free(ref);
}

int WiiDevUpdate(WiiDevRef ref) {
	unsigned char buf[256];
	
	fd_set readset;
	struct timeval timeout = {0,0};
	FD_ZERO(&readset);
	FD_SET(ref->sock, &readset); 
	
	do {
		ssize_t err = select(ref->sock+1, &readset, NULL, NULL, &timeout);
		
		if (err == 0) break;
		
		err = recv(ref->sock, buf, 1, MSG_WAITALL);
		if (err != 1) {
			printf("recv %d returned %ld\n", 1, err);
			return -1;
		}
		int length = buf[0];
        printf("length %d - ", length);
		
		err = recv(ref->sock, buf, length, MSG_WAITALL);
		if (err != length) {
			printf("recv %d returned %ld\n", length, err);
			return -1;
		}
        for (int i = 0; i < length; i++) {
            printf("%x ", buf[i]);
        }
        puts(" ");
		
		if (NULL != ref->dataCallback)
			ref->dataCallback(buf, length);
		
	//	printf("%02X %02X (%d)\n", buf[0], buf[1], length);
	//	printf("recieved packet of length %d from channel %02X\n", length, buf[0]);
		if (length >= 3 && (buf[0] == 0x41 || buf[0] == 0x42) && buf[1] == 0xA1) {
            if (buf[2] == 0x21) {
				int readlen = (buf[5]>>4)+1;
				int addr = (buf[6]<<8) | buf[7];
				
			//	printf("addr %X len %d\n", addr, readlen);
				
                if (NULL != ref->memCallback) {
					ref->memCallback(addr, buf+8, readlen);
				}
				
			}
		}
	} while(0);
	
	return 0;
}

void WiiDevSetMemReadCallback(WiiDevRef ref, WiiDevMemReadCallback callback) {
	ref->memCallback = callback;
}

void WiiDevSetDataReceivedCallback(WiiDevRef ref, WiiDevDataCallback callback) {
	ref->dataCallback = callback;
}

void WiiDevReadMem(WiiDevRef ref, int address, int length) {
	WiiDevData cmd = {
		0xa2, 0x17,
		0,0,0,0,
		0,0};
	
	cmd[2] = (address>>24)&0x000000FF;
	cmd[3] = (address>>16)&0x000000FF;
	cmd[4] = (address>> 8)&0x000000FF;
	cmd[5] = (address>> 0)&0x000000FF;
	
	cmd[6] = (length>>8)&0x00FF;
	cmd[7] = (length>>0)&0x00FF;
	
	WiiDevSend(ref, cmd, sizeof(cmd));
}

void WiiDevWriteMem(WiiDevRef ref, int address, WiiDevData data, int length) {
	WiiDevData cmd = {0xa2,0x16,
		0,0,0,0,
		0,
		0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	
	cmd[2] = (address>>24)&0x000000FF;
	cmd[3] = (address>>16)&0x000000FF;
	cmd[4] = (address>> 8)&0x000000FF;
	cmd[5] = (address>> 0)&0x000000FF;
	
	cmd[6] = length;
	
	memcpy(cmd+7, data, length);
	WiiDevSend(ref, cmd, sizeof(cmd));
}

void WiiDevSend(WiiDevRef ref, WiiDevData data, int length) {
	unsigned char len = length;
	if (write(ref->sock, &len, 1) != 1)
		perror("write length failed");
	if (write(ref->sock, data, length) != length)
		perror("write payload failed");
}

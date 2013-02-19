#include <cstdio>
#include <unistd.h>
#include "BNCClient.h"

namespace {
    union BncExtData {
        unsigned char raw[8];
        unsigned short values[4];
    };
    
    BncExtData bnc_zero;
    BncExtData bnc_17kg;
    BncExtData bnc_34kg;
}

void data_callback(WiiDevData data, int length) {
    if (data[2] == 0x32) {
        BncExtData bnc;
        for (int i = 0; i < sizeof(BncExtData); i++) bnc.raw[i ^ 1] = data[5 + i];
        float sum = 0.0f;
        for (int i = 0; i < 4; i++) {
            float w = 34.0f * (static_cast<float>(bnc.values[i]) - bnc_zero.values[i]) / (bnc_34kg.values[i] - bnc_zero.values[i]);
            std::printf("%f ", w);
            sum += w;
        }
        std::printf("%f", sum);
        std::puts("\n");
    }
}

void memread_callback(int address, const WiiDevData data, int length) {
    if (address == 0xfe) {
        if (length == 2 && data[0] == 4 && data[1] == 2) {
            std::puts("Balance Board found.");
        } else {
            std::puts("Invalid extension controller found.");
        }
    } else if (address == 0x24 || address == 0x2c || address == 0x34) {
        BncExtData bnc;
        for (int i = 0; i < sizeof(BncExtData); i++) bnc.raw[i ^ 1] = data[i];
        if (address == 0x24) {
            std::printf("zero ");
            bnc_zero = bnc;
        } else if (address == 0x2c) {
            std::printf("17kg ");
            bnc_17kg = bnc;
        } else if (address == 0x34) {
            std::printf("34kg ");
            bnc_34kg = bnc;
        }
        for (int i = 0; i < 4; i++) {
            std::printf("%d ", bnc.values[i]);
        }
        std::puts("\n");
    }
}

int main(int argc, const char * argv[]) {
    WiiDevRef device = WiiDevNewFromName("wii1");
    
    if (device == NULL) {
        std::puts("No BNC found.");
        return 0;
    }
    
    WiiDevSetDataReceivedCallback(device, data_callback);
    WiiDevSetMemReadCallback(device, memread_callback);
    
    {
        WiiDevData data1 = {0x55};
        WiiDevData data2 = {0x00};
        WiiDevWriteMem(device, 0x4a400f0, data1, 1);
        WiiDevWriteMem(device, 0x4a400fb, data2, 1);
    }
    
    WiiDevReadMem(device, 0x4a400fe, 2);
    WiiDevReadMem(device, 0x4a40024, 8);
    WiiDevReadMem(device, 0x4a40024 + 8, 8);
    WiiDevReadMem(device, 0x4a40024 + 16, 8);
    
    {
        WiiDevData cmd = {0xa2, 0x12, 0x00, 0x32};
        WiiDevSend(device, cmd, 4);
    }
    
    while (true) {
        int res = WiiDevUpdate(device);
        if (res < 0) break;
        usleep(1000);
    }
    
    WiiDevRelease(device);
    
    return 0;
}


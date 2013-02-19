#include "WiimoteClientBase.h"
#include <unistd.h>

namespace {
    
    class BNCClient : public Wiimote::ClientBase {
    public:
        union ExtensionData {
            unsigned char raw_[8];
            unsigned short values_[4];
            
            void ReadRawData(const Byte* data) {
                for (int i = 0; i < 8; i++) raw_[i ^ 1] = data[i];
            }
        };
        
        ExtensionData configZero_;
        ExtensionData config17kg_;
        ExtensionData config34kg_;
        
        void ShowWeight(const ExtensionData& data) {
            float sum = 0.0f;
            for (int i = 0; i < 4; i++) {
                float w = data.values_[i];
                float zero = configZero_.values_[i];
                if (w < config17kg_.values_[i]) {
                    w = 17.0f * (w - zero) / (config17kg_.values_[i] - zero);
                } else {
                    w = 34.0f * (w - zero) / (config34kg_.values_[i] - zero);
                }
                std::printf("%f ", w);
                sum += w;
            }
            std::printf("%f", sum);
            std::puts("");
        }
        
        void OnReceiveData(const Byte* data, Size length) {
            if (length > 2 && data[2] == 0x32) {
                ExtensionData incoming;
                incoming.ReadRawData(data + 5);
                ShowWeight(incoming);
            }
        }
        
        void OnReadMemory(Address offset, const Byte* data, Size length) {
            if (offset == 0xfe) {
                if (length == 2 && data[0] == 4 && data[1] == 2) {
                    std::puts("Balance Board found.");
                } else {
                    std::puts("Invalid extension controller found.");
                }
            } else if (offset == 0x24) {
                configZero_.ReadRawData(data);
                std::puts("Calibration data (0kg) received.");
            } else if (offset == 0x2c) {
                config17kg_.ReadRawData(data);
                std::puts("Calibration data (17kg) received.");
            } else if (offset == 0x34) {
                config34kg_.ReadRawData(data);
                std::puts("Calibration data (34kg) received.");
            }
        }
    };
}

int main(int argc, const char * argv[]) {
    BNCClient client;
    typedef BNCClient::Byte Byte;

    if (!client.OpenPort(8000 + 1)) {
        std::puts("No BNC found.");
        return 0;
    }
    
    {
        Byte data1[] = {0x55};
        Byte data2[] = {0x00};
        client.RequestWriteMemory(0x4a400f0, data1, 1);
        client.RequestWriteMemory(0x4a400fb, data2, 1);
    }
    
    client.RequestReadMemory(0x4a400fe, 2);
    client.RequestReadMemory(0x4a40024, 8);
    client.RequestReadMemory(0x4a40024 + 8, 8);
    client.RequestReadMemory(0x4a40024 + 16, 8);
    
    {
        Byte data[] = {0xa2, 0x12, 0x00, 0x32};
        client.SendData(data, 4);
    }
    
    while (client.Update()) {
        usleep(1000);
    }
    
    return 0;
}


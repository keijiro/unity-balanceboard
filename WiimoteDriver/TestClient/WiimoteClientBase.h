#pragma once
#ifdef __cplusplus

#include <string>

namespace Wiimote {
    
    class ClientBase {
    public:
        typedef unsigned long Address;
        typedef unsigned long Size;
        typedef unsigned char Byte;
        
        ClientBase();
        ~ClientBase();

        virtual void OnReceiveData(const Byte* data, Size length) = 0;
        virtual void OnReadMemory(Address offset, const Byte* data, Size length) = 0;
        
        bool OpenPort(int port);
        bool Update();

        bool SendData(const Byte* data, Size length);
        
        bool RequestReadMemory(Address address, Size length);
        bool RequestWriteMemory(Address address, const Byte* data, Size length);
        
        const std::string& GetLastError() const {
            return error_;
        }
        
    private:
        int socket_;
        std::string error_;
    };
}

#endif

#include "WiimoteClientBase.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

namespace Wiimote {

    ClientBase::ClientBase() {
        socket_ = -1;
    }
    
    ClientBase::~ClientBase() {
        if (socket_ >= 0) close(socket_);
    }
    
    bool ClientBase::OpenPort(int port) {
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == -1) {
            error_ = "Can't create a socket.";
            return false;
        }

        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        
        if (connect(socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            error_ = "Can't connect to the server.";
            return false;
        }
        
        return true;
    }
    
    bool ClientBase::Update() {
        Byte buffer[256];

        fd_set readset;
        struct timeval timeout = {0,0};
        FD_ZERO(&readset);
        FD_SET(socket_, &readset);
        
        ssize_t ret = select(socket_ + 1, &readset, NULL, NULL, &timeout);
        if (ret == 0) return true;
        
        ret = recv(socket_, buffer, 1, MSG_WAITALL);
        if (ret != 1) {
            error_ = "recv length failed.";
            return false;
        }

        Size length = buffer[0];

        ret = recv(socket_, buffer, length, MSG_WAITALL);
        if (ret != length) {
            error_ = "recv data failed.";
            return false;
        }
        
        OnReceiveData(buffer, length);

        if (length >= 3 && (buffer[0] == 0x41 || buffer[0] == 0x42) && buffer[1] == 0xA1) {
            if (buffer[2] == 0x21) {
                Address memory_length = (buffer[5] >> 4) + 1;
                Address offset = (buffer[6] << 8) | buffer[7];
                OnReadMemory(offset, buffer + 8, memory_length);
            }
        }
        
        return true;
    }
    
    bool ClientBase::SendData(const Byte* data, Size length) {
        Byte length_data = length;
        if (write(socket_, &length_data, 1) != 1) {
            error_ = "write length failed.";
            return false;
        }

        if (write(socket_, data, length) != length) {
            error_ = "write data failed.";
            return false;
        }
        
        return true;
    }

    bool ClientBase::RequestReadMemory(Address address, Size length) {
        Byte buffer[8] = { 0xa2, 0x17 };
        
        buffer[2] = (address >> 24) & 0xff;
        buffer[3] = (address >> 16) & 0xff;
        buffer[4] = (address >>  8) & 0xff;
        buffer[5] = (address >>  0) & 0xff;
        
        buffer[6] = (length >> 8) & 0xff;
        buffer[7] = (length >> 0) & 0xff;
        
        return SendData(buffer, sizeof(buffer));
    }
    
    bool ClientBase::RequestWriteMemory(Address address, const Byte* data, Size length) {
        Byte buffer[23] = { 0xa2, 0x16 };

        buffer[2] = (address >> 24) & 0xff;
        buffer[3] = (address >> 16) & 0xff;
        buffer[4] = (address >>  8) & 0xff;
        buffer[5] = (address >>  0) & 0xff;
        
        buffer[6] = length;

        memcpy(buffer + 7, data, length);

        return SendData(buffer, sizeof(buffer));
    }
}

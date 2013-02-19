using System.Collections;
using System.Net;
using System.Net.Sockets;
using UnityEngine;

namespace Wiimote {
    public class ClientBase {
        protected virtual void OnReceiveData(byte[] data) {}
        protected virtual void OnReadMemory(uint offset, byte[] data) {}

        public ClientBase() {
        	error_ = "";
        }

        public bool OpenPort(int port) {
        	socket_ = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

			IPHostEntry entry = Dns.GetHostEntry("localhost");
	        foreach (IPAddress address in entry.AddressList) {
	        	if (address.AddressFamily == AddressFamily.InterNetwork) {
			        socket_.Connect(new IPEndPoint(address, port));
			        if (socket_.Connected) break;
	        	}
	        }

	        return socket_.Connected;
        }

        public bool Update() {
        	if (socket_.Available < 2) return true;

        	byte[] lengthBuffer = new byte[1];
        	socket_.Receive(lengthBuffer);

        	int length = lengthBuffer[0];

        	byte[] dataBuffer = new byte[length];
        	socket_.Receive(dataBuffer);

	        OnReceiveData(dataBuffer);

	        if (length >= 3 && (dataBuffer[0] == 0x41 || dataBuffer[0] == 0x42) && dataBuffer[1] == 0xA1) {
	            if (dataBuffer[2] == 0x21) {
	                int memoryLength = (dataBuffer[5] >> 4) + 1;
	                uint offset = (uint)((dataBuffer[6] << 8) | dataBuffer[7]);
	                byte[] memoryBuffer = new byte[memoryLength];
	                System.Array.Copy(dataBuffer, 8, memoryBuffer, 0, memoryLength);
	                OnReadMemory(offset, memoryBuffer);
	            }
	        }
	        
	        return true;
        }

        public bool SendData(byte[] data) {
        	byte[] lengthData = { (byte)data.Length };
        	socket_.Send(lengthData);
        	socket_.Send(data);
	        return true;
        }
        
        public bool RequestReadMemory(uint address, int length) {
	        byte[] buffer = {
	        	0xa2, 0x17,
	        	0, 0, 0, 0,
	        	0, 0
	        };
	        
	        buffer[2] = (byte)((address >> 24) & 0xff);
	        buffer[3] = (byte)((address >> 16) & 0xff);
	        buffer[4] = (byte)((address >>  8) & 0xff);
	        buffer[5] = (byte)((address >>  0) & 0xff);
	        
	        buffer[6] = (byte)((length >> 8) & 0xff);
	        buffer[7] = (byte)((length >> 0) & 0xff);
	        
	        return SendData(buffer);
        }
        
        public bool RequestWriteMemory(uint address, byte[] data) {
	        byte[] buffer = {
	        	0xa2, 0x17,
	        	0, 0, 0, 0,
	        	0,
	        	0, 0, 0, 0,
	        	0, 0, 0, 0,
	        	0, 0, 0, 0,
	        	0, 0, 0, 0
	        };
	        
	        buffer[2] = (byte)((address >> 24) & 0xff);
	        buffer[3] = (byte)((address >> 16) & 0xff);
	        buffer[4] = (byte)((address >>  8) & 0xff);
	        buffer[5] = (byte)((address >>  0) & 0xff);
	        
	        buffer[6] = (byte)data.Length;

	        for (int i = 0; i < data.Length; i++) {
	        	buffer[7 + i] = data[i];
	        }
	        
	        return SendData(buffer);
        }

        public string lastError {
        	get {
        		return error_;
        	}
        }
        
        Socket socket_;
        string error_;
    };
}

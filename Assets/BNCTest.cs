using UnityEngine;
using System.Collections;

public class BNCTestClient : Wiimote.ClientBase {
	int[] calibZero_;
	int[] calib17kg_;
	int[] calib34kg_;

	int[] ConvertRawWeightData(byte[] data, int offset) {
		int[] values = new int[4];
		for (int i = 0; i < 4; i++) {
			values[i] = (data[offset] << 8) + data[offset + 1];
			offset += 2;
		}
		return values;
	} 

	string MakeWeightText(int[] values) {
		string text = "";
		float sum = 0.0f;
		for (int i = 0; i < 4; i++) {
			float w = values[i];
			float zero = calibZero_[i];
			if (w < calib17kg_[i]) {
				w = 17.0f * (w - zero) / (calib17kg_[i] - zero);
			} else {
				w = 34.0f * (w - zero) / (calib34kg_[i] - zero);
			}
			text += w + " ";
			sum += w;
		}
		text += sum;
		return text;
	}

    protected override void OnReceiveData(byte[] data) {
    	if (data.Length > 2 && data[2] == 0x32) {
    		int[] values = ConvertRawWeightData(data, 5);
    		Debug.Log(MakeWeightText(values));
    	}
    }

    protected override void OnReadMemory(uint offset, byte[] data) {
    	if (offset == 0xfe) {
    		if (data.Length == 2 && data[0] == 4 && data[1] == 2) {
    			Debug.Log("Balance Board found.");
    		} else {
    			Debug.Log("Invalid extension controller found.");
    		}
    	} else if (offset == 0x24) {
    		calibZero_ = ConvertRawWeightData(data, 0);
			Debug.Log("Calibration data (0kg) received.");
    	} else if (offset == 0x2c) {
    		calib17kg_ = ConvertRawWeightData(data, 0);
			Debug.Log("Calibration data (17kg) received.");
    	} else if (offset == 0x34) {
    		calib34kg_ = ConvertRawWeightData(data, 0);
			Debug.Log("Calibration data (34kg) received.");
    	}
    }
};

public class BNCTest : MonoBehaviour {
	BNCTestClient client_;

	void Start() {
		client_ = new BNCTestClient();
		
		if (!client_.OpenPort(8000 + 1)) {
			Debug.Log("Driver not found.");
			client_ = null;
			return;
		}

		{
			byte[] data1 = { 0x55 };
			byte[] data2 = { 0x00 };
	        client_.RequestWriteMemory(0x4a400f0, data1);
	        client_.RequestWriteMemory(0x4a400fb, data2);
		}

	    client_.RequestReadMemory(0x4a400fe, 2);
	    client_.RequestReadMemory(0x4a40024, 8);
	    client_.RequestReadMemory(0x4a40024 + 8, 8);
	    client_.RequestReadMemory(0x4a40024 + 16, 8);

	    {
	        byte[] data = { 0xa2, 0x12, 0x00, 0x32 };
	        client_.SendData(data);
	    }
    }

	void Update() {
		if (client_ != null) client_.Update();
	}
}

# imu_client.py
import serial
import time
import socketio
import struct

# Global dictionary to store the latest parsed data
latest_imu_data = {
    "acc": {"x": 0.0, "y": 0.0, "z": 0.0},
    "gyro": {"x": 0.0, "y": 0.0, "z": 0.0},
    "angle": {"roll": 0.0, "pitch": 0.0, "yaw": 0.0}
}

sio = socketio.Client()

@sio.event
def connect():
    print("IMU Connected to Flask server!")

@sio.event
def disconnect():
    print("IMU Disconnected from Flask server.")

def parse_imu_data(raw_data):
    # This function is now specifically for the 0x61 protocol
    if len(raw_data) != 30 or raw_data[0] != 0x55 or raw_data[1] != 0x61:
        return None

    # Unpack the 13 raw sensor values (signed short integers, 2 bytes each)
    data_values = struct.unpack('<hhhhhhhhhhhhh', raw_data[2:28])

    latest_imu_data["acc"]["x"] = data_values[0] / 32768.0 * 16.0
    latest_imu_data["acc"]["y"] = data_values[1] / 32768.0 * 16.0
    latest_imu_data["acc"]["z"] = data_values[2] / 32768.0 * 16.0

    latest_imu_data["gyro"]["x"] = data_values[3] / 32768.0 * 2000.0
    latest_imu_data["gyro"]["y"] = data_values[4] / 32768.0 * 2000.0
    latest_imu_data["gyro"]["z"] = data_values[5] / 32768.0 * 2000.0

    latest_imu_data["angle"]["roll"] = data_values[6] / 32768.0 * 180.0
    latest_imu_data["angle"]["pitch"] = data_values[7] / 32768.0 * 180.0
    latest_imu_data["angle"]["yaw"] = data_values[8] / 32768.0 * 180.0
    
    return latest_imu_data

def read_and_send_imu_data():
    while True:
        try:
            ser = serial.Serial('COM3', 115200, timeout=1) 
            print("Connected to IMU sensor on COM3")
            
            # Flush the serial buffer to clear any old or corrupted data
            ser.flushInput()

            while True:
                # Read one byte at a time until we find the start of a packet (0x55)
                header_byte = ser.read(1)
                if not header_byte:
                    continue

                if header_byte[0] == 0x55:
                    # Found a header, now read the next 29 bytes to form a full 30-byte packet
                    raw_data = header_byte + ser.read(29)

                    # Print the raw data
                    print(f"Raw data from IMU: {raw_data.hex()}")

                    parsed_data = parse_imu_data(raw_data)
                    print(f"Parsed data: {parsed_data}")

                    if parsed_data:
                        sio.emit('imu_data', parsed_data)

        except serial.SerialException as e:
            print(f"Error: Could not open serial port: {e}. Retrying in 5 seconds...")
            ser.close()
            time.sleep(5)
        except Exception as e:
            print(f"An unexpected error occurred: {e}. Retrying in 5 seconds...")
            ser.close()
            time.sleep(5)

if __name__ == '__main__':
    try:
        sio.connect('http://localhost:5000')
        read_and_send_imu_data()
    except socketio.exceptions.ConnectionError:
        print("Could not connect to Flask server. Please ensure the server is running.")
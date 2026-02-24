from flask import Flask, render_template
from flask_socketio import SocketIO

app = Flask(__name__)
app.config['SECRET_KEY'] = 'your-secret-key'

socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet", logger=True, engineio_logger=True)

@app.route("/")
def index():
    return render_template("index.html")  # serve the HTML UI

@socketio.on("connect")
def handle_connect():
    print("Client connected")

@socketio.on("disconnect")
def handle_disconnect():
    print("Client disconnected")


@socketio.on("change_sampling_rate")
def handle_change_sampling_rate(data):
    try:
        rate = int(data['rate'])
        socketio.emit("update_rate", {"rate": rate})
    except (KeyError, ValueError) as e:
        print(f"Error processing sampling rate change: {e}")

@socketio.on("sensor_data")
def handle_sensor_data(data):
    print(f"Received from ESP32: {data}")
    socketio.emit("sensor_update", data, to=None) # Forward the FSR data to the frontend

@socketio.on("imu_data")
def handle_imu_data(data):
    print(f"Received from IMU client: {data}")
    socketio.emit("imu_update", data)   # Forward the IMU data to the frontend

if __name__ == "__main__":
    print("Starting Flask-SocketIO server on http://0.0.0.0:5000")
    socketio.run(app, host="0.0.0.0", port=5000)

# Project Setup & Run Guide

## 🔥 Run Project Using Mobile Hotspot

### 1️⃣ Start Hotspot on Your Smartphone
Turn on the mobile hotspot that will be used by:
- Arduino shoes
- Backend server system

All devices must be connected to the same hotspot network.

---

### 2️⃣ Get the Smartphone Hotspot IP Address
On the system that is connected to the hotspot, run:

**Windows:**
```bash
ipconfig
```

**Linux / macOS:**
```bash
ifconfig
```
or
```bash
hostname -I
```

Copy the **IPv4 address** → this is your server IP.

---

### 3️⃣ Update IP in Both Arduino Shoe Files
Open both Arduino files and replace the server IP.

```cpp
const char* server = "YOUR_HOTSPOT_IP";
```

Upload the code to both shoes.

---

### 4️⃣ (Optional) Verify Connected Devices
In your mobile hotspot settings, check the number of connected devices.

It should show:
```
2 connected devices
```
(Your two Arduino shoes)

---

### 5️⃣ Start the Backend Server
Navigate to the backend folder and run:

```bash
node server.js
```

---

### 6️⃣ Launch the UI
Open the browser and run the frontend using:

```bash
http://localhost:<PORT>
```

---

## ✅ Quick Flow
1. Start mobile hotspot
2. Get hotspot IP
3. Add IP in both Arduino files
4. Upload code to shoes
5. (Optional) Verify 2 connected devices
6. Run backend → `node server.js`
7. Launch UI on `localhost`


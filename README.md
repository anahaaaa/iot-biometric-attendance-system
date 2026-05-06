# IoT-Based Biometric Attendance System using ESP8266, MQTT, and AWS IoT

A secure, IoT-powered attendance tracking system using fingerprint recognition, MQTT communication, AWS cloud storage, and a web-based management interface. Built as part of the M.Tech program in Computer Science & Engineering (Data Science and AI) at Cochin University of Science and Technology.

![IoT](https://img.shields.io/badge/Domain-IoT-blue)
![AWS](https://img.shields.io/badge/Cloud-AWS-orange)
![MQTT](https://img.shields.io/badge/Protocol-MQTT-green)
![ESP8266](https://img.shields.io/badge/Board-ESP8266-red)
![Arduino](https://img.shields.io/badge/Built%20With-Arduino-00979D)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen)
---

## 📌 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Tech Stack](#tech-stack)
- [How It Works](#how-it-works)
- [Setup & Installation](#setup--installation)
- [Arduino Code Structure](#arduino-code-structure)
- [Web Interface](#web-interface)
- [Project Timeline](#project-timeline)
- [Authors](#authors)

---

## Overview

Traditional attendance methods — manual sign-ins or ID card swipes — are error-prone and easily manipulated. This system replaces them with biometric fingerprint recognition, providing tamper-proof, real-time attendance logging.

The system captures a fingerprint, publishes the data via MQTT to AWS IoT Core, stores records in a cloud-backed MySQL database, and surfaces everything through a web dashboard for administrators.

---

## Features

- **Biometric Authentication** — Unique fingerprint identification using the R307 sensor
- **Real-Time Data Transfer** — MQTT protocol for low-latency sensor-to-cloud communication
- **Timestamped Records** — Every attendance entry is logged with an exact date and time
- **Cloud Storage** — Secure, scalable storage via AWS IoT Core and MySQL
- **Web Dashboard** — Admin interface to add/remove users, view logs, and generate reports
- **Enrollment Flow** — Remote fingerprint enrollment triggered via MQTT message
- **Duplicate Detection** — Prevents re-enrollment of already registered fingerprints

---

## System Architecture

### High-Level Flow

```
Fingerprint Sensor (R307)
        │
        ▼
  NodeMCU (ESP8266)
        │  MQTT over TLS (port 8883)
        ▼
  AWS IoT Core
        │
        ▼
   MySQL Database
        │
        ▼
   Web Interface (Admin Dashboard)
```

### Low-Level Flow

```
User places finger on R307 Sensor
        │
        ▼
NodeMCU captures & processes fingerprint image
        │
        ▼
Fingerprint matched/enrolled → JSON payload created
        │
        ▼
Published to AWS IoT topic: user/fingerprint/response
        │
        ▼
AWS IoT Rule triggers → Data stored in MySQL
        │
        ▼
Admin views attendance via web interface
```

---

## Hardware Components

| Component | Description | Qty | Cost (INR) |
|---|---|---|---|
| NodeMCU (ESP8266) | Main microcontroller — handles fingerprint processing, WiFi, and MQTT | 1 | ₹394 |
| R307 Fingerprint Sensor | Captures and matches fingerprint templates | 1 | ₹1,080 |
| LED Display | Visual feedback for scan success/failure | 1 | ₹249 |
| Zero PCB | Hardware assembly board for component integration | 1 | ₹30 |
| **Total** | | | **₹1,753** |

---

## Tech Stack

| Layer | Technology |
|---|---|
| Microcontroller | NodeMCU (ESP8266) |
| Fingerprint Sensor | R307 (via Adafruit Fingerprint library) |
| Communication Protocol | MQTT over TLS (BearSSL) |
| Cloud Platform | AWS IoT Core |
| Database | MySQL |
| Backend/Web | Web-based admin interface |
| Libraries | `PubSubClient`, `ArduinoJson`, `Adafruit_Fingerprint`, `ESP8266WiFi`, `WiFiClientSecure` |

---

## How It Works

### Attendance Logging

1. User places their finger on the R307 sensor.
2. NodeMCU captures the fingerprint image and searches for a match.
3. On a successful match, the fingerprint ID is published as a JSON payload to the AWS IoT topic `user/fingerprint/response`.
4. AWS IoT Core routes the data to MySQL, where it is stored with a timestamp.
5. The web dashboard reflects the new attendance entry in real time.

### Fingerprint Enrollment

1. Admin sends an enrollment request via the web interface, which publishes a message to the MQTT topic `user/fingerprint/request` with `{"username": "enroll"}`.
2. NodeMCU receives the message and prompts the user to scan their finger twice.
3. A fingerprint model is created and stored in the sensor's memory.
4. The assigned fingerprint ID is published back to AWS.

### MQTT Topics

| Topic | Direction | Purpose |
|---|---|---|
| `user/fingerprint/response` | Device → Cloud | Publish fingerprint ID on scan |
| `user/fingerprint/request` | Cloud → Device | Trigger enrollment from web |

---

## Setup & Installation

### Prerequisites

- Arduino IDE with ESP8266 board support installed
- AWS account with IoT Core configured
- MQTT broker endpoint and certificates from AWS IoT
- MySQL database instance

### Arduino Libraries Required

Install the following via Arduino Library Manager:

```
Adafruit Fingerprint Sensor Library
PubSubClient
ArduinoJson
ESP8266WiFi (bundled with ESP8266 board package)
```

### Configuration

Create a `secrets.h` file in your Arduino sketch folder with your credentials:

```cpp
#define WIFI_SSID       "your_wifi_ssid"
#define WIFI_PASSWORD   "your_wifi_password"
#define MQTT_HOST       "your-iot-endpoint.amazonaws.com"
#define THINGNAME       "your_thing_name"

// AWS certificates
const char cacert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<your AWS root CA>
-----END CERTIFICATE-----
)EOF";

const char client_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<your device certificate>
-----END CERTIFICATE-----
)EOF";

const char privkey[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
<your private key>
-----END RSA PRIVATE KEY-----
)EOF";
```

### Wiring

| R307 Pin | NodeMCU Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| TX | D2 (GPIO4) |
| RX | D1 (GPIO5) |

### Upload & Run

1. Open the `.ino` sketch in Arduino IDE.
2. Select board: **NodeMCU 1.0 (ESP-12E Module)**
3. Set baud rate to **115200**
4. Upload the sketch and open Serial Monitor to verify connection.

---

## Arduino Code Structure

```
biometric-attendance/
├── biometric_attendance.ino   # Main sketch
├── secrets.h                  # WiFi & AWS credentials (do not commit)
└── README.md
```

Key functions:

| Function | Description |
|---|---|
| `connectAWS()` | Connects to WiFi, syncs NTP time, establishes MQTT connection |
| `messageReceived()` | Callback — handles incoming MQTT messages (e.g., enrollment trigger) |
| `enrollFingerprint()` | Captures and stores a new fingerprint template |
| `convertToTemplate()` | Two-scan fingerprint model creation |
| `publishFingerprintId()` | Publishes fingerprint ID as JSON to AWS IoT |
| `NTPConnect()` | Syncs device clock via NTP for accurate timestamps |

---

## Web Interface

The admin dashboard provides the following pages:

- **Login Page** — Secure admin login with credentials
- **Manage Users** — Add, update, and remove enrolled users
- **Attendance Log** — Filter and view attendance records by date and month
- **MySQL View** — Query-level visibility into the attendance database

---

## Project Timeline

| Week | Dates | Milestone |
|---|---|---|
| Week 1 | Aug 27 – Sep 6 | Cloud connectivity setup and MQTT demonstration |
| Week 2 | Sep 6 – Sep 25 | Fingerprint sensor data displayed on AWS cloud |
| Week 3 | Sep 25 – Oct 9 | Web interface development for attendance viewing |
| Week 4 | Oct 9 – Oct 23 | Cloud and attendance management system integration |
| Week 5 | Oct 23 – Nov 6 | Final testing, MySQL integration, and full deployment |

---

## Authors

**Anagha R S** and **Anna Sebastian**
M.Tech in Computer Science & Engineering (Data Science and AI)
Department of Computer Science & Engineering
Cochin University of Science and Technology

Under the guidance of **Dr. Bijoy A Jose**

*November 2024*

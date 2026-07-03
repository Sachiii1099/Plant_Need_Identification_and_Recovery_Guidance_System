# Plant_Need_Identification_and_Recovery_Guidance_System



##  Project Overview

The Smart Plant Health Monitoring System is an IoT-based solution designed to monitor plant environmental conditions in real time and estimate overall plant health.

Unlike traditional monitoring systems that react instantly to every sensor fluctuation, this project introduces a **time-aware health scoring approach** that mimics how plants actually respond to environmental changes.

Plants do not become unhealthy immediately when sunlight decreases for a few seconds or when soil moisture changes briefly. Therefore, this system uses trend-based monitoring and gradual score adjustments rather than instant penalties.

The system continuously collects data from multiple sensors and displays the plant's condition on an OLED display while calculating an overall Plant Health Score.

---

##  Problem Statement

Most low-cost plant monitoring systems rely on instant sensor readings.

This creates problems because:

* Plants do not react instantly to environmental changes.
* Short-term fluctuations can generate false alarms.
* Temporary darkness, temperature shifts, or watering events may incorrectly indicate poor health.

This project aims to create a smarter monitoring approach that better represents real plant behavior.

---

##  Proposed Solution

The system monitors:

* Soil Moisture
* Ambient Temperature
* Humidity
* Light Exposure

Instead of making decisions from a single reading, the system gradually adjusts the health score based on how long unfavorable conditions persist.

This creates a more realistic representation of plant health.

---

## 🔧 Hardware Components

| Component                 | Purpose                           |
| ------------------------- | --------------------------------- |
| ESP32                     | Main Microcontroller              |
| DHT22 Sensor              | Temperature & Humidity Monitoring |
| Soil Moisture Sensor      | Soil Water Content Detection      |
| LDR Light Sensor Module   | Light Exposure Detection          |
| OLED Display (SSD1306)    | Real-Time Information Display     |
| Breadboard & Jumper Wires | Circuit Connections               |

---

## ⚙️ Working Principle

### 1. Soil Moisture Monitoring

The soil sensor continuously measures moisture levels.

* Wet soil → Healthy score maintained
* Dry soil for extended periods → Score gradually decreases
* Watering restores the score gradually

This simulates how plants require water over time rather than immediately.

---

### 2. Light Monitoring

The LDR sensor detects available light.

To reduce fluctuations:

* Multiple readings are averaged.
* Brightness percentage is calculated.
* Temporary shadows do not instantly reduce plant health.

This mimics real plants that respond to accumulated light exposure rather than second-by-second changes.

---

### 3. Temperature Monitoring

Temperature is continuously checked.

Healthy range:

* 15°C – 35°C

Outside this range:

* Health score decreases gradually over time.

---

### 4. Humidity Monitoring

Humidity is monitored using the DHT22.

Healthy range:

* 30% – 80%

Extended periods outside this range reduce the health score.

---

## 🧠 Health Scoring System

Each parameter receives an individual score:

* Light Score
* Soil Score
* Temperature Score
* Humidity Score

The overall health score is calculated as:

Overall Health Score =
(Light Score + Soil Score + Temperature Score + Humidity Score) / 4

Score Range: 0 – 100

---

## 🌿 Plant Status Classification

| Score    | Status    |
| -------- | --------- |
| 90 – 100 | Excellent |
| 80 – 89  | Healthy   |
| 70 – 79  | Good      |
| 60 – 69  | Stable    |
| 50 – 59  | OK        |
| 40 – 49  | Stress    |
| 30 – 39  | Bad       |
| 20 – 29  | Critical  |
| 10 – 19  | Dying     |
| 0 – 9    | Dead      |

---

## 📟 OLED Display Output

The OLED display shows:

* Temperature
* Humidity
* Soil Moisture
* Light Percentage
* Overall Health Score
* Plant Status

Example:

T: 25.6 C
HUM: 58 %
SOIL: 1450
LIGHT: 80 %
SCORE: 92
STATUS: HEALTHY

---

## 🚀 Future Enhancements

### Phase 2.5

* Time-based environmental scoring
* Trend analysis
* Better plant behavior modeling

### Phase 3

* Machine Learning based plant health prediction
* Personalized plant recommendations
* Species-specific thresholds
* AI-powered care guidance
* Mobile application integration
* Cloud data storage and analytics

---

## 📈 Innovation

The key innovation of this project is that it attempts to model **plant response over time rather than reacting to instant sensor values.**

This makes the system:

* More realistic
* Less sensitive to noise
* Better suited for real-world plant monitoring

---

## 🛠️ Software & Libraries Used

* Arduino IDE
* ESP32 Board Package
* Adafruit SSD1306 Library
* Adafruit GFX Library
* DHT Sensor Library
* Wire Library

---

## 📷 Demonstration

The prototype demonstrates:

✅ Real-time environmental monitoring

✅ OLED-based visualization

✅ Dynamic plant health scoring

✅ Time-aware degradation and recovery logic

✅ Multi-sensor integration using ESP32

---

## 👨‍💻 Author

**Sachidanand Mandal**

B.Tech Student

IoT & Embedded Systems Enthusiast

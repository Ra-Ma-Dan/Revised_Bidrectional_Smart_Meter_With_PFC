# Smart-Energy-Meter-With-Power-Factor-Correction
An ESP-IDF 6.1.0 IoT Smart Energy Meter application utilizing the PZEM-004T sensor to measure live AC electrical telemetry. Features dynamic credit token balance enforcement, low-balance load shedding via an automatic relay trip interlock, and an optimized 4-bank Caps for Power Factor Correction (PFC) system and a Local web Dashboard

# IoT Smart Energy Meter & Automated PFC Controller

A production-grade, asynchronous firmware application built with **ESP-IDF 5.x** for the ESP32 platform. This project implements an advanced household digital prepay energy meter using the **PZEM-004T V3.0** sensor module. It natively tracks live electrical statistics, enforces prepaid credit caps, manages safety interlocks, and features a multi-stage **Binary Combinatorial Power Factor Correction (PFC)** engine managing 4 distinct unequal capacitor banks.

## 🚀 Key Features

*   **PZEM-004T Modbus Engine:** Low-overhead UART driver performing asynchronous telemetry collection (Voltage, Current, Active Power, Frequency, Accumulated Wh Energy, and Power Factor).
*   **Prepaid Credit Logic:** Dynamic deduction engine calculating usage costs in real-time. Automatically isolates line loads via a master high-power relay if balance hits the critical threshold (≤ ₦3.00).
*   **Combinatorial PFC (Power Factor Correction):** Evaluates all \(2^4 = 16\) possible hardware relay permutation states to accurately balance reactive power (\(\text{VAR}\)) deficits down to individual microfarad (\(\mu\text{F}\)) values without inducing leading line states.
*   **Embedded Web Dashboard:** Async JSON API server serving a fully responsive HTML5/CSS3/JS user interface embedded directly into the ESP32's flash array. Supports manual power overrides and secure digital credit keypad recharging.

---

## 🛠️ System Architecture & Pin Configurations

The firmware allocates isolated FreeRTOS tasks with asymmetric priority bands to ensure line safety structures maintain runtime execution dominance.

### Microcontroller Pin Mapping

| Component | ESP32 GPIO | Direction | Purpose |
| :--- | :---: | :---: | :--- |
| **PZEM-004T RX** | GPIO 16 | Input | Receives telemetry frames from the sensor |
| **PZEM-004T TX** | GPIO 17 | Output | Transmits Modbus command frames to the sensor |
| **Main Power Relay** | GPIO 18 | Output | Active-HIGH line master distribution contactor |
| **Capacitor Bank 1** | GPIO 19 | Output | Relay drive for Bank 1 (Lowest value) |
| **Capacitor Bank 2** | GPIO 21 | Output | Relay drive for Bank 2 |
| **Capacitor Bank 3** | GPIO 22 | Output | Relay drive for Bank 3 |
| **Capacitor Bank 4** | GPIO 23 | Output | Relay drive for Bank 4 (Highest value) |

---


📂 Project Structure

├── CMakeLists.txt                   # Top-level project CMake file
├── sdkconfig                        # Generated project configuration file
├── wokwi.toml                       # Wokwi file for configuration
├── diagram.json                     # Diagram file for Wokwi
│
├── main/
│   ├── CMakeLists.txt               # App component CMake file
│   ├── Kconfig                      # Menuconfig configuration options
│   └── main.c                       # Application entry point (FreeRTOS initialization)
│
├── components/                      # Low-level hardware drivers (Peripheral Control)
│   ├── my_gpio/
│   │   ├── CMakeLists.txt
│   │   ├── my_gpio.c                # Low-level GPIO & Relay abstraction
│   │   └── include/
│   │       └── my_gpio.h
│   ├── my_12c/                      # I2C driver for LCD
│   │   ├── CMakeLists.txt
│   │   ├── my_12c.c                 # I2C definitions and initialization
│   │   └── include/
│   │       └── my_12c.h
│   ├── my_nvs/                      # NVS folder
│   │   ├── CMakeLists.txt
│   │   ├── my_nvs.c
│   │   └── include/
│   │       └── my_nvs.h
│   ├── my_wifi/
│   │   ├── CMakeLists.txt
│   │   ├── my_wifi.c
│   │   └── include/
│   │       └── my_wifi.h
│   └── spiffs_storage/
│       ├── CMakeLists.txt
│       ├── spiffs_storage.c         # Local file storing and downloading
│       └── include/
│           └── spiffs_storage.h
│
├── peripherals/                     # High-level peripheral modules & tasks
│   ├── lcd_display/
│   │   ├── CMakeLists.txt
│   │   ├── lcd_display.c            # I2C 16x2 LCD screen initialization and definitions
│   │   └── include/
│   │       └── lcd_display.h
│   ├── pfc_task/
│   │   ├── CMakeLists.txt
│   │   ├── pfc_task.c               # Power factor correction task (capacitor mask switching)
│   │   └── include/
│   │       └── pfc_task.h
│   └── pzem_sensor/
│       ├── CMakeLists.txt
│       ├── pzem_sensor.c            # Main energy sensor (Current, Power, PF, Voltage, etc.)
│       └── include/
│           └── pzem_sensor.h
│
└── server/                          # Web server and frontend files
    ├── meter_http_server/
    │   ├── CMakeLists.txt
    │   ├── meter_http_server.c      # HTTP functions, URIs, and handlers for data transfer
    │   └── include/
    │       └── meter_http_server.h
    └── webpage/
        ├── index.html               # Markup for the local dashboard
        ├── style.css                # Styling file for the markup
        └── scripts.js               # JS file for dynamic web updates and local downloads

    
---

## ⚡ Firmware Operations & Task Loops

1. **`energy_sensor_task` (Priority 5):** Collects UART frames every 1,000ms. If load exists, it subtracts monetary values against `credit_balance` dynamically.
2. **`power_task` (Priority 5):** The safety interlock gatekeeper. Assesses balance requirements and `power_state` every 100ms. It instantly forces the relay open if funds deplete.
3. **`power_factor_correction_task` (Priority 4):** Measures complex phase drift ($\text{VAR}$) under loads $\ge 50\text{W}$. Resolves the optimal binary mask combo from a set of four unique non-identical capacitors to restore efficiency toward a $1.00$ target.

---

## 🔧 Building and Flashing

### Prerequisites
1. Install the official [ESP-IDF toolchain](https://espressif.com) (v6.1.0 or later recommended).
2. Wokwi extension in vscode for simulatoin if no hardware is available
3. Wire up your ESP32 devkit to the PZEM-004T module (Ensure the PZEM is powered with 5V VCC to drive its optical isolation chips, and use a pull-up resistor on the ESP32 TX line if frame timeouts happen). 
                            if real hardware is present, comment out **line 26** in main function 
                            else leave the random generator to get random energy value

### Compilation
Clone the repository and mount the build space environment:
```bash
git clone https://github.com/Ra-Ma-Dan/Smart-Energy-Meter-With-Power-Factor-Correction
cd New_PFC_Energy_Meter

# Set target microcontroller
idf.py set-target esp32

# Configure Wi-Fi credentials and ports via menuconfig menu
idf.py menuconfig

# Build, flash and open serial terminal monitor
idf.py build flash monitor

#open a brower on your system and head to your local host url e.g "http://localhost:8080/" and then enter
# you'll be asked for username and password, it is set in **scripts.js line 2** as
#                            username : ramadan
#                            password : 123456
```

---

## 📈 Frontend API Interface
The embedded dashboard communicates via the following endpoints:
*   `GET /data` - Returns standard JSON packet containing voltage, current, power, energy, frequency, power factor, credit, and an active capacitor selection bitmask.
*   `POST /api/power?state=[1|0]` - Overrides master power relay operation permissions.
*   `POST /api/recharge?amount=[value]` - Adds tokenized Naira funds to the local device memory stack.
*   `POST /api/reset` - Commands the PZEM IC register stack to flush accumulated historical Watt-hour counters down to zero.

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.

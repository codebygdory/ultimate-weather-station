# Multi-Sensor Meteorological Station with Rolling Trend Detection

An advanced firmware-driven weather tracking platform built on the ATmega328P architecture featuring real-time environmental math synthesis, rolling historical trend analysis, and a custom glyph-driven telemetry display.

## Technical Highlights

- **Real-Time Thermodynamic Synthesis:** Mathematically derives the localized Heat Index and Dew Point concurrently from raw ambient temperature and relative humidity inputs.
- **Rolling Trend Detection Filtering:** Utilizes a localized data buffer to compute a moving window average, filtering out raw sensor noise and capturing true environmental trajectory.
- **Custom LCD Glyph Visualization:** Modifies the HD44780 character generator RAM (CGRAM) at the driver level to render bespoke visual indicators representing climatic direction changes.

## System Architecture & Components

| Component | Function / Purpose | Interface / Protocol |
| :--- | :--- | :--- |
| **Arduino Uno** | Microcontroller handling calculation arrays and display updates | N/A |
| **DHT11 Sensor** | Captures ambient relative humidity and raw thermal metrics | Digital Single-Wire |
| **LCD Display (16x2)** | Displays real-time metrics, calculated indices, and custom graphics | I2C / Parallel |

## How to Replicate and Test

1. Clone this repository to your local machine.
2. Open your `.ino` file inside your development environment.
3. Wire the physical components based on your designated I/O pin configurations.
4. Flash the code to your ATmega328P / Arduino Uno.
5. Verify telemetry via the display or Serial Monitor.

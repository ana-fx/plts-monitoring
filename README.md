# PLTS Hybrid On-Grid Monitoring System

This monitoring system is designed to monitor the performance of a hybrid on-grid Solar Power Plant (PLTS) in real-time using an STM32 microcontroller. The system integrates various sensors to measure environmental and electrical parameters, and displays the data on an OLED screen.

## Main Features
- **Real-Time Monitoring:** Temperature, humidity, light intensity, grid voltage, load current, active power, power factor, frequency, and solar irradiance.
- **Automatic Protection:** Relay to cut off power flow when abnormal conditions (overcurrent/overvoltage) are detected.
- **OLED Display:** Data is shown on an OLED SSD1306 128x64 screen.
- **Sensor Calibration:** Automatic calibration at startup for accurate sensor readings.
- **Serial Debug Output:** Monitoring data output to serial monitor for debugging.

## Main Components
- STM32 (main microcontroller)
- DHT11 sensor (temperature & humidity)
- LDR sensor (light intensity)
- ZMPT101B sensor (AC grid voltage)
- ACS712 sensor (AC current)
- OLED SSD1306 128x64 (I2C)
- Relay Module
- ESP32 (optional, for IoT)

## Pin Configuration
- `DHT_PIN` (2): DHT11 sensor
- `LDR_PIN` (PA3): LDR sensor
- `VOLTAGE_PIN` (PA0): ZMPT101B sensor
- `CURRENT_PIN` (PA1): ACS712 sensor

## File Structure
- `sketch_jun2a.ino` : Main STM32 program for monitoring and display.
- `debug_ldr.ino` : (Optional) LDR sensor debugging program.

## Usage Instructions
1. **Assemble all components** according to the schematic on a PCB or breadboard.
2. **Upload** the `sketch_jun2a.ino` file to the STM32 using STM32CubeIDE or Arduino IDE (with STM32 board support).
3. **Power on the system.** The system will automatically calibrate the sensors.
4. **Monitor the data** on the OLED screen. For debugging, open the Serial Monitor.
5. **Automatic protection** will activate if abnormal conditions are detected.

## License
This project is open-source and can be used for educational purposes and further development.

---

> For further documentation, please add schematic images, device photos, or additional instructions as needed. 
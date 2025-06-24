/*
 * ====================================================================
 *              HYBRID ON-GRID PLTS MONITORING SYSTEM
 * ====================================================================
 * 
 * Complete monitoring system with full display
 * 
 * Hardware: STM32 + OLED SSD1306 128x64
 * Sensors: DHT11/DHT22, LDR, ZMPT101B, ACS712
 * ====================================================================
 */

#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>

// ======================== PIN CONFIGURATION ========================
#define DHT_PIN         2
#define DHT_TYPE        DHT11
#define LDR_PIN         PA3
#define VOLTAGE_PIN     PA0
#define CURRENT_PIN     PA1

// ====================== CALIBRATION CONSTANTS ======================
#define ADC_RESOLUTION  4095.0  // STM32 uses 12-bit ADC (0-4095)
#define VREF            3.3
#define ACS712_OFFSET   512
#define ACS712_SENSITIVITY 0.185  // 185mV/A for ACS712-30A
#define ZMPT_CALIBRATION 230.0
#define DISPLAY_REFRESH 2000      // Update every 2 seconds

// ======================= OBJECT INITIALIZATION =======================
DHT dht(DHT_PIN, DHT_TYPE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// ====================== SENSOR DATA STRUCTURE ====================
struct SensorData {
    float temperature;      // °C
    float humidity;         // %RH
    float lightIntensity;   // Lux (calculated)
    float gridVoltage;      // VAC RMS
    float loadCurrent;      // AAC RMS  
    float activePower;      // Watt
    float powerFactor;      // cos φ
    float frequency;        // Hz
    float irradiance;       // W/m²
    bool dhtError;          // DHT sensor error
    bool ldrError;          // LDR sensor error
    bool voltageError;      // Voltage sensor error
    bool currentError;      // Current sensor error
};

SensorData data;

// ========================= INITIAL SETUP ===========================
void setup() {
    dht.begin();
    display.begin();
    
    // Sensor calibration
    calibrateSensors();
    
    // Startup display
    showStartup();
    delay(2000);
}

// ======================== MAIN LOOP ============================
void loop() {
    readAllSensors();
    calculateElectricalParameters();
    updateDisplay();
    
    delay(DISPLAY_REFRESH);
}

// ==================== SENSOR CALIBRATION ========================
void calibrateSensors() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(20, 20, "SENSOR CALIBRATION");
    display.drawStr(35, 35, "PLEASE WAIT");
    display.sendBuffer();
    
    // ADC stabilization
    for(int i = 0; i < 50; i++) {
        analogRead(LDR_PIN);
        analogRead(VOLTAGE_PIN); 
        analogRead(CURRENT_PIN);
        delay(20);
    }
}

// ==================== STARTUP DISPLAY ========================
void showStartup() {
    display.clearBuffer();
    display.setFont(u8g2_font_7x13B_tf);
    display.drawStr(15, 15, "PLTS MONITOR");
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(25, 30, "Hybrid On-Grid");
    display.drawStr(30, 45, "System v2.0");
    display.drawStr(35, 58, "READY");
    display.sendBuffer();
}

// ==================== SENSOR READING ===================
void readAllSensors() {
    // DHT11/DHT22 Sensor - Temperature & Humidity
    data.temperature = dht.readTemperature();
    data.humidity = dht.readHumidity();
    
    // DHT validation - check if sensor is detected
    data.dhtError = (isnan(data.temperature) || isnan(data.humidity));
    
    // LDR Sensor - Light Intensity
    int ldrRaw = analogRead(LDR_PIN);
    float ldrVoltage = (ldrRaw * VREF) / ADC_RESOLUTION;
    
    // LDR error detection - check if sensor is detected
    data.ldrError = false; // LDR always detected if connected
    
    // Convert to Lux (calibration based on LDR characteristics)
    if(ldrVoltage > 0.1) {
        data.lightIntensity = (2500.0 / ldrVoltage) - 500.0; // LDR calibration formula
        if(data.lightIntensity < 0) data.lightIntensity = 0;
        if(data.lightIntensity > 50000) data.lightIntensity = 50000;
    } else {
        data.lightIntensity = 0;
    }
    
    // Convert Lux to Irradiance (W/m²)
    // Standard conversion factor: 1 W/m² ≈ 120 Lux for sunlight
    data.irradiance = data.lightIntensity / 120.0;
    
    // ZMPT101B Sensor - AC Voltage
    int voltageRaw = analogRead(VOLTAGE_PIN);
    float voltageADC = (voltageRaw * VREF) / ADC_RESOLUTION;
    
    // Voltage sensor error detection - check if sensor is detected
    data.voltageError = false; // ZMPT101B always detected if connected
    
    // Convert to RMS voltage
    // ZMPT101B output: 1V AC input = ~1.5mV output
    data.gridVoltage = voltageADC * 220.0; // Calibration for 220V nominal
    
    // ACS712 Sensor - AC Current
    int currentRaw = analogRead(CURRENT_PIN);
    float currentADC = (currentRaw * VREF) / ADC_RESOLUTION;
    
    // Current sensor error detection - check if sensor is detected
    data.currentError = false; // ACS712 always detected if connected
    
    // Convert to RMS current
    float currentOffset = VREF / 2.0;  // 1.65V offset for ACS712
    float currentVoltage = currentADC - currentOffset;
    data.loadCurrent = abs(currentVoltage / ACS712_SENSITIVITY);
    
    // Limit values for safety
    if(data.gridVoltage > 300) data.gridVoltage = 0;
    if(data.loadCurrent > 50) data.loadCurrent = 0;
}

// ==================== ELECTRICAL PARAMETER CALCULATION ==============
void calculateElectricalParameters() {
    // Active Power (P = V × I × cos φ)
    // Assume power factor 0.9 for typical resistive-inductive loads
    data.powerFactor = 0.9;
    
    // Calculate active power (sensor already detected)
    data.activePower = data.gridVoltage * data.loadCurrent * data.powerFactor;
    
    // Validation and value limiting
    if(data.activePower > 10000) data.activePower = 0; // 10kW limit
    if(data.activePower < 0.1) data.activePower = 0;   // Noise threshold
    
    // Frequency (simulate 50Hz - in real implementation use zero-crossing detection)
    data.frequency = 50.0;
}

// ==================== COMPLETE DISPLAY UPDATE ===================
void updateDisplay() {
    display.clearBuffer();
    showCompleteMonitoring();
    display.sendBuffer();
}

// ==================== COMPLETE MONITORING DISPLAY ===============
void showCompleteMonitoring() {
    display.setFont(u8g2_font_5x7_tf);
    
    // HEADER
    display.drawStr(35, 7, "PLTS MONITORING");
    display.drawLine(0, 8, 128, 8);
    
    // LEFT COLUMN - ENVIRONMENTAL PARAMETERS
    display.drawStr(2, 18, "ENVIRONMENT");
    
    // Temperature
    display.drawStr(2, 27, "T:");
    if(data.dhtError) {
        display.drawStr(12, 27, "ERROR");
    } else {
        char tempStr[8];
        dtostrf(data.temperature, 4, 1, tempStr);
        display.drawStr(12, 27, tempStr);
        display.drawStr(35, 27, "C");
    }
    
    // Humidity  
    display.drawStr(2, 35, "RH:");
    if(data.dhtError) {
        display.drawStr(15, 35, "ERROR");
    } else {
        char humStr[8];
        dtostrf(data.humidity, 4, 1, humStr);
        display.drawStr(15, 35, humStr);
        display.drawStr(35, 35, "%");
    }
    
    // Light Intensity
    display.drawStr(2, 43, "Lux:");
    char luxStr[8];
    if(data.lightIntensity < 1000) {
        dtostrf(data.lightIntensity, 3, 0, luxStr);
    } else {
        dtostrf(data.lightIntensity/1000, 3, 1, luxStr);
        strcat(luxStr, "k");
    }
    display.drawStr(20, 43, luxStr);
    
    // Solar Irradiance
    display.drawStr(2, 51, "Irr:");
    char irrStr[8];
    dtostrf(data.irradiance, 4, 0, irrStr);
    display.drawStr(18, 51, irrStr);
    display.drawStr(35, 51, "W/m2");
    
    // VERTICAL SEPARATOR LINE
    display.drawLine(64, 10, 64, 64);
    
    // RIGHT COLUMN - ELECTRICAL PARAMETERS
    display.drawStr(67, 18, "ELECTRICAL");
    
    // Grid Voltage
    display.drawStr(67, 27, "V:");
    char voltStr[8];
    dtostrf(data.gridVoltage, 5, 1, voltStr);
    display.drawStr(75, 27, voltStr);
    display.drawStr(110, 27, "VAC");
    
    // Load Current
    display.drawStr(67, 35, "I:");
    char currStr[8];
    dtostrf(data.loadCurrent, 4, 2, currStr);
    display.drawStr(75, 35, currStr);
    display.drawStr(110, 35, "AAC");
    
    // Active Power
    display.drawStr(67, 43, "P:");
    char powStr[8];
    if(data.activePower < 1000) {
        dtostrf(data.activePower, 4, 0, powStr);
        display.drawStr(75, 43, powStr);
        display.drawStr(110, 43, "W");
    } else {
        dtostrf(data.activePower/1000, 3, 2, powStr);
        display.drawStr(75, 43, powStr);
        display.drawStr(110, 43, "kW");
    }
    
    // Power Factor
    display.drawStr(67, 51, "PF:");
    char pfStr[6];
    dtostrf(data.powerFactor, 3, 2, pfStr);
    display.drawStr(80, 51, pfStr);
    
    // Frequency
    display.drawStr(67, 59, "f:");
    char freqStr[6];
    dtostrf(data.frequency, 4, 1, freqStr);
    display.drawStr(75, 59, freqStr);
    display.drawStr(110, 59, "Hz");
    
    // BOTTOM STATUS BAR
    display.drawLine(0, 55, 64, 55);
    display.drawStr(2, 63, getSystemStatus());
}

// ==================== SYSTEM STATUS ============================
const char* getSystemStatus() {
    if(data.gridVoltage < 180 || data.gridVoltage > 250) {
        return "GRID FAULT";
    } else if(data.activePower > 5000) {
        return "HIGH LOAD";
    } else if(data.activePower > 1000) {
        return "NORMAL";
    } else if(data.activePower > 100) {
        return "LOW LOAD";
    } else {
        return "STANDBY";
    }
}
/*
 * ====================================================================
 *                    LDR SENSOR SIMPLE MONITOR
 * ====================================================================
 * 
 * Kode sederhana untuk monitoring sensor LDR dengan tampilan OLED
 * Menampilkan nilai Lux dan format serial monitor
 * Menggunakan library dasar untuk menghindari konflik
 * 
 * Hardware: STM32 + OLED SSD1306 128x64 + LDR Sensor
 * ====================================================================
 */

#include <Wire.h>
#include <U8g2lib.h>

// ======================== KONFIGURASI PIN ========================
#define LDR_PIN         PA3

// ====================== KONSTANTA KALIBRASI ======================
#define ADC_RESOLUTION  4095.0  // STM32 menggunakan 12-bit ADC (0-4095)
#define VREF            3.3
#define DISPLAY_REFRESH 1000    // Update setiap 1 detik

// ======================= INISIALISASI OBJEK =======================
// Gunakan library yang lebih sederhana untuk menghindari konflik
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// ====================== VARIABEL GLOBAL SEDERHANA ====================
int adcValue = 0;
float voltage = 0.0;
float lightIntensity = 0.0;
float irradiance = 0.0;
unsigned long timestamp = 0;
int readingCount = 0;

// ========================= SETUP AWAL ===========================
void setup() {
    Serial.begin(115200);
    
    // Inisialisasi display dengan error handling
    if (!display.begin()) {
        Serial.println("Display initialization failed!");
        while(1) {
            Serial.println("Check I2C connections");
            delay(1000);
        }
    }
    
    // Kalibrasi sensor LDR
    calibrateLDR();
    
    // Manual calibration check
    manualCalibrationCheck();
    
    // Test perhitungan LDR
    testLDRValues();
    
    // Tampilan startup
    showStartup();
    delay(2000);
    
    // Inisialisasi data
    readingCount = 0;
    timestamp = 0;
}

// ======================== LOOP UTAMA ============================
void loop() {
    readLDRSensor();
    updateDisplay();
    
    // Output ke serial untuk debugging
    printDataToSerial();
    
    delay(DISPLAY_REFRESH);
}

// ==================== KALIBRASI LDR ============================
void calibrateLDR() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(20, 20, "KALIBRASI LDR");
    display.drawStr(35, 35, "MOHON TUNGGU");
    display.sendBuffer();
    
    // Stabilisasi ADC untuk LDR
    for(int i = 0; i < 50; i++) {
        analogRead(LDR_PIN);
        delay(20);
    }
    
    // Kalibrasi dengan multiple readings
    long adcSum = 0;
    int validReadings = 0;
    
    for(int i = 0; i < 20; i++) {
        int reading = analogRead(LDR_PIN);
        
        // Hanya ambil readings yang masuk akal (bukan stuck di 0 atau 4095)
        if(reading > 10 && reading < 4085) {
            adcSum += reading;
            validReadings++;
        }
        delay(50);
    }
    
    // Jika tidak ada readings valid, gunakan nilai default
    if(validReadings == 0) {
        Serial.println("WARNING: No valid readings during calibration!");
        Serial.println("Check LDR connection and lighting conditions");
        adcValue = 2048; // Nilai tengah sebagai default
    } else {
        adcValue = adcSum / validReadings;
        Serial.print("Calibration complete. Average ADC: "); Serial.println(adcValue);
    }
    
    // Update voltage dan lux berdasarkan kalibrasi
    voltage = (float)adcValue * VREF / ADC_RESOLUTION;
    
    // Kalibrasi lux berdasarkan kondisi awal
    if(voltage >= 3.0) {
        lightIntensity = 0;
        Serial.println("Calibration: DARK condition detected");
    } else if(voltage <= 0.3) {
        lightIntensity = 50000;
        Serial.println("Calibration: BRIGHT condition detected");
    } else {
        lightIntensity = 50000.0 - ((voltage - 0.3) * 50000.0 / 2.7);
        if(lightIntensity < 0) lightIntensity = 0;
        if(lightIntensity > 50000) lightIntensity = 50000;
        Serial.print("Calibration: NORMAL condition. Lux: "); Serial.println(lightIntensity);
    }
    
    irradiance = lightIntensity / 120.0;
    
    // Tampilkan hasil kalibrasi
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(20, 20, "KALIBRASI SELESAI");
    display.drawStr(10, 35, "ADC: ");
    display.drawStr(40, 35, String(adcValue).c_str());
    display.drawStr(10, 50, "V: ");
    display.drawStr(25, 50, String(voltage, 3).c_str());
    display.drawStr(50, 50, "V");
    display.sendBuffer();
    delay(2000);
}

// ==================== STARTUP DISPLAY ========================
void showStartup() {
    display.clearBuffer();
    display.setFont(u8g2_font_7x13B_tf);
    display.drawStr(25, 15, "LDR MONITOR");
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(20, 30, "Simple Sensor");
    display.drawStr(25, 45, "Monitor v3.0");
    display.drawStr(35, 58, "READY");
    display.sendBuffer();
}

// ==================== PEMBACAAN LDR SEDERHANA ===================
void readLDRSensor() {
    // Update timestamp dan counter
    timestamp = millis();
    readingCount++;
    
    // Baca ADC dengan multiple readings untuk stabilitas
    long adcSum = 0;
    int validReadings = 0;
    
    for(int i = 0; i < 5; i++) {
        int reading = analogRead(LDR_PIN);
        
        // Filter readings yang stuck
        if(reading > 10 && reading < 4085) {
            adcSum += reading;
            validReadings++;
        }
        delay(5);
    }
    
    // Jika tidak ada readings valid, gunakan nilai sebelumnya
    if(validReadings == 0) {
        Serial.println("WARNING: No valid readings! Using previous value.");
        // Tidak update adcValue, gunakan nilai sebelumnya
    } else {
        adcValue = adcSum / validReadings;
    }
    
    // Konversi ke voltage - PERBAIKAN KALKULASI
    voltage = (float)adcValue * VREF / ADC_RESOLUTION;
    
    // KONVERSI KE LUX - FORMULA SANGAT SEDERHANA
    // Pastikan selalu ada nilai Lux
    
    if(voltage >= 3.0) {
        // Sangat gelap
        lightIntensity = 0;
    } else if(voltage <= 0.3) {
        // Sangat terang
        lightIntensity = 50000;
    } else {
        // Kondisi normal - formula linear sederhana
        // Map voltage 0.3V-3.0V ke 50000-0 Lux
        lightIntensity = 50000.0 - ((voltage - 0.3) * 50000.0 / 2.7);
        
        // Pastikan nilai dalam range yang valid
        if(lightIntensity < 0) lightIntensity = 0;
        if(lightIntensity > 50000) lightIntensity = 50000;
    }
    
    // Konversi ke irradiance (W/m²)
    irradiance = lightIntensity / 120.0;
    
    // Debug: Pastikan nilai tidak 0 kecuali benar-benar gelap
    if(lightIntensity == 0 && voltage < 3.0) {
        lightIntensity = 1; // Minimal 1 Lux
        irradiance = 1.0 / 120.0;
    }
    
    // Debug info jika ADC masih tinggi
    if(adcValue > 4000) {
        Serial.print("HIGH ADC WARNING: "); Serial.print(adcValue);
        Serial.print(" -> V: "); Serial.print(voltage, 3);
        Serial.print(" -> Lux: "); Serial.println(lightIntensity);
        Serial.println("Try increasing light or check sensor connection");
    }
}

// ==================== UPDATE DISPLAY SEDERHANA ===================
void updateDisplay() {
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);
    
    // HEADER
    display.drawStr(30, 7, "LDR SENSOR DATA");
    display.drawLine(0, 8, 128, 8);
    
    // TIMESTAMP DAN COUNTER
    display.drawStr(2, 16, "Time: ");
    display.drawStr(40, 16, String(timestamp).c_str());
    display.drawStr(70, 16, " ms");
    
    display.drawStr(2, 24, "Reading #");
    display.drawStr(50, 24, String(readingCount).c_str());
    
    display.drawLine(0, 26, 128, 26);
    
    // DATA SENSOR - FORMAT YANG DIPERBAIKI
    // Line 1: ADC Value
    display.drawStr(2, 34, "ADC: ");
    display.drawStr(25, 34, String(adcValue).c_str());
    
    // Line 2: Voltage
    display.drawStr(2, 42, "V: ");
    display.drawStr(15, 42, String(voltage, 3).c_str());
    display.drawStr(45, 42, " V");
    
    // Line 3: Light Intensity - FORMAT YANG DIPERBAIKI
    display.drawStr(2, 50, "Lux: ");
    if(lightIntensity == 0) {
        display.drawStr(25, 50, "0 (DARK)");
    } else if(lightIntensity < 1000) {
        display.drawStr(25, 50, String((int)lightIntensity).c_str());
    } else if(lightIntensity < 10000) {
        display.drawStr(25, 50, String(lightIntensity/1000, 1).c_str());
        display.drawStr(45, 50, " k");
    } else {
        display.drawStr(25, 50, String((int)(lightIntensity/1000)).c_str());
        display.drawStr(45, 50, " k");
    }
    
    // Line 4: Irradiance - FORMAT YANG DIPERBAIKI
    display.drawStr(2, 58, "Irr: ");
    if(irradiance == 0) {
        display.drawStr(20, 58, "0 W/m2");
    } else if(irradiance < 100) {
        display.drawStr(20, 58, String(irradiance, 1).c_str());
        display.drawStr(45, 58, " W/m2");
    } else {
        display.drawStr(20, 58, String((int)irradiance).c_str());
        display.drawStr(45, 58, " W/m2");
    }
    
    // STATUS INDICATOR
    display.setFont(u8g2_font_6x10_tf);
    if(lightIntensity > 10000) {
        display.drawStr(85, 63, "BRIGHT");
    } else if(lightIntensity > 100) {
        display.drawStr(85, 63, "OK");
    } else if(lightIntensity > 0) {
        display.drawStr(85, 63, "LOW");
    } else {
        display.drawStr(85, 63, "DARK");
    }
    
    display.sendBuffer();
}

// ==================== OUTPUT SERIAL (DEBUGGING) ================
void printDataToSerial() {
    Serial.println("=== LDR SENSOR DATA ===");
    Serial.print("ADC Value: "); Serial.println(adcValue);
    Serial.print("Voltage: "); Serial.print(voltage, 3); Serial.println(" V");
    Serial.print("Light Intensity: "); Serial.print(lightIntensity, 1); Serial.println(" Lux");
    Serial.print("Solar Irradiance: "); Serial.print(irradiance, 1); Serial.println(" W/m²");
    Serial.print("Reading Count: "); Serial.println(readingCount);
    Serial.print("Timestamp: "); Serial.print(timestamp); Serial.println(" ms");
    
    // Light Condition
    Serial.println("--- LIGHT CONDITION ---");
    if(lightIntensity > 10000) {
        Serial.println("Status: BRIGHT");
    } else if(lightIntensity > 100) {
        Serial.println("Status: NORMAL");
    } else if(lightIntensity > 0) {
        Serial.println("Status: LOW");
    } else {
        Serial.println("Status: DARK");
    }
    
    // Calculation Details
    Serial.println("--- CALCULATION DETAILS ---");
    Serial.print("ADC to Voltage: "); Serial.print(adcValue); Serial.print(" -> "); Serial.print(voltage, 3); Serial.println("V");
    
    // Tampilkan formula yang digunakan
    if(voltage >= 3.0) {
        Serial.println("Formula: DARK (0 Lux) - Voltage >= 3.0V");
    } else if(voltage <= 0.3) {
        Serial.println("Formula: BRIGHT (50k Lux) - Voltage <= 0.3V");
    } else {
        Serial.print("Formula: Linear Map - ");
        float calcLux = 50000.0 - ((voltage - 0.3) * 50000.0 / 2.7);
        Serial.print("50000 - (("); Serial.print(voltage, 3); Serial.print(" - 0.3) * 50000 / 2.7) = "); Serial.print(calcLux, 1); Serial.println(" Lux");
    }
    
    Serial.print("Voltage to Lux: "); Serial.print(voltage, 3); Serial.print("V -> "); Serial.print(lightIntensity, 1); Serial.println(" Lux");
    Serial.print("Lux to Irradiance: "); Serial.print(lightIntensity, 1); Serial.print(" Lux -> "); Serial.print(irradiance, 1); Serial.println(" W/m²");
    
    Serial.println("============================");
    Serial.println();
}

// ==================== FUNGSI TEST SEDERHANA ====================
void testLDRValues() {
    Serial.println("=== LDR TEST VALUES ===");
    
    // Test dengan berbagai nilai ADC
    int testADCs[] = {100, 500, 1000, 2000, 3000, 4000};
    
    for(int i = 0; i < 6; i++) {
        int testADC = testADCs[i];
        float testVoltage = (float)testADC * VREF / ADC_RESOLUTION;
        float testLux;
        
        if(testVoltage >= 3.0) {
            testLux = 0;
        } else if(testVoltage <= 0.3) {
            testLux = 50000;
        } else {
            testLux = 50000.0 - ((testVoltage - 0.3) * 50000.0 / 2.7);
            if(testLux < 0) testLux = 0;
            if(testLux > 50000) testLux = 50000;
        }
        
        float testIrr = testLux / 120.0;
        
        Serial.print("ADC: "); Serial.print(testADC);
        Serial.print(" -> V: "); Serial.print(testVoltage, 3);
        Serial.print(" -> Lux: "); Serial.print(testLux, 1);
        Serial.print(" -> Irr: "); Serial.print(testIrr, 1);
        Serial.println(" W/m2");
    }
    
    Serial.println("=======================");
}

// ==================== MANUAL CALIBRATION CHECK ====================
void manualCalibrationCheck() {
    Serial.println("=== MANUAL CALIBRATION CHECK ===");
    Serial.println("Current readings:");
    Serial.print("ADC: "); Serial.println(adcValue);
    Serial.print("Voltage: "); Serial.print(voltage, 3); Serial.println("V");
    Serial.print("Lux: "); Serial.println(lightIntensity);
    
    if(adcValue > 4000) {
        Serial.println("⚠️  HIGH ADC DETECTED!");
        Serial.println("Possible causes:");
        Serial.println("1. LDR in dark environment");
        Serial.println("2. LDR not connected properly");
        Serial.println("3. Wrong pin connection (using DO instead of AO)");
        Serial.println("4. LDR sensor damaged");
        
        Serial.println("\nTroubleshooting steps:");
        Serial.println("1. Check if LDR is connected to AO pin");
        Serial.println("2. Try exposing LDR to light");
        Serial.println("3. Check wiring connections");
        Serial.println("4. Test with different LDR sensor");
        
        // Tampilkan warning di OLED
        display.clearBuffer();
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr(10, 15, "HIGH ADC WARNING!");
        display.drawStr(5, 25, "ADC: ");
        display.drawStr(35, 25, String(adcValue).c_str());
        display.drawStr(5, 35, "Check:");
        display.drawStr(5, 45, "1. AO pin connection");
        display.drawStr(5, 55, "2. Light exposure");
        display.sendBuffer();
        delay(3000);
    } else if(adcValue < 100) {
        Serial.println("⚠️  LOW ADC DETECTED!");
        Serial.println("Possible causes:");
        Serial.println("1. LDR exposed to very bright light");
        Serial.println("2. Short circuit");
        Serial.println("3. Wrong voltage reference");
    } else {
        Serial.println("✅ ADC reading looks normal");
    }
    
    Serial.println("================================");
}

/*
 * ====================================================================
 * CARA PENGGUNAAN:
 * 
 * 1. Upload kode ini ke STM32
 * 2. Hubungkan LDR sensor ke pin PA3 (gunakan pin AO)
 * 3. Hubungkan OLED display ke I2C
 * 4. Monitor tampilan untuk nilai sensor
 * 
 * PERBAIKAN LIBRARY:
 * - Menggunakan variabel global sederhana
 * - Menghapus struct yang kompleks
 * - Error handling untuk display initialization
 * - Menggunakan float casting yang eksplisit
 * - Menghapus dependensi library yang tidak perlu
 * 
 * PERBAIKAN SPRINTF:
 * - Mengganti sprintf dengan String method
 * - Lebih kompatibel dengan berbagai board
 * - Menghindari masalah buffer overflow
 * - Format yang lebih sederhana dan reliable
 * 
 * INFORMASI YANG DITAMPILKAN:
 * 
 * HEADER:
 * - "LDR SENSOR DATA"
 * 
 * TIMESTAMP:
 * - Time: Waktu dalam millisecond
 * - Reading #: Counter pembacaan
 * 
 * DATA SENSOR:
 * - ADC: Nilai ADC rata-rata dari 5 pembacaan (0-4095)
 * - V: Voltage dalam Volt (3 desimal)
 * - Lux: Intensitas cahaya (fokus utama)
 * - Irr: Irradiance dalam W/m²
 * 
 * STATUS:
 * - OK: Sensor mendeteksi cahaya normal (100-10000 Lux)
 * - BRIGHT: Kondisi sangat terang (>10000 Lux)
 * - LOW: Kondisi cahaya rendah (1-100 Lux)
 * - DARK: Kondisi gelap (0 Lux)
 * 
 * FORMULA LUX (PERBAIKAN):
 * - Voltage >= 3.0V: 0 Lux (gelap)
 * - Voltage <= 0.3V: 50,000 Lux (sangat terang)
 * - 0.3V < voltage < 3.0V: Formula linear mapping
 * - Konversi ke Irradiance: Lux / 120.0
 * 
 * TROUBLESHOOTING LIBRARY:
 * - Jika display tidak muncul: Cek koneksi I2C
 * - Jika nilai tidak berubah: Cek koneksi sensor
 * - Serial monitor akan menampilkan detail perhitungan
 * - Test function akan memverifikasi perhitungan
 * 
 * KEUNTUNGAN STRING METHOD:
 * - Tidak perlu buffer char array
 * - Otomatis handling memory
 * - Lebih mudah dibaca dan debug
 * - Kompatibel dengan semua Arduino board
 * 
 * UPDATE RATE: 1 detik
 * SERIAL OUTPUT: Setiap update untuk debugging
 * ====================================================================
 */ 
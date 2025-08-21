#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin definitions
const int PDA20H_PIN = A0;        // Analog pin for PDA20H sensor
const int LED_GREEN = D5;         // Green LED for normal operation
const int LED_RED = D6;           // Red LED for high methane alert
const int BUZZER_PIN = D7;        // Buzzer for audio alert

// Sensor calibration constants
const float REFERENCE_VOLTAGE = 3.3;
const int ADC_RESOLUTION = 1024;
const float BASELINE_VOLTAGE = 0.5;   // Baseline in clean air
const float SENSITIVITY_FACTOR = 0.02; // V/ppm
const float MIN_DETECTION_PPM = 5.0;
const float ALARM_THRESHOLD = 50.0;

// Measurement variables
float currentVoltage = 0.0;
float methanePPM = 0.0;
unsigned long lastReading = 0;
const unsigned long READING_INTERVAL = 1000; // 1 second

// Data storage for trend analysis
const int MAX_READINGS = 100;
float ppmReadings[MAX_READINGS];
unsigned long timestamps[MAX_READINGS];
int readingIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\nThaparSat ELC - ESP8266 + PDA20H Methane Detection System");

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  initializeWiFi();
  calibrateSensor();

  Serial.println("System initialized! Methane detection active with 5ppm sensitivity");
}

void loop() {
  if (millis() - lastReading >= READING_INTERVAL) {
    readPDA20HSensor();
    processReading();
    checkAlarmConditions();
    storeReading();
    lastReading = millis();
  }
  delay(100);
}

void initializeWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void readPDA20HSensor() {
  int rawADC = analogRead(PDA20H_PIN);
  currentVoltage = (rawADC * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
  methanePPM = voltageToMethane(currentVoltage);
  if (methanePPM < MIN_DETECTION_PPM) methanePPM = 0.0;
}

float voltageToMethane(float voltage) {
  if (voltage <= BASELINE_VOLTAGE) return 0.0;
  float voltageDiff = voltage - BASELINE_VOLTAGE;
  float ppm = voltageDiff / SENSITIVITY_FACTOR;
  return max(0.0, ppm);
}

void processReading() {
  Serial.print("Time: ");
  Serial.print(millis());
  Serial.print(" ms | Voltage: ");
  Serial.print(currentVoltage, 3);
  Serial.print(" V | Methane: ");
  Serial.print(methanePPM, 2);
  Serial.println(" ppm");
}

bool alarmActive = false;
void checkAlarmConditions() {
  if (methanePPM >= ALARM_THRESHOLD && !alarmActive) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    tone(BUZZER_PIN, 1000);
    alarmActive = true;
    Serial.println("*** ALARM: High methane detected! ***");
  } else if (methanePPM < ALARM_THRESHOLD && alarmActive) {
    noTone(BUZZER_PIN);
    alarmActive = false;
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else if (methanePPM >= MIN_DETECTION_PPM) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  }
}

void storeReading() {
  ppmReadings[readingIndex] = methanePPM;
  timestamps[readingIndex] = millis();
  readingIndex = (readingIndex + 1) % MAX_READINGS;
}

void calibrateSensor() {
  Serial.println("\nPerforming sensor calibration... Ensure clean air environment");
  delay(5000); // Stabilize sensor

  float calibrationSum = 0.0;
  int calibrationSamples = 50;
  for (int i = 0; i < calibrationSamples; i++) {
    int rawADC = analogRead(PDA20H_PIN);
    float voltage = (rawADC * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
    calibrationSum += voltage;
    delay(100);
  }

  float averageBaseline = calibrationSum / calibrationSamples;
  Serial.print("Baseline voltage calibrated: ");
  Serial.print(averageBaseline, 3);
  Serial.println(" V");
  Serial.println("Calibration complete!");
}

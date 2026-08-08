// Include necessary libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

//--- PIN DEFINITIONS ---//
// Sensors
#define LDR_PIN A0
#define DHT_PIN 2
#define SMOKE_SENSOR_PIN A1
#define IR_SENSOR_PIN 3
#define TRIG_PIN 8
#define ECHO_PIN 9
// Actuators & Inputs
#define BUZZER_PIN 10
#define RESET_BUTTON_PIN 4
#define FAN_MOTOR_RELAY_PIN 6 // Relay IN1 for 12V Fan Motor
#define PUMP_MOTOR_RELAY_PIN 7 // Relay IN2 for 3V Pump Motor

//--- SENSOR & MODULE SETUP ---//
// LCD setup (address 0x27 is common, but may vary. Use an I2C scanner if it doesn't work)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DHT Sensor setup
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

//--- PROJECT THRESHOLDS & SETTINGS (ADJUST THESE VALUES) ---//
const int LDR_ACTIVATION_THRESHOLD = 750; // Value below which the system activates (in darkness)
const float TEMPERATURE_THRESHOLD = 28.0; // Temperature in Celsius to turn on the fan
const int SMOKE_THRESHOLD = 400;          // Analog value from MQ-2 to trigger alarm
const float TANK_HEIGHT_CM = 20.0;        // Total height of your water tank in cm

//--- GLOBAL VARIABLES ---//
bool isSystemActive = false;
bool isSmokeAlarmActive = false;

void setup() {
  // Start serial communication for debugging
  Serial.begin(9600);

  // Initialize LCD and DHT Sensor
  lcd.init();
  lcd.backlight();
  dht.begin();

  // Set pin modes for all components
  pinMode(LDR_PIN, INPUT);
  pinMode(SMOKE_SENSOR_PIN, INPUT);
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP); // Use internal pull-up resistor

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  pinMode(FAN_MOTOR_RELAY_PIN, OUTPUT);
  pinMode(PUMP_MOTOR_RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Ensure all actuators are off at the start
  digitalWrite(FAN_MOTOR_RELAY_PIN, HIGH); // Relays are often active-LOW
  digitalWrite(PUMP_MOTOR_RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  // Initial message on LCD
  lcd.setCursor(0, 0);
  lcd.print("Home Automation");
  lcd.setCursor(0, 1);
  lcd.print("System Standby");
  delay(2000);
  lcd.clear();
}

void loop() {
  // 1. Check for system activation via LDR
  int ldrValue = analogRead(LDR_PIN);
  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  if (ldrValue > LDR_ACTIVATION_THRESHOLD) {
    if (!isSystemActive) {
      isSystemActive = true;
      Serial.println("System Activated");
      lcd.clear();
    }
  } else {
    if (isSystemActive) {
      isSystemActive = false;
      deactivateAllSystems();
      Serial.println("System Deactivated");
    }
  }

  // 2. Run main functions only if the system is active
  if (isSystemActive) {
    handleTemperature();
    handleSmoke();
    handleWaterDispenser();
    displayWaterLevel();
  } else {
    // Display standby message if system is off
    lcd.setCursor(0, 0);
    lcd.print("System Standby  ");
    lcd.setCursor(0, 1);
    lcd.print("Place obstacle  ");
  }

  delay(500); // Main loop delay
}

// --- FUNCTION DEFINITIONS --- //

void handleTemperature() {
  float temp = dht.readTemperature();

  // Check if read failed
  if (isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" C");

  if (temp > TEMPERATURE_THRESHOLD) {
    digitalWrite(FAN_MOTOR_RELAY_PIN, LOW); // Turn fan ON (active-LOW)
    Serial.println("Fan ON");
  } else {
    digitalWrite(FAN_MOTOR_RELAY_PIN, HIGH); // Turn fan OFF
    Serial.println("Fan OFF");
  }
}

void handleSmoke() {
  int smokeValue = analogRead(SMOKE_SENSOR_PIN);
  Serial.print("Smoke Value: ");
  Serial.println(smokeValue);

  // Trigger alarm if smoke detected and alarm isn't already active
  if (smokeValue > SMOKE_THRESHOLD && !isSmokeAlarmActive) {
    isSmokeAlarmActive = true;
    Serial.println("SMOKE DETECTED!");
  }

  // Sound the buzzer if the alarm is active
  if (isSmokeAlarmActive) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Check for reset button press to silence the alarm
  if (digitalRead(RESET_BUTTON_PIN) == LOW && isSmokeAlarmActive) {
    isSmokeAlarmActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Buzzer Reset");
    delay(200); // Debounce delay
  }
}

void handleWaterDispenser() {
  // Corrected logic: IR sensor LOW = object detected -> Pump ON
  if (digitalRead(IR_SENSOR_PIN) == LOW) {
    digitalWrite(PUMP_MOTOR_RELAY_PIN, LOW); // Pump ON (active-LOW relay)
    Serial.println("Object detected -> Pump ON");
  } else {
    digitalWrite(PUMP_MOTOR_RELAY_PIN, HIGH); // Pump OFF
    Serial.println("No object -> Pump OFF");
  }
}

void displayWaterLevel() {
  // Trigger the ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the echo pulse
  long duration = pulseIn(ECHO_PIN, HIGH, 20000UL); // 20ms timeout
  
  // Calculate distance in cm
  float distance = duration * 0.034 / 2;

  // Apply your formula: (7 - distance)
  float adjustedValue = 7.0 - distance;

  // Print to Serial Monitor
  Serial.print("Raw Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Adjusted: ");
  Serial.print(adjustedValue);
  Serial.println(" cm");

  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("Adjusted Value  "); // Clear line
  lcd.setCursor(0, 1);
  lcd.print("Dist: " + String(adjustedValue, 1) + " cm   ");
}



void deactivateAllSystems() {
  digitalWrite(FAN_MOTOR_RELAY_PIN, HIGH); // Fan OFF
  digitalWrite(PUMP_MOTOR_RELAY_PIN, HIGH); // Pump OFF
  digitalWrite(BUZZER_PIN, LOW); // Buzzer OFF
  isSmokeAlarmActive = false;
  lcd.clear();
}

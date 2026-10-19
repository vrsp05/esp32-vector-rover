#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <index.h>

// Initiate WebSocketServer using Port #82
WebSocketsServer webSocket = WebSocketsServer(82);

// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"

// ===========================
// Hardcoded Factory Credentials
// Change this number for each car before flashing! (e.g., BYU-Car-2)
// ===========================
const char *carSSID = "general182";

// ===========================
// Wi-Fi Channel Selection
// Use ONLY channels 1, 6, or 11 to prevent cars from lagging each other.
// ===========================
const int wifiChannel = 1;

// ===========================
// Motor Driver Blueprint & Pins
// ===========================
struct MOTOR_PINS {
  int pinIN1;
  int pinIN2;
};

// Right Motor (M1 & M2) -> Pins 13 & 12 | Left Motor (M3 & M4) -> Pins 15 & 14
MOTOR_PINS rightMotor = {13, 12}; 
MOTOR_PINS leftMotor = {15, 14};

// ===========================
// Speed Governor & Vector Calibration
// ===========================
// maxSpeed sets the raw PWM duty cycle (0-255). 
// Since we are not doing proportional speed anymore, this is the fixed speed the car uses to drive and spin.
int maxSpeed = 150; 

// --- SDL FLOOR CALIBRATION VARIABLES ---
// Adjust these during Phase 4 to map the mathematical vectors to physical reality.
// If you tell the car to spin 90 degrees and it only spins 45, increase SPIN_MS_PER_DEGREE.
const int SPIN_MS_PER_DEGREE = 8;  // Milliseconds of motor run-time per 1 degree of calculated angle
// If a magnitude of 100 drives too far, decrease DRIVE_MS_PER_UNIT.
const int DRIVE_MS_PER_UNIT = 30;   // Milliseconds of motor run-time per 1 unit of calculated magnitude

// ===========================
// Non-Blocking State Machine Variables
// ===========================
// We use a state machine with millis() instead of delay() so the ESP32 can continuously
// listen to the WebSocket. If we used delay(5000) to drive, the Emergency Stop button would be ignored.
enum MoveState { IDLE, SPINNING, DRIVING };
MoveState currentState = IDLE;

unsigned long stateStartTime = 0; // Records the exact millisecond a movement started
int currentSpinDuration = 0;      // How long the current spin phase should last
int currentDriveDuration = 0;     // How long the current drive phase should last

// ===========================
// Movement Functions (Fixed Speed)
// ===========================
void stopCar() {
  ledcWrite(rightMotor.pinIN1, 0);
  ledcWrite(rightMotor.pinIN2, 0);
  ledcWrite(leftMotor.pinIN1, 0);
  ledcWrite(leftMotor.pinIN2, 0);
  Serial.println("Car: STOP");
}

void driveForward() {
  ledcWrite(rightMotor.pinIN1, maxSpeed);
  ledcWrite(rightMotor.pinIN2, 0);
  ledcWrite(leftMotor.pinIN1, maxSpeed);
  ledcWrite(leftMotor.pinIN2, 0);
  Serial.println("Car: DRIVING FORWARD");
}

void turnRight() {
  ledcWrite(rightMotor.pinIN1, 0);
  ledcWrite(rightMotor.pinIN2, maxSpeed);
  ledcWrite(leftMotor.pinIN1, maxSpeed);
  ledcWrite(leftMotor.pinIN2, 0);
  Serial.println("Car: SPINNING RIGHT");
}

void turnLeft() {
  ledcWrite(rightMotor.pinIN1, maxSpeed);
  ledcWrite(rightMotor.pinIN2, 0);
  ledcWrite(leftMotor.pinIN1, 0);
  ledcWrite(leftMotor.pinIN2, maxSpeed);
  Serial.println("Car: SPINNING LEFT");
}

void startCameraServer();
void setupLedFlash();

// ===========================
// WebSocket Client Lock
// ===========================
int activeClient = -1; // -1 means the car is currently available

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  
  if (type == WStype_DISCONNECTED) {
    if (activeClient == num) {
      Serial.println("Driver disconnected. Car is now available.");
      activeClient = -1; 
      currentState = IDLE;
      stopCar(); // SAFETY FEATURE: Hit the brakes if Wi-Fi drops
    }
  } 
  else if (type == WStype_CONNECTED) {
    if (activeClient == -1) {
      activeClient = num;
      Serial.printf("Phone [%u] Connected! Car locked.\n", num);
    } else {
      webSocket.sendTXT(num, "ERROR: Car is currently in use by another student.");
      webSocket.disconnect(num);
    }
  } 
  else if (type == WStype_TEXT) {
    if (num != activeClient) return; 

    String command = String((char*)payload);
    int sepIndex = command.indexOf(':');
    
    // ==========================================
    // PARSE INCOMING DATA
    // ==========================================
    if (sepIndex > 0) { 
      String cmdName = command.substring(0, sepIndex);
      
      // 1. Camera Setting Sliders
      if (cmdName == "led" || cmdName == "bright" || cmdName == "contrast") {
        int cmdValue = command.substring(sepIndex + 1).toInt();
        if (cmdName == "led") {
          ledcWrite(4, map(cmdValue, 0, 255, 0, 40)); 
        } else if (cmdName == "bright") {
          esp_camera_sensor_get()->set_brightness(esp_camera_sensor_get(), cmdValue);
        } else if (cmdName == "contrast") {
          esp_camera_sensor_get()->set_contrast(esp_camera_sensor_get(), cmdValue);
        }
      }
      
      // 2. Vector Math Payload (Format: "vec:X,Y")
      else if (cmdName == "vec") {
        int commaIndex = command.indexOf(',', sepIndex + 1);
        int x = command.substring(sepIndex + 1, commaIndex).toInt();
        int y = command.substring(commaIndex + 1).toInt();

        // Handle Zero Vector (Origin)
        if (x == 0 && y == 0) {
          currentState = IDLE;
          stopCar();
          return;
        }

        // --- PHASE 1: MATH 215 UNIT CIRCLE MATH ---
        float magnitude = sqrt(x*x + y*y);
        
        // Because the car physically starts facing the +X axis (0 degrees),
        // we use standard calculus to find the exact target angle.
        float target_angle = atan2(y, x) * 180.0 / PI; 

        // --- PHASE 2: CONVERT MATH TO TIMERS ---
        currentSpinDuration = abs((int)target_angle) * SPIN_MS_PER_DEGREE;
        currentDriveDuration = magnitude * DRIVE_MS_PER_UNIT;

        // --- PHASE 3: TRIGGER THE STATE MACHINE ---
        Serial.printf("Vector Received. Mag: %.1f, Angle: %.1f deg\n", magnitude, target_angle);
        
        if (currentSpinDuration > 0) {
          currentState = SPINNING;
          // From the +X axis (0 deg):
          // Positive angles are counter-clockwise (Left turn)
          // Negative angles are clockwise (Right turn)
          if (target_angle > 0) turnLeft();
          else turnRight();
        } 
        else {
          currentState = DRIVING;
          driveForward();
        }
        
        stateStartTime = millis();
      }
    }
    
    // ==========================================
    // EMERGENCY STOP BUTTON
    // ==========================================
    else if (command == "stop") {
      Serial.println("EMERGENCY STOP TRIGGERED!");
      currentState = IDLE; // Break out of any active Spin or Drive state
      stopCar();
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // --- CAMERA CONFIGURATION ---
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  // Stability tune for older hardware
  config.xclk_freq_hz = 10000000; 
  config.frame_size = FRAMESIZE_CIF; 
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12; 
  config.fb_count = 2;

  if (!psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // --- MOTOR CONFIGURATION ---
  // Initialize motor pins as PWM speed outputs (1000 Hz, 8-bit resolution)
  ledcAttach(rightMotor.pinIN1, 1000, 8);
  ledcAttach(rightMotor.pinIN2, 1000, 8);
  ledcAttach(leftMotor.pinIN1, 1000, 8);
  ledcAttach(leftMotor.pinIN2, 1000, 8);
  
  stopCar(); // Ensure the car starts in a parked state

  sensor_t *s = esp_camera_sensor_get();
  s->set_exposure_ctrl(s, 1); 
  s->set_aec2(s, 1);          
  s->set_whitebal(s, 1);      
  s->set_awb_gain(s, 1);      
  s->set_wb_mode(s, 0);       

#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  // --- NETWORK CONFIGURATION ---
  WiFi.softAP(carSSID, NULL, wifiChannel);
  
  // Boot sequence: Flash the onboard LED 3 times
  for(int i = 0; i < 3; i++) {
    ledcWrite(4, 25); 
    delay(150);
    ledcWrite(4, 0);   
    delay(150);
  }

  startCameraServer();

  ledcAttach(4, 1000, 8); 
  ledcWrite(4, 0);        
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Camera & Motors Ready!");
}

void loop() {
  // Constantly listen to the WebSocket for Emergency Stops or new vectors
  webSocket.loop(); 
  
  // ==========================================
  // NON-BLOCKING MOVEMENT STATE MACHINE
  // ==========================================
  // This block acts like a supervisor checking a stopwatch. It looks at the current time (millis),
  // compares it to when the movement started, and transitions to the next phase if time is up.
  
  if (currentState == SPINNING) {
    // If the elapsed time is greater than the required spin duration...
    if (millis() - stateStartTime >= currentSpinDuration) {
      stopCar(); // ...stop the spin.
      delay(50); // Tiny physical pause to let hardware momentum settle before driving
      
      // Transition to Phase 2: Driving
      if (currentDriveDuration > 0) {
        currentState = DRIVING;
        driveForward();
        stateStartTime = millis(); // Reset the stopwatch for the drive phase
      } else {
        currentState = IDLE; // Vector was pure X, no Y distance
      }
    }
  } 
  else if (currentState == DRIVING) {
    // If the elapsed time is greater than the required drive duration...
    if (millis() - stateStartTime >= currentDriveDuration) {
      stopCar(); // ...arrived at vector destination!
      currentState = IDLE;
    }
  }

  delay(1); // Tiny pause to keep the ESP32 background tasks stable
}
# Vector Rover Command Center (MATH 215 SDL Project)

## About This Project

This project transforms a standard ESP32-CAM RC car into a vector-driven embedded system for a **MATH 215 (Multivariable Calculus) Self-Directed Learning (SDL) project** at Brigham Young University-Idaho. Originally prototyped as a STEM Camp workshop, the code has been entirely customized to demonstrate the real-world application of 2D Cartesian coordinate math on physical hardware.

Instead of using a traditional D-Pad or joystick, the rover is controlled via a custom mobile web dashboard that accepts strict coordinate vectors ($X, Y$). The onboard C++ firmware applies standard calculus—using the Pythagorean theorem for magnitude and `atan2(y, x)` for angle calculation—to translate abstract mathematical vectors into a physical "Spin-then-Drive" execution. 

To ensure safe operation, the firmware relies on a non-blocking state machine. This allows the ESP32-CAM to execute complex physical vector maneuvers while simultaneously broadcasting a live, low-latency video feed and listening for network "Emergency Stop" commands over a standalone Wi-Fi WebSocket connection.
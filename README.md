# Embedded Firmware Essentials - ESP32-S3

This repository contains all classwork, exercises, and projects for **Embedded Firmware Essentials**, a course offered at **UC Santa Cruz, Silicon Valley Extension**. The course focuses on the fundamentals of embedded firmware programming using the **ESP32-S3** microcontroller and the **C programming language**.

---

## Table of Contents

* [Course Overview](#course-overview)
* [Repository Structure](#repository-structure)
* [Getting Started](#getting-started)
* [Hardware Requirements](#hardware-requirements)
* [Software Requirements](#software-requirements)
* [Example Projects](#example-projects)
* [Contributing](#contributing)
* [License](#license)
* [Acknowledgements](#acknowledgements)

---

## Course Overview

Embedded Firmware Essentials introduces students to programming microcontrollers with a focus on practical applications using the **ESP32-S3**. Students learn:

* Basic C programming for embedded systems
* Using FreeRTOS for multitasking
* Controlling peripherals such as LEDs, buttons, and displays
* Debugging and deploying firmware to the ESP32-S3
* Best practices for embedded systems development

---

## Repository Structure

```
.
├── docs/               # Optional: Class notes or reference materials
├── examples/           # Example programs and exercises
├── projects/           # Completed projects
├── src/                # Source code for exercises and projects
├── include/            # Header files
├── README.md           # This file
└── LICENSE             # License information
```

---

## Getting Started

### Clone the Repository

```bash
git clone https://github.com/<your-username>/<repository-name>.git
cd <repository-name>
```

### Build and Flash Code

1. Install [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/) development environment.
2. Set up the environment:

```bash
. $IDF_PATH/export.sh
```

3. Build and flash an example:

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Hardware Requirements

* **ESP32-S3 Development Board**
* USB cable for programming
* Optional: LEDs, push buttons, 7-segment displays, and breadboard for experiments

---

## Software Requirements

* **ESP-IDF** (latest stable version)
* **C Compiler** (provided with ESP-IDF)
* Terminal or IDE (VSCode recommended with ESP-IDF extension)

---

## Example Projects

* LED blink
* Button-controlled 7-segment display
* FreeRTOS-based multitasking demos
* Sensor reading and data logging

*Note: Each example includes its own README with detailed instructions.*

---

## Contributing

Contributions are welcome! Feel free to:

* Add new example projects
* Improve existing code and documentation
* Report issues or suggest enhancements

Please follow standard GitHub practices with branches and pull requests.

---

## License

This repository is licensed under the [MIT License](LICENSE).

---

## Acknowledgements

* UC Santa Cruz, Silicon Valley Extension
* ESP32-S3 hardware and ESP-IDF documentation
* Open-source community examples and tutorials


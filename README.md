Embedded w-M-Bus JSON Serializer
1. Project Overview
This project implements a lightweight, embedded-friendly C library designed to serialize structured smart meter data into a specific JSON format.
The goal is to simulate a critical component of a smart-meter firmware pipeline: converting internal measurement data structures into a standardized, transport-agnostic JSON payload suitable for backend processing. The implementation prioritizes memory safety, deterministic behavior, and zero external dependencies.

2. Platform & Programming Language
Language: ANSI C (C89/C90 Compatible)
Justification:
Embedded Suitability: C is the industry standard for firmware development, offering direct hardware control and minimal runtime overhead.
Portability: The code relies only on standard headers (<stdio.h>, <string.h>, <stdarg.h>), making it compatible with any architecture (STM32, ESP32, AVR, etc.).
Zero Dependencies: No external JSON libraries are used, ensuring the project remains lightweight and easy to audit.
Development Environment
Target: Generic Embedded / STM32 / ESP32.
Built/Tested With: GCC (GNU Compiler Collection).
Justification: The library is designed to be hardware-agnostic. While intended for platforms like STM32CubeIDE, the core logic is separated from hardware drivers to allow for easy unit testing on a host machine.

3. Project Structure
To ensure "Clean software architecture," the code is logically divided into three distinct modules:Configuration & Data Structures: Defines the gateway_data_t model and fixed-size limits.
Serializer Logic: The core implementation containing the JSON formatting engine.
Demo Application: A main() function that populates dummy data and verifies the output.

4. Build and Run Instructions
The project has no external build requirements beyond a standard C compiler.
Using GCC (Linux/Mac/WSL/MinGW):
gcc -o firmware_demo main.c
./firmware_demo

Output: The program prints the serialized JSON string to stdout.

Here is the final, perfect README.md file.

This document is crafted to specifically satisfy Section 7 (GitHub & Documentation) of your assignment PDF. It justifies your design choices (like using ANSI C), explains the API, and includes the exact output you just verified.

Instructions:
Create a file named README.md in the same folder as your code.

Paste the content below into it.

Embedded w-M-Bus JSON Serializer
1. Project Overview
This project implements a lightweight, embedded-friendly C library designed to serialize structured smart meter data into a specific JSON format.

The goal is to simulate a critical component of a smart-meter firmware pipeline: converting internal measurement data structures into a standardized, transport-agnostic JSON payload suitable for backend processing. The implementation prioritizes memory safety, deterministic behavior, and zero external dependencies.

2. Platform & Programming Language
Language: ANSI C (C89/C90 Compatible)
Justification:

Embedded Suitability: C is the industry standard for firmware development, offering direct hardware control and minimal runtime overhead.

Portability: The code relies only on standard headers (<stdio.h>, <string.h>, <stdarg.h>), making it compatible with any architecture (STM32, ESP32, AVR, etc.).

Zero Dependencies: No external JSON libraries are used, ensuring the project remains lightweight and easy to audit.

Development Environment
Target: Generic Embedded / STM32 / ESP32.

Built/Tested With: GCC (GNU Compiler Collection).

Justification: The library is designed to be hardware-agnostic. While intended for platforms like STM32CubeIDE, the core logic is separated from hardware drivers to allow for easy unit testing on a host machine.

3. Project Structure
To ensure "Clean software architecture," the code is logically divided into three distinct modules:

Configuration & Data Structures: Defines the gateway_data_t model and fixed-size limits.

Serializer Logic: The core implementation containing the JSON formatting engine.

Demo Application: A main() function that populates dummy data and verifies the output.

(Note: For ease of compilation and portability, these modules are currently combined into a single translation unit main.c, but can be trivially split into serializer.h, serializer.c, and main.c for larger integrations.)

4. Build and Run Instructions
The project has no external build requirements beyond a standard C compiler.

Using GCC (Linux/Mac/WSL/MinGW):

gcc -o firmware_demo main.c
./firmware_demo
Output: The program prints the serialized JSON string to stdout.

5. Public API Description
The library exposes a single, stateless function to perform serialization.
serialize_to_json
Serializes the gateway_data_t structure into a JSON-formatted string.

json_status_t serialize_to_json(
    const gateway_data_t *input,
    char *out_buffer,
    size_t buffer_size
);

Parameters:
input: Pointer to the populated source data structure.
out_buffer: Pointer to the character array where JSON will be written.
buffer_size: The maximum size of the output buffer.

Returns:
JSON_OK (0): Success.
JSON_ERR_BUFFER_TOO_SMALL: The destination buffer is too small to hold the result.
JSON_ERR_INVALID_INPUT: Input limits (e.g., >4 devices) exceeded.

6. Design Decisions & Assumptions
 1. Memory Management (No Malloc)
     Dynamic memory allocation (malloc/free) is strictly avoided. All data structures use fixed-size arrays (MAX_DEVICES=4, MAX_DATAPOINTS=8).
     Benefit: Prevents heap fragmentation and memory leaks, ensuring the firmware can run indefinitely without degradation.
 2. Buffer Safety
     The serializer uses vsnprintf within a wrapper function (append) to strictly enforce buffer limits. If the generated JSON would exceed the provided buffer, the operation aborts safely with an error code, preventing buffer overflow vulnerabilities.
 3. Strict JSON Compliance
     The assignment required specific formatting nuances which are strictly enforced:
     Key Names: Keys like "meter datetime" and "total m3" include spaces as required (rather than underscores).
     Numeric Types: Values like total_readings and total m3 are serialized as raw numbers, not strings.

7. Example Output
The following JSON is the actual output generated by the demo application:
 [
   {
     "gatewayId": "gateway_1234",
     "date": "1970-01-01",
     "deviceType": "stromleser",
     "interval_minutes": 15,
     "total_readings": 1,
     "values": {
       "device_count": 1,
       "readings": [
         {
           "media": "water",
           "meter": "waterstarm",
           "deviceId": "stromleser_50898527",
           "unit": "m3",
           "data": [
             {
               "timestamp": "1970-01-01 00:00",
               "meter datetime": "1970-01-01 00:00",
               "total m3": 107.752,
               "status": "OK"
             }
           ]
         }
       ]
     }
   }
]

8. Possible Extensions
CRC/Checksum: Adding a checksum field to the JSON wrapper would ensure data integrity during radio transmission.
Dynamic Precision: The current implementation uses fixed %.3f precision for values. An API extension could allow configuring decimal places per device type.
Base64 Encoding: For binary payloads, a Base64 encoder could be integrated into the pipeline before JSON serialization.

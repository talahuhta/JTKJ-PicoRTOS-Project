# Pico-Morse-Code-Project

# Pico JTKHAT Morse Code Project

The goal of this project is to develop a Morse code messaging system utilizing a Raspberry Pi Pico W and the JTKJ Hat, powered by the FreeRTOS real-time operating system. The device functions as a communication tool capable of interpreting physical movements into Morse code symbols (dots, dashes, and spaces). These symbols are locally translated and sent to an external workstation via USB serial. The system also utilizes onboard actuators, such as an LCD screen, an RGB LED, and a buzzer, to provide comprehensive visual and auditory feedback regarding the system's current state, detected inputs, and communication status. 

---

## To-Do List

### 1. Implement Workstation to Pico Receiving (Tier 2 Requirement)
* **Goal:** Enable the Pico to receive and interpret incoming Morse code from the workstation.
* **Task:** Modify the currently looping `reading_task` (which handles the `READING` state) to actively read incoming USB serial data.
* **Task:** Parse the incoming data string.
* **Task:** Display the received symbols sequentially on the LCD screen and reflect them on the RGB LED.
* **Task:** Map Button 2 to act as a scroll mechanism to manually step through the received symbols.

### 2. Fix the Morse Code Formatting to Match Course Guidelines
* **Goal:** Ensure all messages sent to the workstation adhere strictly to the course's required Morse protocol.
* **Task:** Update the output logic in the `LISTENING` state (`imu_task`). Currently, the system prints `| |` for spaces. 
* **Task:** Change the output so that:
  * Characters are separated by a single space (` `).
  * Words are separated by two spaces (`  `).
  * The final message ends with exactly two spaces followed by a newline (`\n`).

### 4. Course Administration & Final Deliverables
* **Goal:** Ensure all grading criteria are met prior to the final meeting.
* **Task:** Update the main `main.c` file header with comments detailing the names of all group members and explicitly stating what each person contributed to the project (e.g., "Aman did task1, Taneli did task2").
* **Task:** Ensure every group member writes at least one personal diary entry per week. 
* **Task:** Schedule and prepare for the final evaluation meeting (all members must be present and capable of answering questions about the implementation).
- Email about project plan
- Write this week's diary (and check if there was meant to be one last week)

# General Project Plan
## Input (Pico)
* **Motion Detection:** Assign specific physical movements (detected via the ICM42670 accelerometer) to Morse symbols.
  * Hold down Button 2 and move the device in a designated direction.
  * Register movement on X-axis as a dot (`.`), Y-axis as a dash (`-`), and Z-axis as a space (`| |`).
  * Once a threshold is crossed, register the symbol, display it, and immediately break the reading loop to prevent multiple inputs.
  * The loop also breaks if Button 2 is released without sufficient movement.
* **State Switching:** Use Button 1 to switch between program states (`SENDING`, `READING`, `LISTENING`).
  * **Validation:** Before allowing a switch from `SENDING` to `READING`, verify that the message ends with a space (part of the double-space + newline protocol).
  * **Feedback:** Play a success tune on the buzzer and blink the RGB LED (Turquoise) when a valid message is finalized and sent. Blink Red and show an error on the LCD if invalid.
* **Reading Mode Controls:** When in the `READING` state, Button 1 will clear the display and switch back to `SENDING` mode. (Future: Button 2 will scroll through received symbols).

## Input (Workstation)
* **User Entry:** Prompt the user to input letters via the serial monitor.
* **Translation:** Run a Python function to translate the English input into Morse code strings.
* **Transmission:** Send the translated Morse code over USB serial to the Pico upon hitting 'enter'.
* **Error Handling:** Validate input (limit max length, check for illegal characters). Display exceptions to the user in the terminal before attempting to send.

## Output (Pico)
* **LCD & LED Display:**
  * **LED:** Blink Blue for dot, Green for dash, Purple for space, and Red for errors (buffer overflow or invalid character).
  * **LCD:** Display current state ("send mode", "read mode", "awaiting") and print symbols (`.`, `-`, `| |`) as they are registered.
* **On-board Translation:** Once a space is inputted on the Z-axis, pass the prior symbols to a built-in C dictionary to translate them into an English character and display it on the LCD.

## Output (Workstation)
* **Serial Printing:** Print the received characters from the Pico directly to the terminal.
* **Translation & Formatting:** Wait for the termination sequence (two spaces + newline) from the Pico. Once received, display the final translated English message in a user-friendly format on the serial monitor.
<br />
<br />

# Code Structure
### Main Initialization
* Initialize standard I/O and JTKJ Hat SDK peripherals (Buttons, RGB LED, I2C, Display, Buzzer, IMU).
* Create three FreeRTOS tasks: `Sending` (`sending_task`), `Reading` (`reading_task`), and `IMU` (`imu_task`).
* Define and enable GPIO interrupts for Button 1 and Button 2, triggered on the rising edge via a single `gpio_callback` function.
* Start the FreeRTOS task scheduler.

### Sending from Pico to Workstation (Pico Tasks)
**1. Sending Task (`SENDING` State)**
* Print instructions to the Serial Monitor that the Pico is ready to send.
* Wait in `SENDING` state for button interrupts to flag state changes:
  * **Button 2 Pressed (Rising Edge)**: 
    * Check for symbol buffer overflow (if > 7, flash Red LED and clear buffers).
    * Switch program state to `LISTENING`.
  * **Button 1 Pressed (Rising Edge)**:
    * Verify message ends with a space (protocol requirement).
      * *If invalid*: Blink Red LED, display "needs | |" on LCD, and remain in `SENDING` state.
    * *If valid*: 
      * Print the final translated message string to Serial.
      * Play a success tune on the buzzer and blink Turquoise LED.
      * Display "SENT" on the LCD.
      * Switch program state to `READING`.

**2. IMU Task / Data Processing (`LISTENING` State)**
* Wait in `LISTENING` state for physical input (can be aborted back to `SENDING` by pressing any button).
* **Calibration Phase**: Read initial gyroscope data over 10 iterations to calculate offsets and prevent drift.
* **Detection Loop (100Hz)**:
  * Read raw accelerometer and gyroscope data.
  * Apply a complementary filter (alpha = 0.98f) to combine accelerometer and gyroscope data to accurately calculate pitch and roll.
  * Subtract the calculated gravity vector from the accelerometer data to isolate pure linear acceleration.
  * Monitor data until it detects an acceleration threshold in a certain direction:
    * **X-axis (> 0.6g)**: Registers a dot (`.`), blinks Blue LED, updates LCD, and breaks loop.
    * **Y-axis (> 0.5g)**: Registers a dash (`-`), blinks Green LED, updates LCD, and breaks loop.
    * **Z-axis (> 0.5g)**: Registers a space (`| |`), blinks Purple LED, updates LCD, and breaks loop.
  * **Translation (On Z-axis Space)**: 
    * Pass the collected symbol buffer to the internal `translate_morse` C dictionary function.
    * Add the translated English character to the final `messageBuffer`.
    * If translation fails (invalid char sequence), blink Red LED, display `?`, and ignore.

### Receiving from Workstation to Pico
**1. Input Stage (Workstation - Python Client)**
* Pico does nothing (waits in `READING` state).
* Serial asks for input from user.
* Once input entered:
  * Check validity (Exceptions give an error and ask for new input).
  * Translate string into Morse code.
  * Send over USB serial to Pico.

**2. Reading Stage (Pico)**
* *Currently: Loops in `READING` state waiting for Button 1 to be pressed to clear the display and switch back to `SENDING`*.
* **To Be Implemented (Missing Features):**
  * Start displaying when button 2 is pressed (first character is space).
  * Display incoming Morse code one symbol at a time on the LED and LCD.
  * Button 2 will scroll to the next symbol.
  * Space denominates new char, double space denominates a new word.
  * Once it reaches the end, buzz a tune (pressing button 2 will restart message).
  * Send some kind of indication to serial that it was read.


## File structure
|-- src/

     |-- main.
     
     |-- other_file.c
     
|-- include/

     |-- other_file.h
     
|-- assets/

      |-- file1.csv
      
      |-- file2.png
      
|--CMakeList.txt

## Course deliverables
- Personal weekly diary
- Video
- In person meeting/presentation
- Returning device

## AI Usage Declaration
All project planning, code structure, state machine, C code and FreeRTOS task implementations in this project were written entirely by the group memebers other than what is specified in main.c. 

Generative AI (Google Gemini) was used to assist with project documentation. Specifically, it was used to format and expand the `## Code Structure` section of this README to clearly reflect the written C code. 
* **Prompts used:** "can you just update this code structure part of the readme to reflect the changes seen in main... try to keep it mostly the same and same level of depth", followed by "can you format this into git".
* **Modifications made:** The AI's markdown output was copied into the README to organize the technical documentation of the existing code architecture.
https://share.gemini.google/MX6c5LDNp8k1

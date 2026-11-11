# Pico-Morse-Code-Project

RTOS pico morse code project

## EVERYTHING MUST BE DONE 3 DAYS BEFORE NOVEMBER 22nd
To/do:
- Email about project plan
- begin writing code
- Write this week's diary (and check if there was meant to be one last week)
- Install VM and get pico running (Taneli) (this weekend)

Coding Tasks:
- Clean up main.c
- Write very basic program in the sending task to print imu data to serial when button 2 is pressed (start function to retrieve imu data)
  - button 2 is pressed -> return IMU data
- Next step: upgrade the sending task to print the symbol the IMU handler returns
   - button 2 is pressed down (rising edge 0 -> 1) -> begins measuring IMU data
   - movement detected -> function return symbol and breaks loop
   - button 2 being let get go (falling edge 1 -> 0) -> breaks loop (doesn't return anything)
   - sending task code will print the returned value to serial

## General Project Plan
### Input (Pico)
- Assign certain movements to symbols
- holding down button 2 and moving in a certain direction (accelerometer)
  - upon detecting a symbol motion it will print that to serial and break the loop
  - can be exited by releasing button 2 as well
- button 1 to switch modes (only if message has been ended with a double space)
  - we must somehow communicate that to the user
  - play a little tune to indicate that message was sent successfully
- in read mode use button 2 for scrolling (beep tune when done)
- clear cache upon switching states (for memory reasons)
### Input (Workstation)
- Input letters
- Function to translate into morse code
- Type into serial monitor and 'enter' to send
- message to indicate if it was successful and some kind of indication on the pico as well
- limit input max length
- handle all exceptions before sending to pico and display an error to the user
### Output (Pico)
- LCD & LED display morse code
- LED long blink=dash, short blink=dot, no blink=space
- LCD (.,-,||  =  dot,dash,space)
- scroll through with button 2
- end word with beep noise
- loop back if you continue scrolling
### Output (Workstation)
- Received code printed to serial
- invalid characters give some kind of error in serial
- final message is translated to letters
- Displayed on serial monitor in user friendly way

### Other project tasks
## Guidelines
## Code Structure
### Main
- Init everything
- create tasks
- start scheduler
- handle task exceptions
- define interrupts
### Receiving from Workstation to Pico
#### Input Stage (Workstation)
- Pico does nothing
- Serial asks for input from user
- Once input entered:
  - Check validity (Exceptions give an error and ask for new input)
  - Translate
  - Switch to read (buzz tune to indicate successful sending and change stage)
#### Reading Stage (Pico)
- Start displaying when button 2 is pressed (first character is space)
- Display morse code one symbol at a time
  - On the LED and LCD
- Button 2 will scroll to the next symbol
- Space denominates new char
- double space denominates a new word
- Once it reaches the end, it buzzes a tune (pressing button 2 will restart message)
- Switch program state with Button 1
- Send some kind of indication to serial that it was read
### Sending from Pico to Workstation
- Print to the Serial Monitor that the Pico is now ready to send
- Pico
  - Button 2 pressed (rising edge)
    - Call function to process IMU data
      - Begins a loop that monitors IMU data until it detects an acceleration in a certain direction
      - Return a symbol (ends loop)
      - Loop is also ended if button 2 is released (falling edge)
    - single space between chars
    - double space between words
    - send every symbol directly to serial
    - Collect chars in buffer
      - when receive 'space' translate previous symbol inputs into a char
      - error chars are not added to the 
      - translate each char and store in an array
      - Translate function returns error if not real char
      - Empty symbol array = space
    - When Button 1 is pressed
      - message must end with two spaces and a new line
        - error led if it didn't
      - print final translated message and move to a new line
      - play a tune that it was successful
      - Switch state
  #### Send Message Format (Only if newline print):
  "Receiving from Pico"
  - "A"
  - "B"
  - " "
  - "C"
  - "Message "AB C""
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

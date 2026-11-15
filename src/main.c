
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

/* 
 Pico Morse Code Project 
 Creators: Taneli Alahuhta, Sam Zahkarov, Aman Deep 
 
 Member Contributions:
 - Taneli Alahuhta: Project planning, implemented main initializations, button interrupts, FreeRTOS tasks (sending, imu), created helper function (translate_morse, blink led, song, clear_buffers) 
  configured IMU data handling.
 - Sam Zahkarov: [Describe their contribution, e.g., project planning, design, etc.]
 - Aman Deep: [Describe their contribution, e.g., project planning, design, etc.]
 
 AI Usage Declaration:
 Generative AI was used to assist with specific parts of this project:
 
 1. IMU Data Math: AI was used to help create code to handle the mathematics of calculating linear movement from the IMU.
    - https://share.google/aimode/zkAy0dUN0WANw8fvL
    - Modifications made: Adopted the provided mathematical logic to calculate roll, pitch, and gravity vectors inside the `imu_task`.
    - Used provided math to better understand how it works
 2. Buzzer Song: AI was used to generate the frequencies and note array for the success tune.
 3. README organization (more info in REAMDE)
 
 All other C code, logic, and task implementations were written manually.
 */

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048

// Constants
#define MESSAGE_BUFFER_SIZE 255
#define DEG_TO_RAD 0.0174532925f

//Program states
enum state { SENDING = 1, READING, LISTENING };
enum state programState = SENDING;

// Button states
bool button1Pressed = false;
bool button2Pressed = false;

// Buffers and indeces for Morse code symbols and messages
char symbolBuffer[10];
int symbolBufferIndex = 0;
char messageBuffer[MESSAGE_BUFFER_SIZE];
int messageIndex = 0;

// Function declarations
static char translate_morse(const char *morse);
static void clear_buffers(void);
static void song(void);
static void blink_RGBled(uint8_t r, uint8_t g, uint8_t b, uint32_t duration_ms);

// Song (generated with AI)
int16_t song1[] = {
    392, 392, 440, 392, 330, 330, 294, 330,  /* G4 G4 A4 G4 E4 E4 D4 E4 */
    392, 392, 440, 523, 440, 392, 0,         /* G4 G4 A4 C5 A4 G4 - pause */
    523, 523, 587, 523, 440, 440, 392, 440,  /* C5 C5 D5 C5 A4 A4 G4 A4 */
    523, 523, 440, 392, 330, 294, -1         /* C5 C5 A4 G4 E4 D4 - end */
};

// Pico reading task
static void reading_task(void *arg){
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10000));

    for(;;){
        tight_loop_contents(); // Modify with application code here.
        // Handle reading functions while in reading state
        if (programState == READING){
            if (button1Pressed) {
                printf("Button 1 pressed, switching to SENDING state\n");
                programState = SENDING; // Switch to SENDING state
                button1Pressed = false; // Reset the button state
                clear_display();
                write_text("send mode");
            }

            vTaskDelay(pdMS_TO_TICKS(400));
        }
    }
}

// Pico sending task
static void sending_task(void *arg){
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10000));

    printf("Pico is in Sending mode. To begin transmiting morse code press button 2 and move the pico as such:\n left or right = dot\n back or forth = dash\n up or down = space\n");
    printf("Every space will translate the prior inputs into an english character on the workstation. Inputting 2 spaces back to back will simply add a space to your message.\n");
    printf("To transmit and translate your final message to the workstation add one final ending space and press button 1\n");

    write_text("send mode");

    for(;;){
        tight_loop_contents();
        // Handle sending logic while in sending state
        if (programState == SENDING){
            if (button2Pressed) {
                if (symbolBufferIndex > 7) {
                    printf("Symbol buffer overflow, clearing buffer\n");
                    clear_display();
                    blink_RGBled(255, 0, 0, 200); // Blink red LED to indicate buffer overflow
                    clear_buffers(); // Clear the buffers to avoid overflow
                }

                button2Pressed = false; // Reset the button state
                programState = LISTENING; // Switch to LISTENING state
            }

            if (button1Pressed && messageBuffer[messageIndex - 1] == ' ') { // Message must end with a space to be valid
                printf("Button 1 pressed, switching to READING state\n");
                printf("Final transmitted message from pico: %s\n", messageBuffer); // Print the complete message")
                programState = READING; // Switch to READING state
                button1Pressed = false; // Reset the button state
                clear_display();
                write_text("  SENT "); // Display "SENT" on the display
                song(); // Play the song to indicate the end of the message
                blink_RGBled(0, 255, 255, 200); // Blink turqouise to indicate it was sent
                clear_buffers(); // Clear the buffers for the next message
                clear_display();
                write_text("read mode");
            }
            else if (button1Pressed) {
                printf("Message is incomplete (Needs to end with a space to be valid)\n"); // Print a message indicating the incomplete message
                blink_RGBled(255, 0, 0, 50); // Blink led red to indicate an error on the pico
                clear_display();
                write_text("needs | |");
                vTaskDelay(10);
                clear_display();
                button1Pressed = false; // Reset the button state
            }
            vTaskDelay(pdMS_TO_TICKS(400));
        }
    }
}

static void imu_task(void *arg){
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10000));

    for(;;){
        tight_loop_contents();

        // Handle data functions while in data state
        if (programState == LISTENING) {
            // Escape if either button is pressed
            if (button1Pressed || button2Pressed) {
                programState = SENDING; // Switch to SENDING state
                button1Pressed = false; // Reset the button state
                button2Pressed = false; // Reset the button state
                clear_display();
                continue; // Skip the rest of the loop and start over
            }

            // Initialize the ICM42670 sensor with default values
            ICM42670_start_with_default_values();

            float ax, ay, az, gx, gy, gz, t; // Variables to hold accelerometer, gyroscope, and temperature data
            float gx_offset = 0.0f, gy_offset = 0.0f, gz_offset = 0.0f; // Gyroscope offsets
            
            // Read the first sensor data and average to calculate accurate offsets
            // printf("Calculating gyroscope offsets, keep still...\n");
            for (int i = 0; i < 10; i++) {
                ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);
                gx_offset += gx;
                gy_offset += gy;
                gz_offset += gz;
                vTaskDelay(pdMS_TO_TICKS(10)); // Delay between readings
            }
            gx_offset = gx_offset / 10;
            gy_offset = gy_offset / 10;
            gz_offset = gz_offset / 10;

            // printf("Calibration complete!\n");
            // printf("Gyroscope offsets calculated: gx_offset=%.2f, gy_offset=%.2f, gz_offset=%.2f\n", gx_offset, gy_offset, gz_offset);

            // Initialize starting tilt angles from intial sensor readings
            ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);

            // Set angles in relation to Z axis
            float roll = atan2(ay, az);
            float pitch = atan2(-ax, sqrt(ay * ay + az * az));

            // Get the current tick count for timing
            // TickType_t is just a fundemental unit of time data type to represent the system tick count
            TickType_t lastWakeTime = xTaskGetTickCount(); 
            const float dt = 0.01f; // Time step for integration (10ms)
            
            clear_display();
            write_text("awaiting"); // Communicate to user on pico that it is ready to detect an input
            
            while (1) {
                // Set loop to 100hz to match gyro sampling rate
                vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10)); // Delay between readings

                ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);

                // Remove gyroscope offsets
                gx -= gx_offset;
                gy -= gy_offset;
                gx *= DEG_TO_RAD; // Convert to radians
                gy *= DEG_TO_RAD;

                // Instantaneous roll and pitch angles from accelerometer
                float roll_acc = atan2(ay, az);
                float pitch_acc = atan2(-ax, sqrt(ay * ay + az * az));

                // Complementary filter to combine accelerometer and gyroscope data
                // Accelerometers measuring orientation are prone to any external movements but accurate and stable in the long term
                // Gyroscopes are prone to drift but much more smooth and accurate for short term rotational movements
                // Integrating pitch and roll with gyro and mixing with the orientaton obtained from the measure of gravity
                const float alpha = 0.98f; // Filter coefficient
                roll = alpha * (roll + gx * dt) + (1.0f - alpha) * roll_acc;
                pitch = alpha * (pitch + gy * dt) + (1.0f - alpha) * pitch_acc;

                // Calculate gravity vector components based on roll and pitch angles
                float gravity_x = -sinf(pitch);
                float gravity_y = sinf(roll) * cosf(pitch);
                float gravity_z = cosf(roll) * cosf(pitch);

                // Isolate linear motion
                float linear_acc_x = ax - gravity_x;
                float linear_acc_y = ay - gravity_y;
                float linear_acc_z = az - gravity_z;

                // printf("Linear Acceleration - X: %.2f, Y: %.2f, Z: %.2f\n", linear_acc_x, linear_acc_y, linear_acc_z);

                // Create morse code symbols based on linear movements
                if (fabsf(linear_acc_x) > 0.6f) { // Seems to be a tiny be easier to move the pico in this direction so the threshold is different
                    printf(".");
                    blink_RGBled(0, 0, 255, 200); // Blink LED blue to indicate a dot
                    clear_display();
                    write_text("    . "); // Write text to LCD
                    symbolBuffer[symbolBufferIndex++] = '.'; // Add symbol to buffer
                    programState = SENDING; // Switch to SENDING state
                    break; // Exit the loop after detecting a movement to avoid multiple detections in one cycle
                }
                if (fabsf(linear_acc_y) > 0.5f) {
                    printf("-");
                    blink_RGBled(0, 255, 0, 200); // Blink LED green to indicate a dash
                    clear_display();
                    write_text("   - ");
                    symbolBuffer[symbolBufferIndex++] = '-';
                    programState = SENDING;
                    break;
                }
                if (fabsf(linear_acc_z) > 0.5f) {
                    printf("| |");
                    blink_RGBled(255, 0, 255, 200); // Blink LED purple to indicate a space
                    clear_display();
                    write_text("   | | ");
                    char translatedMorse = translate_morse(symbolBuffer);
                    if (translatedMorse != '?') { // Ensure validity of symbols before adding to final message
                        messageBuffer[messageIndex++] = translatedMorse; // Translate the morse symbols sent and add it to the message buffer
                    }
                    programState = SENDING;
                    break;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(400));
        }
    }
}

// GPIO interrupt callback for button presses
static void gpio_callback(uint gpio, uint32_t events) {
    // printf("GPIO Interrupt on pin %d, events: %u\n", gpio, events);
    if (gpio == BUTTON1  && (events ==  8)) {
        button1Pressed = true;
        // printf("Button 1 Interrupt\n");
    }
    if (gpio == BUTTON2 && (events ==  8)) {
        button2Pressed = true;
        // printf("Button 2 Interrupt\n");
    }
}

// Simple function to clear the buffers
static void clear_buffers(void) {
    memset(symbolBuffer, 0, sizeof(symbolBuffer));
    symbolBufferIndex = 0;
    memset(messageBuffer, 0, sizeof(messageBuffer));
    messageIndex = 0;
}

// Function to translate the morse code array into a character
static char translate_morse(const char *morse) {
    // Morse code mapping
    struct MorseMapping {
        const char *code;
        char character;
    } morseMappings[] = {
        {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},
        {".", 'E'}, {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'},
        {"..", 'I'}, {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'},
        {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, {".--.", 'P'},
        {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
        {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'},
        {"-.--", 'Y'}, {"--..", 'Z'},
        {".----", '1'}, {"..---", '2'}, {"...--", '3'},
        {"....-", '4'}, {".....", '5'}, {"-....", '6'},
        {"--...", '7'}, {"---..", '8'}, {"----.", '9'},
        {"-----", '0'}
    };

    // Search for the corresponding character
    if (strlen(morse) == 0) { // Check if the morse code is empty (indicating a space)
        printf("Received space\n");

        return ' '; // Return ' ' if the morse code is just a space
    }

    for (size_t i = 0; i < sizeof(morseMappings) / sizeof(morseMappings[0]); ++i) {
        if (strcmp(morse, morseMappings[i].code) == 0) {
            printf("Translated Morse code '%s' to character '%c'\n", morse, morseMappings[i].character);

            // Clear the symbol buffer after translating
            memset(symbolBuffer, 0, sizeof(symbolBuffer));
            symbolBufferIndex = 0;
            char displayChar[4]; // Buffer to hold the translated character formatted for display
            sprintf(displayChar, "   %c ", morseMappings[i].character); // Use the sprinf function to format the display string

            clear_display();
            write_text(displayChar);

            return morseMappings[i].character;;
        }
    }

    printf("No translation found for Morse code '%s'. Try again\n", morse);

    // Clear the symbol buffer after attempting translation
    memset(symbolBuffer, 0, sizeof(symbolBuffer));
    symbolBufferIndex = 0;

    rgb_led_write(255, 0, 0); // Turn LED red to indicate a failed translation
    vTaskDelay(pdMS_TO_TICKS(100)); // Keep the LED on for 100ms
    rgb_led_write(0, 0, 0); // Turn off the LED

    clear_display();
    write_text("   ? ");

    return '?'; // Return '?' if no match is found
}

// Simple function to loop through and play the notes of song array
// Perhaps could implement more parameters to allow for playing different songs with different settings
static void song() {
    for (int i = 0; song1[i] != -1; i++) { // loop through the song array until the end marker (-1) is reached
        if (song1[i] == 0) { // Check for a pause in the song
            sleep_ms(220); // Pause for 220ms to create a gap between notes
        } else {
            buzzer_play_tone(song1[i], 185); // Play the note for 185ms
        }
    }
}

// Simple helper function to blink the led with specified parameters
static void blink_RGBled(uint8_t r, uint8_t g, uint8_t b, uint32_t duration_ms) {
    rgb_led_write(r, g, b); // Turn on the LED with specified color
    vTaskDelay(pdMS_TO_TICKS(duration_ms)); // Keep the LED on for the specified duration
    rgb_led_write(0, 0, 0); // Turn off the LED
}

int main() {
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    init_button1();
    init_button2();

    init_rgb_led();

    init_i2c_default();
    init_display();
    clear_display();

    init_buzzer();

    init_ICM42670();

    rgb_led_write(0, 0, 0); // Turn down LED because it is really bright and hurts my eyes

    TaskHandle_t SendingTask = NULL;
    // Create the tasks with xTaskCreate
    BaseType_t sendingTaskResult = xTaskCreate(sending_task,       // (en) Task function
                "Sending",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &SendingTask);    // (en) A handle to control the execution of this task
    
    if(sendingTaskResult != pdPASS) {
        printf("Sending task creation failed\n");
        return 0;
    }
    
    TaskHandle_t ReadingTask = NULL;
    // Create the tasks with xTaskCreate
    BaseType_t readingTaskResult = xTaskCreate(reading_task,       // (en) Task function
                "Reading",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &ReadingTask);    // (en) A handle to control the execution of this task
    
    if(readingTaskResult != pdPASS) {
        printf("Reading Task creation failed\n");
        return 0;
    }

    TaskHandle_t IMUTask = NULL;
    // Create the tasks with xTaskCreate
    BaseType_t imuTaskResult = xTaskCreate(imu_task,       // (en) Task function
                "IMU",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &IMUTask);    // (en) A handle to control the execution of this task
    
    if(imuTaskResult != pdPASS) {
        printf("IMU Task creation failed\n");
        return 0;
    }

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, gpio_callback);

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}

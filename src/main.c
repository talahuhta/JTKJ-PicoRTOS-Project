
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

/*
Pico Morse Code Project
Creators: Taneli Alahuhta, Sam Zahkarov, Aman Deep
*/

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048

#define MESSAGE_BUFFER_SIZE 255

//Program states
enum state { SENDING = 1, READING, LISTENING };
enum state programState = SENDING;

// Button states
bool button1Pressed = false;
bool button2Pressed = false;

char symbolBuffer[10]; // Buffer to hold the Morse code symbols
char messageBuffer[MESSAGE_BUFFER_SIZE]; // Buffer to hold the complete Morse code message in characters

// Pico reading task
static void reading_task(void *arg){
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10000));

    printf("Reading task is running\n");

    for(;;){
        tight_loop_contents(); // Modify with application code here.
        // Handle reading functions while in reading state
        if (programState == READING){
            if (button1Pressed) {
                printf("Button 1 pressed, switching to SENDING state\n");
                programState = SENDING; // Switch to SENDING state
                button1Pressed = false; // Reset the button state
            }

            vTaskDelay(pdMS_TO_TICKS(400));
        }
    }
}

// Pico sending task
static void sending_task(void *arg){
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10000));

    for(;;){
        tight_loop_contents(); // Modify with application code here.
        // Handle sending functions while in sending state
        if (programState == SENDING){
            // Add your sending code here
            printf("Sending task is running\n");

            if (button2Pressed) {
                printf("Detecting IMU\n");
                programState = LISTENING; // Switch to LISTENING state
                button2Pressed = false; // Reset the button state
            }

            if (button1Pressed) {
                printf("Button 1 pressed, switching to READING state\n");
                programState = READING; // Switch to READING state
                button1Pressed = false; // Reset the button state
            }

            vTaskDelay(pdMS_TO_TICKS(400));
        }
    }
}

static void data_task(void *arg){
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10000));

    for(;;){
        tight_loop_contents();

        printf("Data task is running\n");

        // Handle data functions while in data state
        if (programState == LISTENING){
            // Initialize the ICM42670 sensor with default values
            ICM42670_start_with_default_values();

            // Read accelerometer, gyroscope, and temperature data
            float ax, ay, az, gx, gy, gz, t;

            // Read sensor data in a loop until significant movement is detected or button 2 is pressed
            for(;;) {
                ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t);

                printf("Accelerometer: ax=%.2f, ay=%.2f, az=%.2f\n", ax, ay, az);

                if (ax > 1.0) {
                    printf(".");
                    button2Pressed = false; // Reset button 2 state
                    symbolBuffer[0] = '.'; // Set the symbol corrosponding to the detected movement
                    programState = SENDING; // Switch to SENDING state
                    break; // Exit the loop
                }
                if (ay > 1.0) {
                    printf("-");
                    button2Pressed = false; // Reset button 2 state
                    symbolBuffer[0] = '-'; // Set the symbol corrosponding to the detected movement
                    programState = SENDING; // Switch to SENDING state
                    break; // Exit the loop
                }
                if (az > 1.0) {
                    printf("| |");
                    button2Pressed = false; // Reset button 2 state
                    symbolBuffer[0] = ' '; // Set the symbol corrosponding to the detected movement
                    programState = SENDING; // Switch to SENDING state
                    break; // Exit the loop
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

int main() {
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    init_button1();
    init_button2();

    init_led();

    init_i2c_default();
    init_display();
    clear_display();

    init_buzzer();

    init_ICM42670();

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

    TaskHandle_t DataTask = NULL;
    // Create the tasks with xTaskCreate
    BaseType_t dataTaskResult = xTaskCreate(data_task,       // (en) Task function
                "Data",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &DataTask);    // (en) A handle to control the execution of this task
    
    if(dataTaskResult != pdPASS) {
        printf("Data Task creation failed\n");
        return 0;
    }

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, gpio_callback);

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}
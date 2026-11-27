#include <stdio.h>
#include "pico/stdlib.h"
#include "tkjhat/sdk.h"

int main(void) {
    stdio_init_all();
    init_hat_sdk();

    // Wait briefly for USB serial to connect
    sleep_ms(2000);

    if (init_ICM42670() == 0) {
        // Note: Depending on your TKJHAT SDK version, this may be named
        // ICM42670_start_with_default_values() or start_sensor_with_default_values()
        ICM42670_start_with_default_values();
    } else {
        printf("IMU init failed\n");
        while (true) sleep_ms(1000);
    }

    // CSV header
    printf("time_ms,ax,ay,az,gx,gy,gz\n");

    float ax, ay, az, gx, gy, gz, temp;

    while (true) {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &temp) == 0) {
            uint32_t t = to_ms_since_boot(get_absolute_time());
            printf("%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                   t, ax, ay, az, gx, gy, gz);
        }
        sleep_ms(20); // ~50 Hz sampling
    }
}
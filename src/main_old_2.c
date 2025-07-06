// #include <stdio.h>
// #include <zephyr/kernel.h>
// #include <zephyr/drivers/gpio.h>

// // Sleep settings
// static const int32_t blink_sleep_ms = 300;
// static const int32_t print_sleep_ms = 1000;

// // Stack size settings
// #define BLINK_THREAD_STACK_SIZE 256

// // Define stack areas for the threads
// K_THREAD_STACK_DEFINE(blink_stack, BLINK_THREAD_STACK_SIZE);

// // Declare thread data structs
// static struct k_thread blink_thread;

// // Get LED struct from DeviceTree
// const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);

// // Blink thread entry point
// void blink_thread_start(void *arg1, void *arg2, void *arg3)
// {
//     int ret;
//     int state = 0;

//     while (1)
//     {
//         state = !state;
//         ret = gpio_pin_set_dt(&led, state);
//         if (ret < 0)
//         {
//             printk("Error: could not toggle pin\r\n");
//         }
//         k_msleep(blink_sleep_ms);
//     }
// }

// void main(void)
// {
//     int ret;
//     k_tid_t blink_tid;

//     // Initialize the LED GPIO pin
//     if (!gpio_is_ready_dt(&led))
//     {
//         printk("Error: GPIO pin %d is not ready\r\n", led.pin);
//         return;
//     }

//     // Set the GPIO pin as output
//     ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
//     if (ret < 0)
//     {
//         printk("Error: could not configure pin %d\r\n", led.pin);
//         return;
//     }

//     // Start the blink thread
//     blink_tid = k_thread_create(&blink_thread,                      // Thread struct
//                                 blink_stack,                        // Stack area
//                                 K_THREAD_STACK_SIZEOF(blink_stack), // Stack size
//                                 blink_thread_start,                 // Entry point
//                                 NULL,                               // arg1
//                                 NULL,                               // arg2
//                                 NULL,                               // arg3
//                                 7,                                  // Priority
//                                 0,                                  // Options
//                                 K_NO_WAIT);                         // Delay

//     while (1)
//     {
//         printk("Blink thread started with ID: %p\r\n", blink_tid);
//         k_msleep(print_sleep_ms);
//     }
// }

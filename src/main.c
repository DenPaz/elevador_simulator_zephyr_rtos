#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define STACK_SIZE 1024
#define ADC_THREAD_PRIORITY 5
#define LED_THREAD_PRIORITY 5
#define OLED_THREAD_PRIORITY 5

// Mutex for OLED display access
K_MUTEX_DEFINE(oled_mutex);

// ADC message structure
struct adc_msg
{
    uint16_t port1;
    uint16_t port2;
};
K_MSGQ_DEFINE(adc_to_led, sizeof(struct adc_msg), 10, 4);
K_MSGQ_DEFINE(adc_to_oled, sizeof(struct adc_msg), 10, 4);

// GPIO configuration for LED
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);

// OLED display configuration
const struct device *display = DEVICE_DT_GET(DT_ALIAS(ssd1306));

// Thread: Simulates ADC readings
void adc_thread(void *p1, void *p2, void *p3)
{
    struct adc_msg msg = {0, 0};

    while (1)
    {
        msg.port1 = (msg.port1 + 100) % 4096;
        msg.port2 = (msg.port2 + 200) % 4096;

        k_msgq_put(&adc_to_led, &msg, K_NO_WAIT);
        k_msgq_put(&adc_to_oled, &msg, K_NO_WAIT);
        LOG_INF("ADC - p1:%d p2:%d", msg.port1, msg.port2);

        k_sleep(K_MSEC(1000));
    }
}

// Thread: Controls LED based on ADC readings
void led_thread(void *p1, void *p2, void *p3)
{
    struct adc_msg msg;

    while (1)
    {
        k_msgq_get(&adc_to_led, &msg, K_FOREVER);
        int led_state = (msg.port2 > 2048) ? 1 : 0;
        gpio_pin_set_dt(&led, led_state);
        LOG_INF("LED Thread - LED %s", led_state ? "ON" : "OFF");
    }
}

// Thread: Updates OLED display with mutex
void oled_thread(void *p1, void *p2, void *p3)
{
    struct adc_msg msg;
    char line1[16], line2[16];

    while (1)
    {
        k_msgq_get(&adc_to_oled, &msg, K_FOREVER);

        snprintf(line1, sizeof(line1), "P1: %4d", msg.port1);
        snprintf(line2, sizeof(line2), "P2: %4d", msg.port2);

        if (k_mutex_lock(&oled_mutex, K_MSEC(500)) == 0)
        {
            cfb_framebuffer_clear(display, true);
            cfb_print(display, line1, 0, 0);
            cfb_print(display, line2, 0, 16);
            cfb_framebuffer_finalize(display);
            k_mutex_unlock(&oled_mutex);
            LOG_INF("OLED updated: %s | %s", line1, line2);
        }

        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_STACK_DEFINE(adc_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(led_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(oled_stack, STACK_SIZE);

struct k_thread adc_thread_data;
struct k_thread led_thread_data;
struct k_thread oled_thread_data;

void main(void)
{
    int ret;

    // Init OLED
    if (!device_is_ready(display))
    {
        LOG_ERR("Display not ready");
        return;
    }

    ret = cfb_framebuffer_init(display);
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize display");
        return;
    }

    // Init LED
    if (!gpio_is_ready_dt(&led))
    {
        LOG_ERR("LED not ready");
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure LED GPIO");
        return;
    }

    // Start threads
    k_thread_create(&adc_thread_data,
                    adc_stack,
                    STACK_SIZE,
                    adc_thread,
                    NULL,
                    NULL,
                    NULL,
                    ADC_THREAD_PRIORITY,
                    0,
                    K_NO_WAIT);
    k_thread_create(&led_thread_data,
                    led_stack,
                    STACK_SIZE,
                    led_thread,
                    NULL,
                    NULL,
                    NULL,
                    LED_THREAD_PRIORITY,
                    0,
                    K_NO_WAIT);
    k_thread_create(&oled_thread_data,
                    oled_stack,
                    STACK_SIZE,
                    oled_thread,
                    NULL,
                    NULL,
                    NULL,
                    OLED_THREAD_PRIORITY,
                    0,
                    K_NO_WAIT);

    LOG_INF("Application started");
    while (1)
    {
        k_sleep(K_MSEC(1000));
    }
}

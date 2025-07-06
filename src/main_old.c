// #include <zephyr/device.h>
// #include <zephyr/kernel.h>
// #include <zephyr/display/cfb.h>
// #include <zephyr/logging/log.h>

// LOG_MODULE_REGISTER(display);

// static const struct device *display = DEVICE_DT_GET(DT_NODELABEL(ssd1306));

// void main(void)
// {
//     if (display == NULL)
//     {
//         LOG_ERR("Display pointer is NULL");
//         return;
//     }

//     if (!device_is_ready(display))
//     {
//         LOG_ERR("Display device is not ready");
//         return;
//     }

//     int ret;
//     ret = cfb_framebuffer_init(display);
//     if (ret != 0)
//     {
//         LOG_ERR("Failed to initialize display");
//         return;
//     }

//     ret = cfb_print(display, "Hello, World!", 0, 0);
//     if (ret != 0)
//     {
//         LOG_ERR("Failed to print to display");
//         return;
//     }

//     ret = cfb_framebuffer_finalize(display);
//     if (ret != 0)
//     {
//         LOG_ERR("Failed to finalize display");
//         return;
//     }
// }

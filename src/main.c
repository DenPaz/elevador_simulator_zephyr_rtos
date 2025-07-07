#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

/*==============================================================================
 *                            CONFIGURAÇÕES
 *============================================================================*/
#define STACK_SIZE 1024
#define MOVE_TIME_MS 2000
#define MAX_PENDING_REQUESTS 5

// Prioridades das threads
#define BUTTON_PRIORITY 3
#define ELEVATOR_PRIORITY 4
#define DISPLAY_PRIORITY 5

/*==============================================================================
 *                            GPIO PERIFÉRICOS
 *============================================================================*/
// Definição dos botões
const struct gpio_dt_spec btn1 = GPIO_DT_SPEC_GET(DT_ALIAS(btn1), gpios);
const struct gpio_dt_spec btn2 = GPIO_DT_SPEC_GET(DT_ALIAS(btn2), gpios);
const struct gpio_dt_spec btn3 = GPIO_DT_SPEC_GET(DT_ALIAS(btn3), gpios);
const struct gpio_dt_spec btn4 = GPIO_DT_SPEC_GET(DT_ALIAS(btn4), gpios);

// Definição do display
const struct device *display = DEVICE_DT_GET(DT_ALIAS(display));

/*==============================================================================
 *                            SINCRONIZAÇÃO & FILAS
 *============================================================================*/
// Mutexes
K_MUTEX_DEFINE(display_mutex);
K_MUTEX_DEFINE(queue_mutex);

// Fila de mensagens entre botões e elevador
K_MSGQ_DEFINE(elevator_queue, sizeof(uint8_t), 10, 4);

// Fila de requisições pendentes
static uint8_t pending_requests[MAX_PENDING_REQUESTS];
static uint8_t pending_count;

/*==============================================================================
 *                            ESTADOS GLOBAIS
 *============================================================================*/
typedef enum
{
    ELEV_READY = 'R',
    ELEV_MOVING = 'M'
} elev_state_t;
static volatile uint8_t current_floor = 1;
static volatile uint8_t last_requested_floor = 0;
static volatile elev_state_t elevator_state = ELEV_READY;

/*==============================================================================
 *                            PROTÓTIPOS LOCAIS
 *============================================================================*/
static void button_thread(void *arg1, void *arg2, void *arg3);
static void elevator_thread(void *arg1, void *arg2, void *arg3);
static void display_thread(void *arg1, void *arg2, void *arg3);

/*==============================================================================
 *                            THREADS
 *============================================================================*/
// Lê debounced os quatro botões e enfileira chamadas
static void button_thread(void *arg1, void *arg2, void *arg3)
{
    const struct gpio_dt_spec *buttons[] = {&btn1, &btn2, &btn3, &btn4};
    bool last_state[4] = {0};
    uint8_t floor;

    // Estado inicial
    for (int i = 0; i < 4; i++)
    {
        last_state[i] = gpio_pin_get_dt(buttons[i]) == 0;
    }

    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            bool current_state = (gpio_pin_get_dt(buttons[i]) == 0);

            // Borda de descida: botão pressionado
            if (current_state && !last_state[i])
            {
                floor = i + 1;
                LOG_INF("Botão do andar %d pressionado", floor);
                k_msgq_put(&elevator_queue, &floor, K_NO_WAIT);

                // Adiciona à fila de requisições pendentes
                k_mutex_lock(&queue_mutex, K_FOREVER);
                if (pending_count < MAX_PENDING_REQUESTS)
                {
                    pending_requests[pending_count++] = floor;
                }
                k_mutex_unlock(&queue_mutex);
            }
            last_state[i] = current_state;
        }
        k_sleep(K_MSEC(50));
    }
}

// Processa a fila de elevador
static void elevator_thread(void *arg1, void *arg2, void *arg3)
{
    uint8_t target_floor;

    while (1)
    {
        while (1)
        {
            // Bloqueia até receber uma requisição
            k_msgq_get(&elevator_queue, &target_floor, K_FOREVER);

            // Remove do pending_requests
            k_mutex_lock(&queue_mutex, K_FOREVER);
            for (int i = 0; i < pending_count; i++)
            {
                pending_requests[i] = pending_requests[i + 1];
            }
            if (pending_count)
            {
                pending_count--;
            }
            k_mutex_unlock(&queue_mutex);

            // Atualiza o estado
            last_requested_floor = target_floor;
            elevator_state = ELEV_MOVING;
            LOG_INF("Elevador indo para o andar %d", target_floor);

            // Move o elevador piso a piso
            while (current_floor != target_floor)
            {
                k_sleep(K_MSEC(MOVE_TIME_MS));
                if (current_floor < target_floor)
                {
                    current_floor++;
                }
                else
                {
                    current_floor--;
                }
                LOG_INF("Andar atual: %d", current_floor);
            }

            // Elevador chegou ao destino
            LOG_INF("Elevador chegou ao andar %d", current_floor);
            elevator_state = ELEV_READY;
            last_requested_floor = 0;
        }
    }
}

// Exibe o estado do elevador no display
static void display_thread(void *arg1, void *arg2, void *arg3)
{
    char line1[32], line2[32], line3[32], line4[32];

    while (1)
    {
        snprintf(line1, sizeof(line1), "Andar: %d", current_floor);
        if (last_requested_floor > 0)
        {
            snprintf(line2, sizeof(line2), "Chamado: %d", last_requested_floor);
        }
        else
        {
            snprintf(line2, sizeof(line2), "Chamado: ");
        }
        snprintf(line3, sizeof(line3), "Estado: %c", (char)elevator_state);

        // Exibe a fila de requisições pendentes
        k_mutex_lock(&queue_mutex, K_MSEC(100));
        if (pending_count)
        {
            int pos = 0;
            for (int i = 0; i < pending_count; i++)
            {
                pos += snprintf(line4 + pos, sizeof(line4) - pos, "%d%s", pending_requests[i],
                                (i < pending_count - 1) ? "," : "");
            }
        }
        else
        {
            snprintf(line4, sizeof(line4), "Fila: Vazia");
        }
        k_mutex_unlock(&queue_mutex);

        if (k_mutex_lock(&display_mutex, K_MSEC(500)) == 0)
        {
            cfb_print(display, "                      ", 0, 0);
            cfb_print(display, "                      ", 0, 16);
            cfb_print(display, "                      ", 0, 32);
            cfb_print(display, "                      ", 0, 48);
            cfb_print(display, line1, 0, 0);
            cfb_print(display, line2, 0, 16);
            cfb_print(display, line3, 0, 32);
            cfb_print(display, line4, 0, 48);
            cfb_framebuffer_finalize(display);
            k_mutex_unlock(&display_mutex);
        }
        k_sleep(K_MSEC(500));
    }
}

// Definição das stacks e threads
K_THREAD_STACK_DEFINE(button_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(elevator_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(display_stack, STACK_SIZE);
struct k_thread button_thread_data;
struct k_thread elevator_thread_data;
struct k_thread display_thread_data;

/*==============================================================================
 *                              FUNÇÃO MAIN
 *============================================================================*/
void main(void)
{
    int ret;

    // Inicializa o display
    if (!device_is_ready(display))
    {
        LOG_ERR("Display não está pronto");
        return;
    }
    ret = cfb_framebuffer_init(display);
    if (ret != 0)
    {
        LOG_ERR("Falha ao inicializar o display");
        return;
    }

    // Inicializa os botões
    const struct gpio_dt_spec *buttons[] = {&btn1, &btn2, &btn3, &btn4};
    for (int i = 0; i < 4; i++)
    {
        if (!gpio_is_ready_dt(buttons[i]))
        {
            LOG_ERR("Botão %d não está pronto", i + 1);
            return;
        }
        ret = gpio_pin_configure_dt(buttons[i], GPIO_INPUT | GPIO_PULL_UP);
        if (ret != 0)
        {
            LOG_ERR("Falha ao configurar o GPIO do botão %d", i + 1);
            return;
        }
    }

    // Cria as threads
    k_thread_create(&button_thread_data, button_stack, STACK_SIZE,
                    button_thread, NULL, NULL, NULL,
                    BUTTON_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&elevator_thread_data, elevator_stack, STACK_SIZE,
                    elevator_thread, NULL, NULL, NULL,
                    ELEVATOR_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&display_thread_data, display_stack, STACK_SIZE,
                    display_thread, NULL, NULL, NULL,
                    DISPLAY_PRIORITY, 0, K_NO_WAIT);

    LOG_INF("Sistema iniciado");
}

/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/

#define _GNU_SOURCE

#include "board/gpio_driver.h"
#include "board/board_spec.h"
#include "board/ex10_osal.h"
#include "board/time_helpers.h"
#include "ex10_api/ex10_macros.h"
#include "ex10_api/ex10_print.h"
#include "ex10_api/trace.h"

#include <errno.h>
#include <fcntl.h>
#include <gpiod.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum R807_PIN_NUMBERS
{
    BOARD_POWER_PIN = 5,
    READY_N_PIN     = 7,
    EX10_ENABLE_PIN = 12,
    RESET_N_PIN     = 13,
    TEST            = 24,
    IRQ_N_PIN       = 25,
};

/**
 * The following RPI GPIO pins are not connected to any R807 functions
 * and can be used for debug purposes.
 */
static uint8_t const r807_debug_pins[] = {2, 3, 4};

/**
 * List of Ex10 GPIO test pins.
 * DIO 6 and 8 cannot be tested for the following reasons:
 * DIO 6 (GPIO  4) - Pin voltage reading is always low due to the
 *                   voltage divider connected to DIO 6 inside the
 *                   Transceiver.
 * DIO 8 (GPIO 27) - Pin is already allocated to RPi SPI 1 pin.
 */
static uint8_t const ex10_gpio_test_pins[][2] = {
    {0, 22},  // DIO  0: GPIO 22
    {1, 23},  // DIO  1: GPIO 23
    {2, 18},  // DIO  2: GPIO 18
    {3, 19},  // DIO  3: GPIO 19
    // {6, 4},    // DIO  6: GPIO  4
    // {8, 27},   // DIO  8: GPIO 27
    {13, 17},  // DIO 13: GPIO 17
    {14, 21},  // DIO 14: GPIO 21
};

/**
 * The R807 board LEDs are connected to the RPi GPIO pins:
 * - LED_0: RPi GPIO 16
 * - LED_1: RPi GPIO 26
 * - LED_2: RPi GPIO 20
 * - LED_3: RPi GPIO  6
 * The ordering of r807_led_pins[] should match the hardware.
 */
static uint8_t const r807_led_pins[] = {16, 26, 20, 6};

/* libgpiod handles */
static const char*        consumer         = "Ex10 SDK";
static const char*        gpiochip_label   = "pinctrl-bcm2835";
static const char*        gpiochip_label2  = "pinctrl-bcm2711";
static struct gpiod_chip* chip             = NULL;
static struct gpiod_line* power_line       = NULL;
static struct gpiod_line* reset_line       = NULL;
static struct gpiod_line* ex10_enable_line = NULL;
static struct gpiod_line* ready_n_line     = NULL;
static struct gpiod_line* ex10_test_line   = NULL;
static struct gpiod_line* irq_n_line       = NULL;

static struct gpiod_line* debug_lines[ARRAY_SIZE(r807_debug_pins)] = {NULL};
static struct gpiod_line*
                          ex10_gpio_test_lines[ARRAY_SIZE(ex10_gpio_test_pins)] = {NULL};
static struct gpiod_line* led_lines[ARRAY_SIZE(r807_led_pins)] = {NULL};

static pthread_t irq_n_monitor_pthread;

static void (*irq_n_cb)(void) = NULL;

// When set to false, inhibit the IRQ_N monitor thread from calling the
// registered callback function pointer irq_n_cb.
// This value is set to true when the register_irq_callback() function
// successfully starts the IRQ_N monitor thread.
// This value is set to false when deregister_irq_callback() is called.
// The interface functions irq_monitor_callback_enable() and
// irq_monitor_callback_is_enabled() provides external access to this value.
static bool irq_monitor_callback_enable_flag = false;

/*
 * This lock guards the Host Interface (i.e. SPI interface) from being
 * accessed by both the client thread and the IRQ_N monitor thread at the
 * same time.
 *
 * A lock is used in the Impinj Reference Design because on the RPi the
 * interrupt handler for IRQ_N is dispatched by a normal POSIX thread.
 * This thread is comments and documentation is called the IRQ_N monitor
 * thread.
 *
 * A lock can be acquired because it is done in a thread and NOT in an
 * actual interrupt context.
 *
 * On a port to a microcontroller, a lock should NOT be used because
 * acquiring a lock in an interrupt handler can cause a deadlock. In this
 * case, the IRQ_N interrupt should be temporarily disabled.
 */
static pthread_mutex_t irq_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Guards the callback function pointer irq_n_cb, during registration,
 * deregistration and callback dispatch/execution.
 *
 * On a port to a microcontroller, use of this flag would be similar to masking
 * the IRQ_N interrupt vector.
 */
static pthread_mutex_t irq_n_callback_lock = PTHREAD_MUTEX_INITIALIZER;

static void irq_n_pthread_cleanup(void* arg)
{
    struct gpiod_line* irq_n_line_arg = arg;
    if (irq_n_line_arg != irq_n_line)
    {
        ex10_eprintf("irq_n_line_arg != irq_n_line: %p, %p\n",
                     (void const*)irq_n_line_arg,
                     (void const*)irq_n_line);
    }

    if (irq_n_line)  // Valid line detected
    {
        gpiod_line_release(irq_n_line);
        irq_n_line = NULL;
    }
    else
    {
        ex10_eprintf("irq_line: NULL\n");
    }

    tracepoint(pi_ex10sdk, GPIO_mutex_unlock, ex10_get_thread_id());

    // Try to take the irq_lock. If the try fails then the lock is already
    // acquired. Regardless of try's success or failure, release the lock.
    // Note: pthread cancellation may leave the mutex in a locked
    // state and it is not an error. This happens when the call sequence:
    //   Ex10GpioDriver.irq_enable(false);
    //   Ex10SpiDriver.read() or write();
    //   Ex10GpioDriver.irq_enable(true);
    // is cancelled during the read() or write() operation; the call to
    // irq_enable(true) will not be made, and this cancellation cleanup handler
    // will be invoked.
    pthread_mutex_trylock(&irq_lock);
    int const unlock_result = pthread_mutex_unlock(&irq_lock);
    if (unlock_result != 0)
    {
        ex10_eprintf("pthread_mutex_unlock(irq_lock) failed: %d %s\n",
                     unlock_result,
                     strerror(unlock_result));
    }

    pthread_mutex_trylock(&irq_n_callback_lock);
    int const unlock_result_2 = pthread_mutex_unlock(&irq_n_callback_lock);
    if (unlock_result_2 != 0)
    {
        ex10_eprintf("pthread_mutex_unlock(cb_lock) failed: %d %s\n",
                     unlock_result_2,
                     strerror(unlock_result_2));
    }
}

/**
 * pthread for monitoring IRQ_N for interrupts.
 *
 * @param thread_arg Unused at present. This argument could be used
 *                   to indicate whether errors occurred within the thread
 *                   execution lifetime.
 *
 * @return void*     The return pointer will be the same as the pointer
 *                   passed in.
 */
static void* irq_n_monitor(void* thread_arg)
{
    // Cancellation is deferred until the thread next calls a function
    // that is a cancellation point
    int old_type = -1;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);

    if (irq_n_line != NULL)
    {
        ex10_eprintf("tid: %u, irq_n_line already allocated\n",
                     ex10_get_thread_id());
        return thread_arg;
    }

    // Reserve IRQ_N pin
    if (chip == NULL)
    {
        ex10_eprintf("chip == NULL\n");
        return thread_arg;
    }

    irq_n_line = gpiod_chip_get_line(chip, IRQ_N_PIN);
    if (irq_n_line == NULL)
    {
        ex10_eprintf(
            "tid: %u, "
            "gpiod_chip_get_line(IRQ_N_PIN) failed: %d %s\n",
            ex10_get_thread_id(),
            errno,
            strerror(errno));
        return thread_arg;
    }

    // Note: pthread_cleanup_push() is a macro, and completes its statement
    // within the macro itself. Therefore the trailing semicolon ';' is omitted
    // here to silences the static analysis "empty expression" warning.
    // clang-format off
    pthread_cleanup_push(irq_n_pthread_cleanup, (void*)irq_n_line)  // ;

    // Add edge monitoring
    int const result_code =
        gpiod_line_request_falling_edge_events(irq_n_line, consumer);
    // clang-format on
    if (result_code != 0)
    {
        ex10_eprintf(
            "tid: %u, "
            "gpiod_line_request_falling_edge_events() failed: %d %s\n",
            ex10_get_thread_id(),
            errno,
            strerror(errno));
        return thread_arg;
    }

    // Note: After this point, irq_n_line is allocated and must be cleaned up
    // via the call to irq_n_pthread_cleanup() when the while() loop exits.
    while (true)
    {
        // Block waiting, with no timeout, for a falling edge.
        // This function calls ppoll(), which is a pthread cancellation point,
        // on an array (gpiod calls it a 'bulk') of file descriptors.
        int const event_status = gpiod_line_event_wait(irq_n_line, NULL);
        tracepoint(pi_ex10sdk, GPIO_irq_n_low);

        // Clear the falling edge event.
        // The underlying system call is read(), which is a cancellation point.
        struct gpiod_line_event event;
        gpiod_line_event_read(irq_n_line, &event);
        if (event.event_type != GPIOD_LINE_EVENT_FALLING_EDGE)
        {
            ex10_eprintf("unexpected: event_type: %d, irq_n_monitor() exit\n",
                         event.event_type);
            break;
        }

        if (event_status == 1)
        {
            pthread_mutex_lock(&irq_n_callback_lock);
            if (irq_n_cb && irq_monitor_callback_enable_flag)
            {
                (*irq_n_cb)();
            }
            pthread_mutex_unlock(&irq_n_callback_lock);
        }
        else
        {
            ex10_eprintf("IRQ_N monitoring failed with %d\n", event_status);
            break;
        }
    }
    pthread_cleanup_pop(irq_n_line);

    return thread_arg;
}

static void irq_enable(bool enable)
{
    if (enable)
    {
        // Unlock to allow IRQ_N handler to run
        tracepoint(pi_ex10sdk, GPIO_mutex_unlock, ex10_get_thread_id());
        pthread_mutex_unlock(&irq_lock);
    }
    else
    {
        // Lock to prevent IRQ_N handler from running
        tracepoint(pi_ex10sdk, GPIO_mutex_lock_request, ex10_get_thread_id());
        pthread_mutex_lock(&irq_lock);
        tracepoint(pi_ex10sdk, GPIO_mutex_lock_acquired, ex10_get_thread_id());
    }
}

enum PudnConfig
{
    PudnNone = 0,
    PudnUp   = 1,
    PudnDown = 2,
};

static void bcm2835_configure_pudn(uint32_t volatile* gpio_base,
                                   uint8_t            pin,
                                   enum PudnConfig    config)
{
    // See the BCM2835 Arm Peripherals Manual
    // Write the desired config
    const uint32_t gppud_offset = 0x25;
    *(gpio_base + gppud_offset) = config;

    // Wait 150 cycles
    get_ex10_time_helpers()->busy_wait_ms(10);

    // Write GPPUDCLK0 to indicate which GPIOs to apply the configuration to.
    // This is a typical bitmask where bit n is set to apply the configuration
    // for GPIOn.
    uint32_t       value            = (1 << pin);
    const uint32_t gppudclk0_offset = 0x26;
    *(gpio_base + gppudclk0_offset) = value;

    // Wait 150 cycles
    get_ex10_time_helpers()->busy_wait_ms(10);

    // Remove GPIO selection
    *(gpio_base + gppudclk0_offset) = 0;
}

static void bcm2711_configure_pudn(uint32_t volatile* gpio_base,
                                   uint8_t            pin,
                                   enum PudnConfig    config)
{
    // See the BCM2711 Arm Peripherals Manual
    const uint32_t gpio_pup_pdn_cntrl_reg0_offset = 0x39;

    // Do a read-modify-write to the config register. Each GPIO has 2 bits of
    // configuration in this register, so there's a total of 16 GPIO configs
    // packed in here. Clear and then set the bits for the GPIO we care about.
    uint32_t gpio_pull_reg = *(gpio_base + gpio_pup_pdn_cntrl_reg0_offset);
    gpio_pull_reg &= (3u << pin);
    gpio_pull_reg |= (config << pin);
    *(gpio_base + gpio_pup_pdn_cntrl_reg0_offset) = gpio_pull_reg;
}

static int32_t configure_gpio_pudn(uint8_t pin, enum PudnConfig config)
{
    // The return value of this function. Note that there is a single return
    // point in this function. This is to properly clean up resources prior
    // to the return.
    // Errors encountered first have priority and must not be over-written
    // by subsequent errors, since later errors are often caused by the
    // initial error condition.
    int32_t result_code = 0;
    int     model_fd    = open("/proc/device-tree/model", O_RDONLY);
    if (model_fd == -1)
    {
        ex10_eprintf("open('/proc/device-tree/model') failed: %s\n",
                     strerror(errno));
        result_code = errno;
    }

    char model_str[64];
    ex10_memzero(&model_str, sizeof(model_str));

    if (result_code == 0)
    {
        ssize_t const n_read = read(model_fd, model_str, sizeof(model_str));
        if (n_read <= 0 || model_str[0] == 0)
        {
            result_code = (errno != 0) ? errno : ENODEV;
        }
    }

    int gpio_base_fd = open("/dev/gpiomem", O_RDWR | O_CLOEXEC);
    if (gpio_base_fd == -1)
    {
        ex10_eprintf("open('/dev/gpiomem') failed: %s\n", strerror(errno));
        result_code = (result_code != 0) ? result_code : errno;
    }

    size_t const mmap_length = 1024u;
    uint32_t*    gpio_base   = mmap(
        NULL, mmap_length, PROT_READ | PROT_WRITE, MAP_SHARED, gpio_base_fd, 0);
    if (gpio_base == MAP_FAILED)
    {
        ex10_eprintf("mmap('/dev/gpiomem') failed: %s\n", strerror(errno));
        result_code = (result_code != 0) ? result_code : errno;
    }
    else
    {
        if (strstr(model_str, "Pi 3") != NULL)
        {
            bcm2835_configure_pudn(gpio_base, pin, config);
        }
        else if (strstr(model_str, "Pi 4") != NULL)
        {
            bcm2711_configure_pudn(gpio_base, pin, config);
        }
        else
        {
            ex10_eprintf("Unknown device model: %s\n", model_str);
            result_code = (result_code != 0) ? result_code : ENODEV;
        }
        munmap(gpio_base, mmap_length);
    }

    if (gpio_base_fd >= 0)
    {
        close(gpio_base_fd);
    }

    if (model_fd >= 0)
    {
        close(model_fd);
    }

    return result_code;
}

static int32_t make_result_code(int result_code, int error_value)
{
    if (result_code == 0)
    {
        return 0;
    }

    return (errno != 0) ? errno : error_value;
}

static int32_t set_board_power(bool power_on)
{
    if (power_line != NULL)
    {
        int const result_code = gpiod_line_set_value(power_line, power_on);
        return make_result_code(result_code, ENODEV);
    }
    else
    {
        return ENODEV;
    }
}

static bool get_board_power(void)
{
    // Note: The assumption here is that if the pin was not allocated then
    // the pin level is false.
    // Board specific implementations will need to set this value appropriately.
    return (power_line == NULL) ? false : gpiod_line_get_value(power_line);
}

static int32_t set_ex10_enable(bool enable)
{
    if (ex10_enable_line != NULL)
    {
        int const result_code = gpiod_line_set_value(ex10_enable_line, enable);
        return make_result_code(result_code, ENODEV);
    }
    else
    {
        return ENODEV;
    }
}

static bool get_ex10_enable(void)
{
    return (ex10_enable_line == NULL) ? false
                                      : gpiod_line_get_value(ex10_enable_line);
}

static int32_t register_irq_callback(void (*cb_func)(void))
{
    int32_t result_code = 0;
    pthread_mutex_lock(&irq_n_callback_lock);
    if (irq_n_cb == NULL)
    {
        irq_n_cb = cb_func;

        result_code =
            pthread_create(&irq_n_monitor_pthread, NULL, irq_n_monitor, NULL);
        if (result_code == 0)
        {
            irq_monitor_callback_enable_flag = true;
        }
        else
        {
            irq_monitor_callback_enable_flag = false;
            ex10_eprintf("pthread_create() failed: %d, %s\n",
                         result_code,
                         strerror(result_code));
        }
    }
    else
    {
        ex10_eprintf("already registered\n");
        result_code = EBUSY;
    }

    pthread_mutex_unlock(&irq_n_callback_lock);
    return result_code;
}

static int32_t deregister_irq_callback(void)
{
    // Note: This function may be called multiple times during board teardown;
    // i.e. double deregistration. gpio_cleanup() will call this function
    // regardless of state to insure gpio driver resource release.
    // Therefore, an error will be returned from this function if the IRQ_N
    // monitor thread has already been joined. Ignore the error returned.
    pthread_mutex_lock(&irq_n_callback_lock);
    irq_monitor_callback_enable_flag = false;
    irq_n_cb                         = NULL;
    pthread_mutex_unlock(&irq_n_callback_lock);

    // Reason(s) for pthread_join() or pthread_cancel() to fail in the
    // gpio_driver.c context:
    // ESRCH (3) No thread with the ID thread could be found.
    //       i.e. the thread was not successfully created when
    //       register_irq_monitor_callback() was called.
    int const error_cancel = pthread_cancel(irq_n_monitor_pthread);
    int const error_join   = pthread_join(irq_n_monitor_pthread, NULL);

    return (error_cancel == 0) ? error_join : error_cancel;
}

static void irq_monitor_callback_enable(bool enable)
{
    pthread_mutex_lock(&irq_n_callback_lock);
    irq_monitor_callback_enable_flag = enable;
    pthread_mutex_unlock(&irq_n_callback_lock);
}

static bool irq_monitor_callback_is_enabled(void)
{
    pthread_mutex_lock(&irq_n_callback_lock);
    bool const enable = irq_monitor_callback_enable_flag;
    pthread_mutex_unlock(&irq_n_callback_lock);
    return enable;
}

static bool thread_is_irq_monitor(void)
{
    pthread_t const tid_self = pthread_self();
    return pthread_equal(tid_self, irq_n_monitor_pthread) ? true : false;
}

static void gpio_release_all_lines(void)
{
    if (power_line)
    {
        gpiod_line_release(power_line);
        power_line = NULL;
    }
    if (reset_line)
    {
        gpiod_line_release(reset_line);
        reset_line = NULL;
    }
    if (ex10_enable_line)
    {
        gpiod_line_release(ex10_enable_line);
        ex10_enable_line = NULL;
    }
    if (ready_n_line)
    {
        gpiod_line_release(ready_n_line);
        ready_n_line = NULL;
    }
    if (ex10_test_line)
    {
        gpiod_line_release(ex10_test_line);
        ex10_test_line = NULL;
    }
}

static int32_t gpio_initialize(bool board_power_on,
                               bool ex10_enable,
                               bool reset)
{
    // NOTE: Inputs default to Pull-Up enable. Pull-up/pull-down values are not
    //       modifiable from userspace until Linux v5.5.
    chip = gpiod_chip_open_by_label(gpiochip_label);
    /* Kernel 5.x-based RPi OS uses a different chip label */
    if (!chip)
    {
        chip = gpiod_chip_open_by_label(gpiochip_label2);
    }
    if (!chip)
    {
        return (errno != 0) ? errno : ENODEV;
    }

    gpio_release_all_lines();

    // The EX10 TEST line should always be driven low.
    ex10_test_line = gpiod_chip_get_line(chip, TEST);
    if (ex10_test_line == NULL)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    int result_code = gpiod_line_request_output(ex10_test_line, consumer, 0u);
    if (result_code != 0)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    int32_t pull_result = configure_gpio_pudn(BOARD_POWER_PIN, PudnNone);
    if (pull_result != 0)
    {
        return pull_result;
    }

    power_line = gpiod_chip_get_line(chip, BOARD_POWER_PIN);
    if (power_line == NULL)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    result_code =
        gpiod_line_request_output(power_line, consumer, board_power_on);
    if (result_code != 0)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    pull_result = configure_gpio_pudn(EX10_ENABLE_PIN, PudnNone);
    if (pull_result != 0)
    {
        return pull_result;
    }

    ex10_enable_line = gpiod_chip_get_line(chip, EX10_ENABLE_PIN);
    if (ex10_enable_line == NULL)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    result_code =
        gpiod_line_request_output(ex10_enable_line, consumer, ex10_enable);
    if (result_code != 0)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    pull_result = configure_gpio_pudn(RESET_N_PIN, PudnNone);
    if (pull_result != 0)
    {
        return pull_result;
    }

    reset_line = gpiod_chip_get_line(chip, RESET_N_PIN);
    if (reset_line == NULL)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    result_code = gpiod_line_request_output(reset_line, consumer, !reset);
    if (result_code != 0)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    pull_result = configure_gpio_pudn(READY_N_PIN, PudnNone);
    if (pull_result != 0)
    {
        return pull_result;
    }

    ready_n_line = gpiod_chip_get_line(chip, READY_N_PIN);
    if (ready_n_line == NULL)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    result_code = gpiod_line_request_input(ready_n_line, consumer);
    if (result_code != 0)
    {
        return (errno != 0) ? errno : ENOENT;
    }

    // Enable debug pins as outputs with their initial level at '1'.
    for (size_t idx = 0u; idx < ARRAY_SIZE(r807_debug_pins); ++idx)
    {
        pull_result = configure_gpio_pudn(r807_debug_pins[idx], PudnNone);
        if (pull_result != 0)
        {
            return pull_result;
        }

        debug_lines[idx] = gpiod_chip_get_line(chip, r807_debug_pins[idx]);
        if (debug_lines[idx] == NULL)
        {
            return (errno != 0) ? errno : ENOENT;
        }

        result_code = gpiod_line_request_output(debug_lines[idx], consumer, 1u);
        if (result_code != 0)
        {
            return (errno != 0) ? errno : ENOENT;
        }
    }

    for (size_t idx = 0u; idx < ARRAY_SIZE(ex10_gpio_test_pins); ++idx)
    {
        pull_result =
            configure_gpio_pudn(ex10_gpio_test_pins[idx][1], PudnNone);
        if (pull_result != 0)
        {
            return pull_result;
        }

        ex10_gpio_test_lines[idx] =
            gpiod_chip_get_line(chip, ex10_gpio_test_pins[idx][1]);
        if (ex10_gpio_test_lines[idx] == NULL)
        {
            return (errno != 0) ? errno : ENOENT;
        }

        result_code =
            gpiod_line_request_input(ex10_gpio_test_lines[idx], consumer);
        if (result_code != 0)
        {
            return (errno != 0) ? errno : ENOENT;
        }
    }

    // Enable LED pins as outputs with their initial level at '0' (LEDs off)
    for (size_t idx = 0u; idx < ARRAY_SIZE(r807_led_pins); ++idx)
    {
        pull_result = configure_gpio_pudn(r807_led_pins[idx], PudnNone);
        if (pull_result != 0)
        {
            return pull_result;
        }

        led_lines[idx] = gpiod_chip_get_line(chip, r807_led_pins[idx]);
        if (led_lines[idx] == NULL)
        {
            return (errno != 0) ? errno : ENOENT;
        }

        result_code = gpiod_line_request_output(led_lines[idx], consumer, 0u);
        if (result_code != 0)
        {
            return (errno != 0) ? errno : ENOENT;
        }
    }

    if (ex10_enable && !board_power_on)
    {
        ex10_eprintf("Ex10 Line Conflict: enable on without board power");
        return ENXIO;
    }

    // Note: It is not typical for either the Impinj Reader Chip power supply
    // nor the ENABLE lines to be enabled when the GPIO driver is initialized.
    // Powering up and down the Impinj Reader Chip is intended to be the role
    // of the Ex10PowerTRansactor module.
    if (board_power_on && ex10_enable)
    {
        ex10_eprintf("note: unexpected power, enable initialization\n");
        // Wait for the TCXO to settle.
        // See power_transactor.c, ENABLE_TO_RESET_RELEASE_TIME_MS.
        get_ex10_time_helpers()->busy_wait_ms(10);
        set_ex10_enable(true);
    }

    return 0;
}

static void gpio_cleanup(void)
{
    deregister_irq_callback();

    gpio_release_all_lines();

    for (size_t idx = 0u; idx < ARRAY_SIZE(debug_lines); ++idx)
    {
        if (debug_lines[idx] != NULL)
        {
            gpiod_line_release(debug_lines[idx]);
            debug_lines[idx] = NULL;
        }
    }

    for (size_t idx = 0u; idx < ARRAY_SIZE(led_lines); ++idx)
    {
        if (led_lines[idx] != NULL)
        {
            gpiod_line_release(led_lines[idx]);
            led_lines[idx] = NULL;
        }
    }

    if (chip)
    {
        gpiod_chip_close(chip);
        chip = NULL;
    }
}

static int32_t assert_ready_n(void)
{
    if (ready_n_line != NULL)
    {
        gpiod_line_release(ready_n_line);
        int const result_code =
            gpiod_line_request_output(ready_n_line, consumer, 0);
        if (result_code == 0)
        {
            return 0;
        }
    }

    return (errno != 0) ? errno : ENOENT;
}

static int32_t release_ready_n(void)
{
    if (ready_n_line != NULL)
    {
        gpiod_line_release(ready_n_line);
        int const result_code =
            gpiod_line_request_input(ready_n_line, consumer);
        if (result_code == 0)
        {
            return 0;
        }
    }

    return (errno != 0) ? errno : ENOENT;
}

static int32_t assert_reset_n(void)
{
    if (reset_line != NULL)
    {
        int const result_code = gpiod_line_set_value(reset_line, 0);
        if (result_code == 0)
        {
            return 0;
        }
    }

    return (errno != 0) ? errno : ENOENT;
}

static int32_t deassert_reset_n(void)
{
    if (reset_line != NULL)
    {
        int const result_code = gpiod_line_set_value(reset_line, 1);
        if (result_code == 0)
        {
            return 0;
        }
    }

    return (errno != 0) ? errno : ENOENT;
}

static int32_t reset_device(void)
{
    int32_t const result_code_1 = assert_reset_n();
    get_ex10_time_helpers()->busy_wait_ms(10);
    int32_t const result_code_2 = deassert_reset_n();
    return (result_code_1 != 0) ? result_code_1 : result_code_2;
}

static int ready_n_pin_get(void)
{
    int gpio_level = gpiod_line_get_value(ready_n_line);
    return gpio_level;
}

static int32_t busy_wait_ready_n(uint32_t timeout_ms)
{
    struct Ex10TimeHelpers const* time_helpers = get_ex10_time_helpers();

    uint32_t const start_time = time_helpers->time_now();

    // Check for ready n low or get a timeout
    int result_code = 0;
    int gpio_level  = 1;
    do
    {
        gpio_level = gpiod_line_get_value(ready_n_line);
        if (gpio_level == -1)
        {
            ex10_eprintf("gpiod_line_get_value() failed: %d, %s\n",
                         errno,
                         strerror(errno));
            result_code = -1;
        }

        if (time_helpers->time_elapsed(start_time) > timeout_ms)
        {
            ex10_eprintf("timeout: %u ms expired\n", timeout_ms);
            errno       = ETIMEDOUT;
            result_code = -1;
        }

    } while ((gpio_level != 0) && (result_code == 0));

    tracepoint(pi_ex10sdk, GPIO_ready_n_low);
    return (result_code == 0) ? 0 : errno;
}

static bool get_test_pin_level(uint8_t pin_no)
{
    for (size_t pin_idx = 0; pin_idx < ARRAY_SIZE(ex10_gpio_test_pins);
         ++pin_idx)
    {
        if (pin_no == ex10_gpio_test_pins[pin_idx][0])
        {
            return gpiod_line_get_value(ex10_gpio_test_lines[pin_idx]);
        }
    }
    return false;  // not a valid pin_no
}

static size_t debug_pin_get_count(void)
{
    return ARRAY_SIZE(r807_debug_pins);
}

static bool debug_pin_get(uint8_t pin_idx)
{
    if (pin_idx < ARRAY_SIZE(r807_debug_pins))
    {
        return gpiod_line_get_value(debug_lines[pin_idx]);
    }
    return false;
}

static void debug_pin_set(uint8_t pin_idx, bool value)
{
    if (pin_idx < ARRAY_SIZE(r807_debug_pins))
    {
        gpiod_line_set_value(debug_lines[pin_idx], value);
    }
}

static void debug_pin_toggle(uint8_t pin_idx)
{
    if (pin_idx < ARRAY_SIZE(r807_debug_pins))
    {
        int const value = gpiod_line_get_value(debug_lines[pin_idx]);
        gpiod_line_set_value(debug_lines[pin_idx], value ^ 1);
    }
}

static size_t led_pin_get_count(void)
{
    return ARRAY_SIZE(r807_led_pins);
}

static bool led_pin_get(uint8_t pin_idx)
{
    if (pin_idx < ARRAY_SIZE(r807_led_pins))
    {
        return gpiod_line_get_value(led_lines[pin_idx]);
    }

    return false;
}

static void led_pin_set(uint8_t pin_idx, bool value)
{
    if (pin_idx < ARRAY_SIZE(r807_led_pins))
    {
        gpiod_line_set_value(led_lines[pin_idx], value);
    }
}

static void led_pin_toggle(uint8_t pin_idx)
{
    if (pin_idx < ARRAY_SIZE(r807_led_pins))
    {
        int const value = gpiod_line_get_value(led_lines[pin_idx]);
        gpiod_line_set_value(led_lines[pin_idx], value ^ 1);
    }
}

static struct Ex10GpioDriver const ex10_gpio_driver = {
    .gpio_initialize                 = gpio_initialize,
    .gpio_cleanup                    = gpio_cleanup,
    .set_board_power                 = set_board_power,
    .get_board_power                 = get_board_power,
    .set_ex10_enable                 = set_ex10_enable,
    .get_ex10_enable                 = get_ex10_enable,
    .register_irq_callback           = register_irq_callback,
    .deregister_irq_callback         = deregister_irq_callback,
    .irq_monitor_callback_enable     = irq_monitor_callback_enable,
    .irq_monitor_callback_is_enabled = irq_monitor_callback_is_enabled,
    .irq_enable                      = irq_enable,
    .thread_is_irq_monitor           = thread_is_irq_monitor,
    .assert_reset_n                  = assert_reset_n,
    .deassert_reset_n                = deassert_reset_n,
    .release_ready_n                 = release_ready_n,
    .assert_ready_n                  = assert_ready_n,
    .reset_device                    = reset_device,
    .busy_wait_ready_n               = busy_wait_ready_n,
    .ready_n_pin_get                 = ready_n_pin_get,
    .get_test_pin_level              = get_test_pin_level,
    .debug_pin_get_count             = debug_pin_get_count,
    .debug_pin_get                   = debug_pin_get,
    .debug_pin_set                   = debug_pin_set,
    .debug_pin_toggle                = debug_pin_toggle,
    .led_pin_get_count               = led_pin_get_count,
    .led_pin_get                     = led_pin_get,
    .led_pin_set                     = led_pin_set,
    .led_pin_toggle                  = led_pin_toggle,
};

struct Ex10GpioDriver const* get_ex10_gpio_driver(void)
{
    return &ex10_gpio_driver;
}

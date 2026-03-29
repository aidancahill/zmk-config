/*
 * Analog Joystick Input Driver
 * Reads ADC channels and generates mouse/scroll input events
 *
 * Right joystick: mouse movement + left click
 * Left joystick: scroll + right click
 */

#define DT_DRV_COMPAT zmk_input_analog_joystick

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(analog_joystick, CONFIG_INPUT_LOG_LEVEL);

#define ADC_RESOLUTION 12
#define ADC_MAX ((1 << ADC_RESOLUTION) - 1)
#define ADC_CENTER (ADC_MAX / 2)

struct analog_joystick_config {
    const struct adc_dt_spec x_channel;
    const struct adc_dt_spec y_channel;
    struct gpio_dt_spec button_gpio;
    uint16_t deadzone;
    uint16_t poll_interval_ms;
    int16_t sensitivity;
    bool is_scroll;  /* true = scroll mode, false = mouse mode */
};

struct analog_joystick_data {
    const struct device *dev;
    struct k_work_delayable poll_work;
    int16_t x_center;
    int16_t y_center;
    bool button_state;
    bool calibrated;
};

static int read_adc(const struct adc_dt_spec *spec, int16_t *val) {
    int16_t buf;
    struct adc_sequence seq = {
        .buffer = &buf,
        .buffer_size = sizeof(buf),
    };
    int ret;

    ret = adc_sequence_init_dt(spec, &seq);
    if (ret < 0) {
        return ret;
    }

    ret = adc_read_dt(spec, &seq);
    if (ret < 0) {
        return ret;
    }

    *val = buf;
    return 0;
}

static void joystick_poll(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct analog_joystick_data *data = CONTAINER_OF(dwork, struct analog_joystick_data, poll_work);
    const struct device *dev = data->dev;
    const struct analog_joystick_config *cfg = dev->config;
    int16_t x_raw, y_raw;
    int ret;

    /* Read ADC channels */
    ret = read_adc(&cfg->x_channel, &x_raw);
    if (ret < 0) {
        goto reschedule;
    }

    ret = read_adc(&cfg->y_channel, &y_raw);
    if (ret < 0) {
        goto reschedule;
    }

    /* Calibrate center on first read */
    if (!data->calibrated) {
        data->x_center = x_raw;
        data->y_center = y_raw;
        data->calibrated = true;
        goto reschedule;
    }

    /* Calculate delta from center */
    int16_t dx = x_raw - data->x_center;
    int16_t dy = y_raw - data->y_center;

    /* Apply deadzone */
    if (abs(dx) < cfg->deadzone) {
        dx = 0;
    }
    if (abs(dy) < cfg->deadzone) {
        dy = 0;
    }

    /* Scale by sensitivity */
    if (dx != 0 || dy != 0) {
        int16_t move_x = (dx * cfg->sensitivity) / ADC_CENTER;
        int16_t move_y = (dy * cfg->sensitivity) / ADC_CENTER;

        if (cfg->is_scroll) {
            /* Scroll mode: left joystick */
            if (move_x != 0) {
                input_report_rel(dev, INPUT_REL_HWHEEL, move_x, false, K_FOREVER);
            }
            if (move_y != 0) {
                /* Invert Y for natural scroll */
                input_report_rel(dev, INPUT_REL_WHEEL, -move_y, true, K_FOREVER);
            }
        } else {
            /* Mouse mode: right joystick */
            if (move_x != 0) {
                input_report_rel(dev, INPUT_REL_X, move_x, false, K_FOREVER);
            }
            if (move_y != 0) {
                input_report_rel(dev, INPUT_REL_Y, move_y, true, K_FOREVER);
            }
        }
    }

    /* Read button */
    if (cfg->button_gpio.port != NULL) {
        bool pressed = !gpio_pin_get_dt(&cfg->button_gpio);  /* Active LOW */
        if (pressed != data->button_state) {
            data->button_state = pressed;
            if (cfg->is_scroll) {
                /* Left joystick button = right click */
                input_report_key(dev, INPUT_BTN_RIGHT, pressed ? 1 : 0, true, K_FOREVER);
            } else {
                /* Right joystick button = left click */
                input_report_key(dev, INPUT_BTN_LEFT, pressed ? 1 : 0, true, K_FOREVER);
            }
        }
    }

reschedule:
    k_work_schedule(&data->poll_work, K_MSEC(cfg->poll_interval_ms));
}

static int analog_joystick_init(const struct device *dev) {
    const struct analog_joystick_config *cfg = dev->config;
    struct analog_joystick_data *data = dev->data;
    int ret;

    data->dev = dev;
    data->calibrated = false;
    data->button_state = false;

    /* Setup ADC channels */
    if (!adc_is_ready_dt(&cfg->x_channel)) {
        LOG_ERR("X ADC not ready");
        return -ENODEV;
    }
    ret = adc_channel_setup_dt(&cfg->x_channel);
    if (ret < 0) {
        LOG_ERR("X ADC setup failed: %d", ret);
        return ret;
    }

    if (!adc_is_ready_dt(&cfg->y_channel)) {
        LOG_ERR("Y ADC not ready");
        return -ENODEV;
    }
    ret = adc_channel_setup_dt(&cfg->y_channel);
    if (ret < 0) {
        LOG_ERR("Y ADC setup failed: %d", ret);
        return ret;
    }

    /* Setup button GPIO */
    if (cfg->button_gpio.port != NULL) {
        if (!gpio_is_ready_dt(&cfg->button_gpio)) {
            LOG_ERR("Button GPIO not ready");
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&cfg->button_gpio, GPIO_INPUT | GPIO_PULL_UP);
        if (ret < 0) {
            LOG_ERR("Button GPIO setup failed: %d", ret);
            return ret;
        }
    }

    k_work_init_delayable(&data->poll_work, joystick_poll);
    k_work_schedule(&data->poll_work, K_MSEC(cfg->poll_interval_ms));

    LOG_INF("Analog joystick initialized (%s mode)",
            cfg->is_scroll ? "scroll" : "mouse");
    return 0;
}

#define ANALOG_JOYSTICK_INIT(n)                                             \
    static struct analog_joystick_data joystick_data_##n;                   \
    static const struct analog_joystick_config joystick_cfg_##n = {         \
        .x_channel = ADC_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), 0),           \
        .y_channel = ADC_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), 1),           \
        .button_gpio = GPIO_DT_SPEC_INST_GET_OR(n, button_gpios, {0}),     \
        .deadzone = DT_INST_PROP(n, deadzone),                             \
        .poll_interval_ms = DT_INST_PROP(n, poll_interval_ms),             \
        .sensitivity = DT_INST_PROP(n, sensitivity),                       \
        .is_scroll = DT_INST_PROP(n, scroll_mode),                         \
    };                                                                      \
    DEVICE_DT_INST_DEFINE(n, analog_joystick_init, NULL,                    \
                          &joystick_data_##n, &joystick_cfg_##n,            \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,          \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(ANALOG_JOYSTICK_INIT)

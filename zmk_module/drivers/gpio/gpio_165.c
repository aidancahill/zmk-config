/*
 * 74HC165 PISO Shift Register GPIO Input Driver
 *
 * Uses SPI CS pin (GPIO_ACTIVE_HIGH) as the PL (parallel load) pin:
 * - Between SPI transactions: CS inactive (LOW) = PL LOW = loading parallel data
 * - During SPI transaction: CS active (HIGH) = PL HIGH = shift/serial mode
 */

#define DT_DRV_COMPAT zmk_gpio_165

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_165, CONFIG_GPIO_LOG_LEVEL);

struct gpio_165_config {
    struct gpio_driver_config common;
    struct spi_dt_spec spi;
    uint8_t ngpios;
};

struct gpio_165_data {
    struct gpio_driver_data common;
    struct k_mutex lock;
    uint32_t input_state;
};

static int gpio_165_read(const struct device *dev) {
    const struct gpio_165_config *cfg = dev->config;
    struct gpio_165_data *data = dev->data;
    int bytes = (cfg->ngpios + 7) / 8;
    uint8_t buf[4] = {0};
    int ret;

    struct spi_buf rx_buf = {
        .buf = buf,
        .len = bytes,
    };
    struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1,
    };

    /* SPI read: CS goes HIGH (PL HIGH = shift mode), data clocks in */
    ret = spi_read_dt(&cfg->spi, &rx);
    if (ret < 0) {
        LOG_ERR("SPI read failed: %d", ret);
        return ret;
    }
    /* CS returns LOW (PL LOW = parallel load mode for next read) */

    k_mutex_lock(&data->lock, K_FOREVER);
    data->input_state = 0;
    for (int i = 0; i < bytes; i++) {
        data->input_state |= ((uint32_t)buf[i]) << ((bytes - 1 - i) * 8);
    }
    k_mutex_unlock(&data->lock);

    return 0;
}

static int gpio_165_port_get_raw(const struct device *dev, gpio_port_value_t *value) {
    struct gpio_165_data *data = dev->data;
    int ret;

    ret = gpio_165_read(dev);
    if (ret < 0) {
        return ret;
    }

    k_mutex_lock(&data->lock, K_FOREVER);
    *value = data->input_state;
    k_mutex_unlock(&data->lock);

    return 0;
}

static int gpio_165_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags) {
    const struct gpio_165_config *cfg = dev->config;

    if (pin >= cfg->ngpios) {
        return -EINVAL;
    }

    if (flags & GPIO_OUTPUT) {
        return -ENOTSUP;
    }

    return 0;
}

static int gpio_165_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
                                         gpio_port_value_t value) {
    return -ENOTSUP;
}

static int gpio_165_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins) {
    return -ENOTSUP;
}

static int gpio_165_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins) {
    return -ENOTSUP;
}

static int gpio_165_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins) {
    return -ENOTSUP;
}

static int gpio_165_init(const struct device *dev) {
    const struct gpio_165_config *cfg = dev->config;
    struct gpio_165_data *data = dev->data;

    if (!spi_is_ready_dt(&cfg->spi)) {
        LOG_ERR("SPI device not ready");
        return -ENODEV;
    }

    k_mutex_init(&data->lock);
    data->input_state = 0;

    LOG_INF("74HC165 initialized with %d GPIOs", cfg->ngpios);
    return 0;
}

static DEVICE_API(gpio, gpio_165_api) = {
    .pin_configure = gpio_165_pin_configure,
    .port_get_raw = gpio_165_port_get_raw,
    .port_set_masked_raw = gpio_165_port_set_masked_raw,
    .port_set_bits_raw = gpio_165_port_set_bits_raw,
    .port_clear_bits_raw = gpio_165_port_clear_bits_raw,
    .port_toggle_bits = gpio_165_port_toggle_bits,
};

#define GPIO_165_INIT(n)                                                    \
    static struct gpio_165_data gpio_165_data_##n;                          \
    static const struct gpio_165_config gpio_165_config_##n = {             \
        .common = {                                                         \
            .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),            \
        },                                                                  \
        .spi = SPI_DT_SPEC_INST_GET(n,                                      \
            SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),    \
        .ngpios = DT_INST_PROP(n, ngpios),                                 \
    };                                                                      \
    DEVICE_DT_INST_DEFINE(n, gpio_165_init, NULL,                           \
                          &gpio_165_data_##n, &gpio_165_config_##n,         \
                          POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,           \
                          &gpio_165_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_165_INIT)

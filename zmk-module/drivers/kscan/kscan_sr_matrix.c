/*
 * Custom kscan driver for 74HC595 shift register matrix
 * Uses SPI to drive column shift registers, GPIO to read rows
 */

#define DT_DRV_COMPAT groguv12_kscan_sr_matrix

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(kscan_sr_matrix, CONFIG_KSCAN_LOG_LEVEL);

#define NUM_COLS DT_INST_PROP(0, col_count)
#define NUM_ROWS DT_INST_PROP(0, row_count)
#define SR_BYTES DT_INST_PROP(0, sr_bytes)
#define POLL_INTERVAL_MS DT_INST_PROP(0, poll_interval_ms)
#define DEBOUNCE_MS DT_INST_PROP(0, debounce_ms)

struct kscan_sr_data {
    const struct device *dev;
    kscan_callback_t callback;
    struct k_work_delayable poll_work;
    uint8_t matrix_state[14];  /* one byte per col, bits = rows */
    int64_t debounce_time[14][5];
    bool enabled;
};

struct kscan_sr_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec latch;
    struct gpio_dt_spec rows[5];
};

static void shift_out_col(const struct kscan_sr_config *cfg, int col) {
    uint8_t buf[3] = {0, 0, 0};

    if (col >= 0) {
        int byte_idx = 2 - (col / 8);
        int bit = col % 8;
        buf[byte_idx] = 1 << bit;
    }

    struct spi_buf tx_buf = {
        .buf = buf,
        .len = 3,
    };
    struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };

    spi_write_dt(&cfg->spi, &tx);

    /* Pulse latch */
    gpio_pin_set_dt(&cfg->latch, 1);
    gpio_pin_set_dt(&cfg->latch, 0);
}

static void poll_matrix(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct kscan_sr_data *data = CONTAINER_OF(dwork, struct kscan_sr_data, poll_work);
    const struct device *dev = data->dev;
    const struct kscan_sr_config *cfg = dev->config;
    int64_t now = k_uptime_get();

    for (int col = 0; col < NUM_COLS; col++) {
        shift_out_col(cfg, col);

        /* Small delay for signal to settle */
        k_busy_wait(5);

        for (int row = 0; row < NUM_ROWS; row++) {
            bool pressed = gpio_pin_get_dt(&cfg->rows[row]);
            bool old_pressed = (data->matrix_state[col] >> row) & 1;

            if (pressed != old_pressed) {
                if ((now - data->debounce_time[col][row]) > DEBOUNCE_MS) {
                    if (pressed) {
                        data->matrix_state[col] |= (1 << row);
                    } else {
                        data->matrix_state[col] &= ~(1 << row);
                    }
                    data->debounce_time[col][row] = now;

                    if (data->callback) {
                        data->callback(dev, row, col, pressed);
                    }
                }
            }
        }
    }

    /* Clear shift registers */
    shift_out_col(cfg, -1);

    if (data->enabled) {
        k_work_schedule(&data->poll_work, K_MSEC(POLL_INTERVAL_MS));
    }
}

static int kscan_sr_configure(const struct device *dev, kscan_callback_t callback) {
    struct kscan_sr_data *data = dev->data;
    data->callback = callback;
    return 0;
}

static int kscan_sr_enable(const struct device *dev) {
    struct kscan_sr_data *data = dev->data;
    data->enabled = true;
    k_work_schedule(&data->poll_work, K_NO_WAIT);
    return 0;
}

static int kscan_sr_disable(const struct device *dev) {
    struct kscan_sr_data *data = dev->data;
    data->enabled = false;
    k_work_cancel_delayable(&data->poll_work);
    return 0;
}

static int kscan_sr_init(const struct device *dev) {
    struct kscan_sr_data *data = dev->data;
    const struct kscan_sr_config *cfg = dev->config;
    int ret;

    data->dev = dev;

    /* Configure latch pin */
    if (!gpio_is_ready_dt(&cfg->latch)) {
        LOG_ERR("Latch GPIO not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&cfg->latch, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure latch pin: %d", ret);
        return ret;
    }

    /* Configure row pins as input with pull-down */
    for (int i = 0; i < NUM_ROWS; i++) {
        if (!gpio_is_ready_dt(&cfg->rows[i])) {
            LOG_ERR("Row %d GPIO not ready", i);
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&cfg->rows[i], GPIO_INPUT | GPIO_PULL_DOWN);
        if (ret < 0) {
            LOG_ERR("Failed to configure row %d: %d", i, ret);
            return ret;
        }
    }

    /* Verify SPI is ready */
    if (!spi_is_ready_dt(&cfg->spi)) {
        LOG_ERR("SPI device not ready");
        return -ENODEV;
    }

    /* Init state */
    memset(data->matrix_state, 0, sizeof(data->matrix_state));
    memset(data->debounce_time, 0, sizeof(data->debounce_time));

    /* Clear shift registers */
    shift_out_col(cfg, -1);

    k_work_init_delayable(&data->poll_work, poll_matrix);

    LOG_INF("groguv12 kscan initialized: %d cols, %d rows", NUM_COLS, NUM_ROWS);
    return 0;
}

static const struct kscan_driver_api kscan_sr_api = {
    .config = kscan_sr_configure,
    .enable_callback = kscan_sr_enable,
    .disable_callback = kscan_sr_disable,
};

static struct kscan_sr_data kscan_sr_data_0;

static const struct kscan_sr_config kscan_sr_config_0 = {
    .spi = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),
    .latch = GPIO_DT_SPEC_INST_GET(0, latch_gpios),
    .rows = {
        GPIO_DT_SPEC_INST_GET_BY_IDX(0, row_gpios, 0),
        GPIO_DT_SPEC_INST_GET_BY_IDX(0, row_gpios, 1),
        GPIO_DT_SPEC_INST_GET_BY_IDX(0, row_gpios, 2),
        GPIO_DT_SPEC_INST_GET_BY_IDX(0, row_gpios, 3),
        GPIO_DT_SPEC_INST_GET_BY_IDX(0, row_gpios, 4),
    },
};

DEVICE_DT_INST_DEFINE(0, kscan_sr_init, NULL,
                      &kscan_sr_data_0, &kscan_sr_config_0,
                      POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,
                      &kscan_sr_api);

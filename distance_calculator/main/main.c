#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_err.h"


// ============================================================
// GPIO
// ============================================================

#define TRIG_GPIO GPIO_NUM_5
#define ECHO_GPIO GPIO_NUM_18
#define LED_GPIO  GPIO_NUM_2


// ============================================================
// I2C / OLED
// ============================================================

#define SDA_GPIO  GPIO_NUM_21
#define SCL_GPIO  GPIO_NUM_22

#define OLED_ADDR 0x3C

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t oled;


// ============================================================
// 5x7 FONT
// ============================================================

typedef struct
{
    char character;
    uint8_t pixels[5];

} glyph_t;


static const glyph_t font[] =
{
    { ' ', {0x00,0x00,0x00,0x00,0x00} },

    { '0', {0x3E,0x51,0x49,0x45,0x3E} },
    { '1', {0x00,0x42,0x7F,0x40,0x00} },
    { '2', {0x42,0x61,0x51,0x49,0x46} },
    { '3', {0x21,0x41,0x45,0x4B,0x31} },
    { '4', {0x18,0x14,0x12,0x7F,0x10} },
    { '5', {0x27,0x45,0x45,0x45,0x39} },
    { '6', {0x3C,0x4A,0x49,0x49,0x30} },
    { '7', {0x01,0x71,0x09,0x05,0x03} },
    { '8', {0x36,0x49,0x49,0x49,0x36} },
    { '9', {0x06,0x49,0x49,0x29,0x1E} },

    { 'A', {0x7E,0x11,0x11,0x11,0x7E} },

    { 'C', {0x3E,0x41,0x41,0x41,0x22} },

    { 'D', {0x7F,0x41,0x41,0x22,0x1C} },

    { 'E', {0x7F,0x49,0x49,0x49,0x41} },

    // ADD THESE
    { 'F', {0x7F,0x09,0x09,0x09,0x01} },
    { 'I', {0x00,0x41,0x7F,0x41,0x00} },

    { 'G', {0x3E,0x41,0x49,0x49,0x7A} },

    { 'M', {0x7F,0x02,0x0C,0x02,0x7F} },

    { 'N', {0x7F,0x02,0x04,0x08,0x7F} },

    { 'R', {0x7F,0x09,0x19,0x29,0x46} },

    { 'S', {0x46,0x49,0x49,0x49,0x31} },

    { 'T', {0x01,0x01,0x7F,0x01,0x01} },

    { ':', {0x00,0x36,0x36,0x00,0x00} }
};


// ============================================================
// GET GLYPH
// ============================================================

static const uint8_t *get_glyph(char character)
{
    int count = sizeof(font) / sizeof(font[0]);

    for (int i = 0; i < count; i++)
    {
        if (font[i].character == character)
        {
            return font[i].pixels;
        }
    }

    return font[0].pixels;
}


// ============================================================
// OLED COMMAND
// ============================================================

static void oled_command(uint8_t command)
{
    uint8_t data[2] =
    {
        0x00,
        command
    };

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            oled,
            data,
            2,
            1000
        )
    );
}


// ============================================================
// OLED DATA
// ============================================================

static void oled_data(
    uint8_t *data,
    size_t length)
{
    uint8_t buffer[129];

    buffer[0] = 0x40;

    memcpy(
        &buffer[1],
        data,
        length
    );

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            oled,
            buffer,
            length + 1,
            1000
        )
    );
}


// ============================================================
// I2C INITIALIZATION
// ============================================================

static void i2c_init(void)
{
    i2c_master_bus_config_t bus_config =
    {
        .clk_source = I2C_CLK_SRC_DEFAULT,

        .i2c_port = I2C_NUM_0,

        .sda_io_num = SDA_GPIO,

        .scl_io_num = SCL_GPIO,

        .glitch_ignore_cnt = 7,

        .flags.enable_internal_pullup = true
    };


    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &i2c_bus
        )
    );


    i2c_device_config_t device_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,

        .device_address = OLED_ADDR,

        .scl_speed_hz = 100000
    };


    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            i2c_bus,
            &device_config,
            &oled
        )
    );
}


// ============================================================
// OLED INITIALIZATION
// ============================================================

static void oled_init(void)
{
    oled_command(0xAE);

    oled_command(0xD5);
    oled_command(0x80);

    oled_command(0xA8);
    oled_command(0x3F);

    oled_command(0xD3);
    oled_command(0x00);

    oled_command(0x40);

    oled_command(0x8D);
    oled_command(0x14);

    oled_command(0x20);
    oled_command(0x00);

    oled_command(0xA1);

    oled_command(0xC8);

    oled_command(0xDA);
    oled_command(0x12);

    oled_command(0x81);
    oled_command(0x7F);

    oled_command(0xD9);
    oled_command(0xF1);

    oled_command(0xDB);
    oled_command(0x40);

    oled_command(0xA4);

    oled_command(0xA6);

    oled_command(0xAF);
}


// ============================================================
// SET OLED POSITION
// ============================================================

static void oled_set_position(
    uint8_t page,
    uint8_t column)
{
    oled_command(0xB0 + page);

    oled_command(
        0x00 | (column & 0x0F)
    );

    oled_command(
        0x10 | ((column >> 4) & 0x0F)
    );
}


// ============================================================
// CLEAR OLED
// ============================================================

static void oled_clear(void)
{
    uint8_t blank[128];

    memset(
        blank,
        0x00,
        sizeof(blank)
    );

    for (int page = 0; page < 8; page++)
    {
        oled_set_position(page, 0);

        oled_data(
            blank,
            128
        );
    }
}


// ============================================================
// DRAW ONE CHARACTER AT 2X SIZE
//
// Original: 5 x 7
// Output:   10 x 14
//
// Uses TWO OLED pages.
// ============================================================

static void oled_draw_char_2x(
    char character,
    uint8_t page,
    uint8_t column)
{
    const uint8_t *glyph =
        get_glyph(character);


    uint8_t top[10];
    uint8_t bottom[10];


    memset(top, 0, sizeof(top));
    memset(bottom, 0, sizeof(bottom));


    for (int x = 0; x < 5; x++)
    {
        uint8_t source =
            glyph[x];


        /*
         * Horizontal scaling:
         *
         * Every source column becomes
         * TWO columns.
         */

        for (int y = 0; y < 7; y++)
        {
            if (source & (1 << y))
            {
                int scaled_y =
                    y * 2;


                /*
                 * Vertical scaling:
                 *
                 * Every source pixel becomes
                 * TWO vertical pixels.
                 */


                if (scaled_y < 8)
                {
                    top[x * 2]     |=
                        (1 << scaled_y);

                    top[x * 2 + 1] |=
                        (1 << scaled_y);


                    if (scaled_y + 1 < 8)
                    {
                        top[x * 2]     |=
                            (1 << (scaled_y + 1));

                        top[x * 2 + 1] |=
                            (1 << (scaled_y + 1));
                    }
                }
                else
                {
                    int bottom_y =
                        scaled_y - 8;


                    bottom[x * 2] |=
                        (1 << bottom_y);

                    bottom[x * 2 + 1] |=
                        (1 << bottom_y);


                    if (bottom_y + 1 < 8)
                    {
                        bottom[x * 2] |=
                            (1 << (bottom_y + 1));

                        bottom[x * 2 + 1] |=
                            (1 << (bottom_y + 1));
                    }
                }
            }
        }
    }


    /*
     * Write top half
     */

    oled_set_position(
        page,
        column
    );

    oled_data(
        top,
        10
    );


    /*
     * Write bottom half
     */

    oled_set_position(
        page + 1,
        column
    );

    oled_data(
        bottom,
        10
    );
}


// ============================================================
// DRAW STRING AT 2X SIZE
// ============================================================

static void oled_draw_string_2x(
    const char *text,
    uint8_t page,
    uint8_t column)
{
    while (*text != '\0')
    {
        oled_draw_char_2x(
            *text,
            page,
            column
        );


        /*
         * Character width = 10 pixels
         * Space = 2 pixels
         */

        column += 12;


        text++;
    }
}


// ============================================================
// HC-SR04 MEASUREMENT
// ============================================================

static float measure_distance_cm(void)
{
    gpio_set_level(
        TRIG_GPIO,
        0
    );

    esp_rom_delay_us(2);


    gpio_set_level(
        TRIG_GPIO,
        1
    );

    esp_rom_delay_us(10);

    gpio_set_level(
        TRIG_GPIO,
        0
    );


    /*
     * Wait for ECHO HIGH
     */

    int64_t wait_start =
        esp_timer_get_time();


    while (
        gpio_get_level(ECHO_GPIO) == 0
    )
    {
        if (
            esp_timer_get_time()
            - wait_start > 30000
        )
        {
            return -1.0f;
        }
    }


    int64_t echo_start =
        esp_timer_get_time();


    /*
     * Wait for ECHO LOW
     */

    while (
        gpio_get_level(ECHO_GPIO) == 1
    )
    {
        if (
            esp_timer_get_time()
            - echo_start > 30000
        )
        {
            return -1.0f;
        }
    }


    int64_t echo_end =
        esp_timer_get_time();


    int64_t echo_time =
        echo_end - echo_start;


    return (
        echo_time * 0.0343f
    ) / 2.0f;
}


// ============================================================
// GPIO INITIALIZATION
// ============================================================

static void gpio_init_all(void)
{
    gpio_config_t output_config =
    {
        .pin_bit_mask =
            (1ULL << TRIG_GPIO) |
            (1ULL << LED_GPIO),

        .mode = GPIO_MODE_OUTPUT,

        .pull_up_en =
            GPIO_PULLUP_DISABLE,

        .pull_down_en =
            GPIO_PULLDOWN_DISABLE,

        .intr_type =
            GPIO_INTR_DISABLE
    };


    ESP_ERROR_CHECK(
        gpio_config(
            &output_config
        )
    );


    gpio_config_t echo_config =
    {
        .pin_bit_mask =
            (1ULL << ECHO_GPIO),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en =
            GPIO_PULLUP_DISABLE,

        .pull_down_en =
            GPIO_PULLDOWN_DISABLE,

        .intr_type =
            GPIO_INTR_DISABLE
    };


    ESP_ERROR_CHECK(
        gpio_config(
            &echo_config
        )
    );
}


// ============================================================
// MAIN
// ============================================================

void app_main(void)
{
    printf("\n");
    printf("==============================\n");
    printf(" ESP32 DISTANCE MONITOR\n");
    printf("==============================\n");


    gpio_init_all();

    i2c_init();

    oled_init();

    oled_clear();


    while (1)
    {
        float distance =
            measure_distance_cm();


        oled_clear();


        if (distance < 0)
        {
            printf("Echo timeout\n");

            oled_draw_string_2x(
                "DANGER",
                2,
                28
            );

            gpio_set_level(
                LED_GPIO,
                0
            );
        }
        else
        {
            int distance_int =
                (int)distance;


            if (distance_int > 999)
            {
                distance_int = 999;
            }


            printf(
                "Distance: %d cm\n",
                distance_int
            );


            char distance_text[20];


            snprintf(
                distance_text,
                sizeof(distance_text),
                "DIST:%d CM",
                distance_int
            );


            /*
             * DISTANCE
             *
             * 10 characters maximum
             * at 12 pixels each = 120 pixels
             */

            oled_draw_string_2x(
                distance_text,
                0,
                4
            );


            /*
             * DANGER / SAFE
             */

            if (distance < 50.0f)
            {
                gpio_set_level(
                    LED_GPIO,
                    1
                );


                oled_draw_string_2x(
                    "DANGER",
                    4,
                    28
                );
            }
            else
            {
                gpio_set_level(
                    LED_GPIO,
                    0
                );


                oled_draw_string_2x(
                    "SAFE",
                    4,
                    40
                );
            }
        }


        vTaskDelay(
            pdMS_TO_TICKS(500)
        );
    }
}
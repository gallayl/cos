#include "display.h"
#include "cyd_board_config.h"

#include "esp_log.h"
#include <LovyanGFX.hpp>

static const char *const TAG = "display";

class CydDisplay : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341_2 panel;
    lgfx::Bus_SPI bus;
    lgfx::Light_PWM backlight;
    lgfx::Touch_XPT2046 touch;

  public:
    CydDisplay()
    {
        {
            auto cfg = bus.config();
            cfg.spi_host = CYD_DISP_SPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = CYD_DISP_SPI_FREQ_WRITE;
            cfg.freq_read = CYD_DISP_SPI_FREQ_READ;
            cfg.pin_sclk = CYD_DISP_PIN_SCLK;
            cfg.pin_mosi = CYD_DISP_PIN_MOSI;
            cfg.pin_miso = CYD_DISP_PIN_MISO;
            cfg.pin_dc = CYD_DISP_PIN_DC;
            bus.config(cfg);
            panel.setBus(&bus);
        }

        {
            auto cfg = panel.config();
            cfg.pin_cs = CYD_DISP_PIN_CS;
            cfg.pin_rst = CYD_DISP_PIN_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = CYD_PANEL_WIDTH;
            cfg.panel_height = CYD_PANEL_HEIGHT;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = CYD_PANEL_OFFSET_ROTATION;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = CYD_PANEL_READABLE;
            cfg.invert = CYD_PANEL_INVERT;
            cfg.rgb_order = CYD_PANEL_RGB_ORDER;
            cfg.dlen_16bit = false;
            cfg.bus_shared = CYD_PANEL_BUS_SHARED;
            panel.config(cfg);
        }

        {
            auto cfg = backlight.config();
            cfg.pin_bl = CYD_BL_PIN;
            cfg.invert = CYD_BL_INVERT;
            cfg.freq = CYD_BL_FREQ;
            cfg.pwm_channel = CYD_BL_PWM_CHANNEL;
            backlight.config(cfg);
            panel.setLight(&backlight);
        }

        {
            auto cfg = touch.config();
            cfg.x_min = CYD_TOUCH_X_MIN;
            cfg.x_max = CYD_TOUCH_X_MAX;
            cfg.y_min = CYD_TOUCH_Y_MIN;
            cfg.y_max = CYD_TOUCH_Y_MAX;
            cfg.pin_int = CYD_TOUCH_PIN_INT;
            cfg.bus_shared = CYD_TOUCH_BUS_SHARED;
            cfg.offset_rotation = CYD_TOUCH_OFFSET_ROTATION;
            cfg.spi_host = CYD_TOUCH_SPI_HOST;
            cfg.freq = CYD_TOUCH_SPI_FREQ;
            cfg.pin_sclk = CYD_TOUCH_PIN_SCLK;
            cfg.pin_mosi = CYD_TOUCH_PIN_MOSI;
            cfg.pin_miso = CYD_TOUCH_PIN_MISO;
            cfg.pin_cs = CYD_TOUCH_PIN_CS;
            touch.config(cfg);
            panel.setTouch(&touch);
        }

        setPanel(&panel);
    }
};

static CydDisplay lcd;
static bool initialized = false;
static uint8_t current_brightness = CYD_BL_DEFAULT_BRIGHTNESS;

extern "C" esp_err_t display_init(void)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "Display already initialized");
        return ESP_OK;
    }

    lcd.init();
    lcd.setBrightness(CYD_BL_DEFAULT_BRIGHTNESS);
    initialized = true;

    ESP_LOGI(TAG, "Display initialized (%dx%d)", CYD_PANEL_WIDTH, CYD_PANEL_HEIGHT);
    return ESP_OK;
}

extern "C" uint8_t display_get_rotation(void)
{
    return lcd.getRotation();
}

extern "C" esp_err_t display_set_rotation(uint8_t rotation)
{
    if (rotation > 7)
    {
        return ESP_ERR_INVALID_ARG;
    }
    lcd.setRotation(rotation);
    return ESP_OK;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- cal_data is an output buffer filled by calibrateTouch()
extern "C" esp_err_t display_calibrate_touch(uint16_t cal_data[DISPLAY_CAL_DATA_LEN])
{
    if (cal_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    lcd.fillScreen(TFT_BLACK);
    lcd.setCursor(20, 0);
    lcd.setTextFont(2);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.println("Touch corners as indicated");
    lcd.setTextFont(1);
    lcd.println();
    lcd.printf("Calibrating for rotation: %d", lcd.getRotation());

    lcd.calibrateTouch(cal_data, TFT_MAGENTA, TFT_BLACK, 15);

    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.println("\nCalibration complete!");
    lcd.waitDisplay();

    ESP_LOGI(TAG, "Touch calibration complete (rotation %d)", lcd.getRotation());
    return ESP_OK;
}

extern "C" esp_err_t display_set_touch_calibration(const uint16_t cal_data[DISPLAY_CAL_DATA_LEN])
{
    if (cal_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    lcd.setTouchCalibrate(const_cast<uint16_t *>(cal_data));
    return ESP_OK;
}

extern "C" esp_err_t display_set_brightness(uint8_t brightness)
{
    lcd.setBrightness(brightness);
    current_brightness = brightness;
    return ESP_OK;
}

extern "C" uint8_t display_get_brightness(void)
{
    return current_brightness;
}

extern "C" void display_fill_screen(uint16_t color)
{
    lcd.fillScreen(color);
}

extern "C" void display_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    lcd.fillRect(x, y, w, h, color);
}

extern "C" void display_draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg)
{
    lcd.setCursor(x, y);
    lcd.setTextColor(fg, bg);
    lcd.setTextSize(1);
    lcd.print(text);
}

extern "C" void display_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg)
{
    lcd.setCursor(x, y);
    lcd.setTextColor(fg, bg);
    lcd.setTextSize(1);
    lcd.write(static_cast<uint8_t>(c));
}

extern "C" int display_get_width(void)
{
    return lcd.width();
}

extern "C" int display_get_height(void)
{
    return lcd.height();
}

extern "C" void display_set_text_font(uint8_t font_id)
{
    lcd.setTextFont(font_id);
}

extern "C" void display_start_write(void)
{
    lcd.startWrite();
}

extern "C" void display_end_write(void)
{
    lcd.endWrite();
}

extern "C" void display_wait(void)
{
    lcd.waitDisplay();
}

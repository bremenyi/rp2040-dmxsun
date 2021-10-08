#include "statusleds.h"

#include "ws2812.pio.h"           // Header file for the PIO program

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio1, 3, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
      ((uint32_t) (r) << 8) |
      ((uint32_t) (g) << 16) |
      (uint32_t) (b);
}

void StatusLeds::init() {
    // Create the program for the status LEDS in PIO1, SM3
    pio_program = pio_add_program(pio1, &ws2812_program);
    ws2812_program_init(pio1, 3, pio_program, PIN_LEDS, 800000, false);

    // * 4 is sizeof(uint32_t)
    memset(this->pixels, 0x00, 8 * 4);
    memset(this->toBlinkOn, 0x00, 8 * 3 * 4);
    memset(this->toBlinkOff, 0x00, 8 * 3 * 4);
}

void StatusLeds::setStatic(uint8_t ledNum, bool red, bool green, bool blue) {
    pixels[ledNum] = (green ? 255 : 0) << 16 | (red ? 255 : 0) << 8 | (blue ? 255 : 0);
}

void StatusLeds::setStaticOn(uint8_t ledNum, bool red, bool green, bool blue) {
    uint32_t pixel = pixels[ledNum];
    if (red) {
        pixel = pixel | 0x0000ff00;
    }
    if (green) {
        pixel = pixel | 0x00ff0000;
    }
    if (blue) {
        pixel = pixel | 0x000000ff;
    }
    pixels[ledNum] = pixel;
}

void StatusLeds::setStaticOff(uint8_t ledNum, bool red, bool green, bool blue) {
    uint32_t pixel = pixels[ledNum];
    if (red) {
        pixel = pixel & 0xffff00ff;
    }
    if (green) {
        pixel = pixel & 0xff00ffff;
    }
    if (blue) {
        pixel = pixel & 0xffffff00;
    }
    pixels[ledNum] = pixel;
}

void StatusLeds::setBlinkOnce(uint8_t ledNum, bool red, bool green, bool blue) {
    uint32_t time_now = board_millis();
    if (!toBlinkOn[ledNum][0] && red) {
        if (toBlinkOff[ledNum][0]) {
            toBlinkOn[ledNum][0] = toBlinkOff[ledNum][0] + 200;
        } else {
            toBlinkOn[ledNum][0] = time_now;
        }
    }
    if (!toBlinkOn[ledNum][1] && green) {
        if (toBlinkOff[ledNum][1]) {
            toBlinkOn[ledNum][1] = toBlinkOff[ledNum][1] + 200;
        } else {
            toBlinkOn[ledNum][1] = time_now;
        }
    }
    if (!toBlinkOn[ledNum][2] && blue) {
        if (toBlinkOff[ledNum][2]) {
            toBlinkOn[ledNum][2] = toBlinkOff[ledNum][2] + 200;
        } else {
            toBlinkOn[ledNum][2] = time_now;
        }
    }
};

void StatusLeds::cyclicTask() {
    uint32_t time_now = board_millis();

    if ((lastRefresh + 10) > time_now) {
        return;
    }
    lastRefresh = time_now;

    for (uint8_t i = 0; i < 8; i++) {
        if (toBlinkOff[i][0] && (toBlinkOff[i][0] < time_now)) {
            this->setStaticOff(i, 1, 0, 0);
            toBlinkOff[i][0] = 0;
        }
        if (toBlinkOff[i][1] && (toBlinkOff[i][1] < time_now)) {
            this->setStaticOff(i, 0, 1, 0);
            toBlinkOff[i][1] = 0;
        }
        if (toBlinkOff[i][2] && (toBlinkOff[i][2] < time_now)) {
            this->setStaticOff(i, 0, 0, 1);
            toBlinkOff[i][2] = 0;
        }

        if (toBlinkOn[i][0] && (toBlinkOn[i][0] < time_now)) {
            this->setStaticOn(i, 1, 0, 0);
            toBlinkOn[i][0] = 0;
            toBlinkOff[i][0] = time_now + 200;
        }
        if (toBlinkOn[i][1] && (toBlinkOn[i][1] < time_now)) {
            this->setStaticOn(i, 0, 1, 0);
            toBlinkOn[i][1] = 0;
            toBlinkOff[i][1] = time_now + 200;
        }
        if (toBlinkOn[i][2] && (toBlinkOn[i][2] < time_now)) {
            this->setStaticOn(i, 0, 0, 1);
            toBlinkOn[i][2] = 0;
            toBlinkOff[i][2] = time_now + 200;
        }
    }

    this->writeLeds();
}

void StatusLeds::setBrightness(uint8_t brightness) {
    this->brightness = brightness;
    this->writeLeds();
}

void StatusLeds::writeLeds() {
    for (uint8_t i = 0; i < 8; i++) {
        if (this->brightness == 255) {
            put_pixel(this->pixels[i]);
        } else if (this->brightness == 0) {
            put_pixel(0);
        } else {
            uint8_t r = this->pixels[i] >> 8;
            uint8_t g = this->pixels[i] >> 16;
            uint8_t b = this->pixels[i] & 0xff;
            r = (uint16_t)r * this->brightness / 255;
            g = (uint16_t)g * this->brightness / 255;
            b = (uint16_t)b * this->brightness / 255;
            put_pixel(g << 16 | r << 8 | b);
        }
    }
}
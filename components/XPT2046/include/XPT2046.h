#ifndef _XPT2046_H
#define _XPT2046_H

#include "string.h"
#include "stdio.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_partition.h"
#include "freertos/semphr.h"
#include "esp_log.h"

typedef struct position {
    int x;
    int y;
} position;


typedef struct {
//    uint8_t pin_num_miso;        /*!<MasterIn, SlaveOut pin*/
//    uint8_t pin_num_mosi;        /*!<MasterOut, SlaveIn pin*/
//    uint8_t pin_num_clk;         /*!<SPI Clock pin*/
    uint8_t pin_num_cs;          /*!<SPI Chip Select Pin*/
//    uint8_t pin_num_dc;          /*!<Pin to select Data or Command for LCD*/
//    uint8_t pin_num_rst;         /*!<Pin to hardreset LCD*/
//    uint8_t pin_num_bckl;        /*!<Pin for adjusting Backlight- can use PWM/DAC too*/
    int clk_freq;                /*!< spi clock frequency */
//    uint8_t rst_active_level;    /*!< reset pin active level */
//    uint8_t bckl_active_level;   /*!< back-light active level */
    spi_host_device_t spi_host;  /*!< spi host index*/
} xpt_conf_t;

typedef struct {
    uint8_t dc_io;
    uint8_t dc_level;
} xpt_dc_t;

#define TOUCH_CMD_X       0xD0
#define TOUCH_CMD_Y       0x90
#define XPT2046_SMPSIZE   20

#ifdef __cplusplus

class XPT2046 {

private:
    spi_device_handle_t spi_wr = NULL;

public:
    XPT2046(xpt_conf_t * xpt_conf, int w, int h);

    int x();
    int y();

    bool isPressed();

    void initializeDevice();

    void setRotation(int r);

    int getSample(uint8_t pin);

    void sample();

    void calibration();

private:
    position pos;
    bool pressed;
    int _rotation;
    int _offset_x;
    int _offset_y;
    int _width;
    int _height;
    int _scale_x;
    int _scale_y;
};

#endif

#endif

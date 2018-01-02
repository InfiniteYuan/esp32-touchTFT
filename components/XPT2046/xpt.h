#ifndef _XPT_H
#define _XPT_H
#include "XPT2046.h"

#ifdef __cplusplus
extern "C" {
#endif

void init(xpt_conf_t * xpt_conf, spi_device_handle_t * spi_wr);
uint16_t xpt_data(spi_device_handle_t spi, const uint8_t data, int len);

#ifdef __cplusplus
}
#endif

#endif

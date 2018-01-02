#include "xpt.h"

void init(xpt_conf_t * xpt_conf, spi_device_handle_t * spi_wr)
{

//    spi_bus_config_t buscfg = {
//        .miso_io_num = GPIO_NUM_25,
//        .mosi_io_num = GPIO_NUM_23,
//        .sclk_io_num = GPIO_NUM_19,
//        .quadwp_io_num = -1,
//        .quadhd_io_num = -1,
//    };
//    spi_bus_initialize(HSPI_HOST, &buscfg, 1);


    spi_device_interface_config_t devcfg = {
        // Use low speed to read ID.
        .clock_speed_hz = xpt_conf->clk_freq,     //Clock out frequency
        .mode = 0,                                //SPI mode 0
        .spics_io_num = xpt_conf->pin_num_cs,     //CS pin
        .queue_size = 7,                          //We want to be able to queue 7 transactions at a time
    };

    spi_bus_add_device((spi_host_device_t) xpt_conf->spi_host, &devcfg, spi_wr);
}

uint16_t xpt_data(spi_device_handle_t spi, const uint8_t data, int len)
{
    esp_err_t ret;
    uint8_t datas[3] = {0};
    datas[0] = data;
    if (len == 0) {
        return 0;    //no need to send anything
    }

    spi_transaction_t t = {
        .length = len * 8 * 3,              // Len is in bytes, transaction length is in bits.
        .tx_buffer = &datas,              // Data
        .flags = SPI_TRANS_USE_RXDATA,
    };
    ret = spi_device_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);              // Should have had no issues.
//    ESP_LOGI("XPT2046", "data0:%d,data1:%d,data2:%d,data3:%d", t.rx_data[0],t.rx_data[1],t.rx_data[2],t.rx_data[3]);

    return (t.rx_data[1] << 8 | t.rx_data[2]) >> 3;
}

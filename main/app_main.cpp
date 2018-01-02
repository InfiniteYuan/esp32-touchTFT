/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "iot_xpt2046.h"
#include "lwip/api.h"
#include "bitmap.h"
#include "iot_lcd.h"
#include "iot_wifi_conn.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "include/app_touch.h"

static const char* TAG = "ESP-CAM";
CXpt2046 *xpt;

/* wifi AP */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            break;
        default:
            break;
    }
    return ESP_OK;
}

void initialise_wifi(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t wifi_config;
    memcpy(wifi_config.ap.ssid, "123456789", sizeof("123456789"));
    memcpy(wifi_config.ap.password, "123456789", sizeof("123456789"));
    wifi_config.ap.ssid_len = strlen("123456789");
    wifi_config.ap.max_connection = 1;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    esp_wifi_start();
}
/* wifi AP */

void touch_calibration()
{
    uint16_t px[2], py[2], xPot[4], yPot[4];
    float xFactor, yFactor;
    int _offset_x, _offset_y;

    lcd_display_top_left();
    ESP_LOGD(TAG, "please input top-left button...");
    do{
        xpt->sample();
    }while(!xpt->is_pressed());
    xPot[0] = xpt->getposition().x;
    yPot[0] = xpt->getposition().y;
    ESP_LOGD(TAG, "X:%d; Y:%d.", xPot[0], yPot[0]);

    lcd_display_top_right();
    vTaskDelay(500 / portTICK_RATE_MS);
    ESP_LOGD(TAG, "please input top-right button...");
    do{
        xpt->sample();
    }while(!xpt->is_pressed());
    xPot[1] = xpt->getposition().x;
    yPot[1] = xpt->getposition().y;
    ESP_LOGD(TAG, "X:%d; Y:%d.", xPot[1], yPot[1]);

    lcd_display_bottom_right();
    vTaskDelay(500 / portTICK_RATE_MS);
    ESP_LOGD(TAG, "please input bottom-right button...");
    do{
        xpt->sample();
    }while(!xpt->is_pressed());
    xPot[2] = xpt->getposition().x;
    yPot[2] = xpt->getposition().y;
    ESP_LOGD(TAG, "X:%d; Y:%d.", xPot[2], yPot[2]);

    lcd_display_bottom_left();
    vTaskDelay(500 / portTICK_RATE_MS);
    ESP_LOGD(TAG, "please input bottom-left button...");
    do{
        xpt->sample();
    }while(!xpt->is_pressed());
    xPot[3] = xpt->getposition().x;
    yPot[3] = xpt->getposition().y;
    ESP_LOGD(TAG, "X:%d; Y:%d.", xPot[3], yPot[3]);

    /* 处理读取到的四个点的数据，整合成对角的两个点 */
    px[0] = (xPot[0] + xPot[1]) / 2;
    py[0] = (yPot[0] + yPot[3]) / 2;

    px[1] = (xPot[2] + xPot[3]) / 2;
    py[1] = (yPot[2] + yPot[1]) / 2;
    ESP_LOGD(TAG, "X:%d; Y:%d.", px[0], py[0]);
    ESP_LOGD(TAG, "X:%d; Y:%d.", px[1], py[1]);

    /* 求出比例因数 */
    xFactor = (float) 240 / (px[1] - px[0]);
    yFactor = (float) 320 / (py[1] - py[0]);

    /* 求出偏移量 */
    _offset_x = (int16_t) 240 - ((float) px[1] * xFactor);
    _offset_y = (int16_t) 320 - ((float) py[1] * yFactor);

    xpt->set_offset(xFactor, yFactor, _offset_x, _offset_y);
    ESP_LOGD(TAG, "xFactor:%f, yFactor:%f, Offset X:%d; Y:%d.", xFactor, yFactor, _offset_x, _offset_y);
    clear_screen();
}

void init_xpt2046()
{
    xpt_conf_t xpt_conf = {
            .pin_num_cs = GPIO_NUM_26,
            .clk_freq = 1 * 1000 * 1000,
            .spi_host = HSPI_HOST,
    };
    if(xpt == NULL){
        xpt= new CXpt2046(&xpt_conf, 320, 240);
    }
    xpt->set_rotation(0);
    touch_calibration();
}

void sample()
{
    while(1){
        xpt->sample();
        if(xpt->is_pressed()){
            ESP_LOGI("XPT2046", "getSample x: %d ; y: %d", xpt->x(), xpt->y());
        }
    }
}

extern "C" void app_main()
{
    app_lcd_init();
    init_xpt2046();
    lcd_init_wifi();

    CWiFi *my_wifi = CWiFi::GetInstance(WIFI_MODE_STA);
    printf("connect wifi\n");
    my_wifi->Connect(WIFI_SSID, WIFI_PASSWORD, portMAX_DELAY);
    ip4_addr_t s_ip_addr = my_wifi->IP();

    lcd_wifi_connect_complete();
    // VERY UNSTABLE without this delay after init'ing wifi... however, much more stable with a new Power Supply
    vTaskDelay(500 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Free heap: %u", xPortGetFreeHeapSize());

    sample();
}

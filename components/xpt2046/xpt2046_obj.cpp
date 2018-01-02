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

CXpt2046::CXpt2046(xpt_conf_t * xpt_conf, int w, int h)
{
    iot_xpt2046_init(xpt_conf, &m_spi);
    m_pressed = false;
    m_rotation = 0;
    m_offset_x = 0;
    m_offset_y = 0;
    m_width = w;
    m_height = h;
    m_xfactor = 0;
    m_yfactor = 0;
}

bool CXpt2046::is_pressed()
{
    return m_pressed;
}

int CXpt2046::get_sample(uint8_t command)
{
//    ESP_LOGI("XPT2046", "getSample");
    return iot_xpt2046_readdata(m_spi, command, 1);
}

void CXpt2046::sample()
{
    position samples[XPT2046_SMPSIZE];
    position distances[XPT2046_SMPSIZE];
    m_pressed = true;

    int aveX = 0;
    int aveY = 0;

    for (int i = 0; i < XPT2046_SMPSIZE; i++) {
        samples[i].x = get_sample(TOUCH_CMD_X);
        samples[i].y = get_sample(TOUCH_CMD_Y);
//        ESP_LOGI("XPT2046", "getSample:%d, y:%d", samples[i].x, samples[i].y);
        if (samples[i].x == 0 || samples[i].x == 4095 || samples[i].y == 0
                || samples[i].y == 4095) {
            m_pressed = false;
        }
        aveX += samples[i].x;
        aveY += samples[i].y;
    }

    aveX /= XPT2046_SMPSIZE;
    aveY /= XPT2046_SMPSIZE;
    for (int i = 0; i < XPT2046_SMPSIZE; i++) {
        distances[i].x = i;
        distances[i].y = ((aveX - samples[i].x) * (aveX - samples[i].x))
                + ((aveY - samples[i].y) * (aveY - samples[i].y));
    }

    for (int i = 0; i < XPT2046_SMPSIZE - 1; i++) {
        for (int j = 0; j < XPT2046_SMPSIZE - 1; j++) {
            if (samples[j].y > samples[j + 1].y) {
                int yy = samples[j + 1].y;
                samples[j + 1].y = samples[j].y;
                samples[j].y = yy;
                int xx = samples[j + 1].x;
                samples[j + 1].x = samples[j].x;
                samples[j].x = xx;
            }
        }
    }

    int tx = 0;
    int ty = 0;
    for (int i = 0; i < (XPT2046_SMPSIZE >> 1); i++) {
        tx += samples[distances[i].x].x;
        ty += samples[distances[i].x].y;
    }

    m_pos.x = tx / (XPT2046_SMPSIZE >> 1);
    m_pos.y = ty / (XPT2046_SMPSIZE >> 1);
}

void CXpt2046::set_rotation(int r)
{
    m_rotation = r % 4;
}

position CXpt2046::getposition()
{
    return m_pos;
}

int CXpt2046::y()
{
    int x = m_offset_x + m_pos.x * m_xfactor;
    int y = m_offset_y + m_pos.y * m_yfactor;

    if(x > m_height){
        x = m_height;
    } else if(x < 0){
        x = 0;
    }
    if(y > m_width){
        y = m_width;
    } else if(y < 0){
        y = 0;
    }
    switch (m_rotation) {
        case 0:
            return x;
        case 3:
            return m_height - y;
        case 2:
            return m_width - x;
        case 1:
            return y;
    }
    return 0;
}

int CXpt2046::x()
{
    int x = m_offset_x + m_pos.x * m_xfactor;
    int y = m_offset_y + m_pos.y * m_yfactor;

    if(x > m_height){
        x = m_height;
    } else if(x < 0){
        x = 0;
    }
    if(y > m_width){
        y = m_width;
    } else if(y < 0){
        y = 0;
    }
    switch (m_rotation) {
        case 0:
            return y;
        case 3:
            return x;
        case 2:
            return m_height - y;
        case 1:
            return m_width - x;
    }
    return 0;
}

void CXpt2046::calibration()
{
    uint16_t px[2], py[2], xPot[4], yPot[4];

    //left-top
    while (!is_pressed()) {
        sample();
    }
    xPot[0] = x();
    yPot[0] = y();

    //right-top
    while (!is_pressed()) {
        sample();
    }
    xPot[1] = x();
    yPot[1] = y();

    //right-bottom
    while (!is_pressed()) {
        sample();
    }
    xPot[2] = x();
    yPot[2] = y();

    //left-bottom
    while (!is_pressed()) {
        sample();
    }
    xPot[3] = x();
    yPot[3] = y();

    /* 处理读取到的四个点的数据，整合成对角的两个点 */
    px[0] = (xPot[0] + xPot[3]) / 2;
    py[0] = (yPot[0] + yPot[1]) / 2;

    px[1] = (xPot[2] + yPot[1]) / 2;
    py[1] = (yPot[2] + yPot[3]) / 2;

    /* 求出比例因数 */
    m_xfactor = (float) m_width / (px[1] - px[0]);
    m_yfactor = (float) m_height / (py[1] - py[0]);

    /* 求出偏移量 */
    m_offset_x = (int16_t) m_width - ((float) px[1] * m_xfactor);
    m_offset_y = (int16_t) m_height - ((float) py[1] * m_yfactor);
}

void CXpt2046::set_offset(float xfactor, float yfactor, int x_offset, int y_offset)
{
    m_xfactor = xfactor;
    m_yfactor = yfactor;
    m_offset_x = x_offset;
    m_offset_y = y_offset;
}


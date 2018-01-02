#include "XPT2046.h"
#include "xpt.h"

XPT2046::XPT2046(xpt_conf_t * xpt_conf, int w, int h)
{
    init(xpt_conf, &spi_wr);
    pressed = false;
    _rotation = 0;
    _scale_x = 1.0;
    _scale_y = 1.0;
    _offset_x = 0;
    _offset_y = 0;
    _width = w;
    _height = h;
}

bool XPT2046::isPressed()
{
    return pressed;
}

int XPT2046::y() {
    int x = _offset_x + ((float)(pos.x * _width / 4096) * _scale_x);
    int y = _offset_y + ((float)(pos.y * _height / 4096) * _scale_y);

    switch (_rotation) {
        case 0:
            return x;
        case 3:
            return _height - y;
        case 2:
            return _width - x;
        case 1:
            return y;
    }
    return 0;
}

int XPT2046::x() {
    int x = _offset_x + ((float)(pos.x * _width / 4096) * _scale_x);
    int y = _offset_y + ((float)(pos.y * _height / 4096) * _scale_y);

    switch (_rotation) {
        case 0:
            return y;
        case 3:
            return x;
        case 2:
            return _height - y;
        case 1:
            return _width - x;
    }
    return 0;
}

int XPT2046::getSample(uint8_t pin)
{
//    ESP_LOGI("XPT2046", "getSample");
    return xpt_data(spi_wr, pin, 1);
}

void XPT2046::sample()
{
    position samples[XPT2046_SMPSIZE];
    position distances[XPT2046_SMPSIZE];
    pressed = true;

    int aveX = 0;
    int aveY = 0;

    for (int i = 0; i < XPT2046_SMPSIZE; i++) {
        samples[i].x = getSample(TOUCH_CMD_X);
        samples[i].y = getSample(TOUCH_CMD_Y);
//        ESP_LOGI("XPT2046", "getSample:%d, y:%d", samples[i].x, samples[i].y);
        if (samples[i].x == 0 || samples[i].x == 4095 || samples[i].y == 0 || samples[i].y == 4095){
            pressed = false;
        }
        aveX += samples[i].x;
        aveY += samples[i].y;
    }

    aveX /= XPT2046_SMPSIZE;
    aveY /= XPT2046_SMPSIZE;
    for (int i = 0; i < XPT2046_SMPSIZE; i++) {
        distances[i].x = i;
        distances[i].y = ((aveX - samples[i].x) * (aveX - samples[i].x)) + ((aveY - samples[i].y) * (aveY - samples[i].y));
    }

    for (int i = 0; i < XPT2046_SMPSIZE-1; i++) {
        for (int j = 0; j < XPT2046_SMPSIZE-1; j++) {
            if (samples[j].y > samples[j+1].y) {
                int yy = samples[j+1].y;
                samples[j+1].y = samples[j].y;
                samples[j].y = yy;
                int xx = samples[j+1].x;
                samples[j+1].x = samples[j].x;
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

    pos.x = tx / (XPT2046_SMPSIZE >> 1);
    pos.y = ty / (XPT2046_SMPSIZE >> 1);
}

void XPT2046::setRotation(int r) {
    _rotation = r % 4;
}

void XPT2046::calibration()
{
    uint16_t px[2], py[2], xPot[4], yPot[4];
    float xFactor, yFactor;

    //left-top
    while(!isPressed()){
        sample();
    }
    xPot[0] = x();
    yPot[0] = y();

    //right-top
    while(!isPressed()){
        sample();
    }
    xPot[1] = x();
    yPot[1] = y();

    //right-bottom
    while(!isPressed()){
        sample();
    }
    xPot[2] = x();
    yPot[2] = y();

    //left-bottom
    while(!isPressed()){
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
    xFactor = (float)_width / (px[1] - px[0]);
    yFactor = (float)_height / (py[1] - py[0]);

    /* 求出偏移量 */
    _offset_x = (int16_t)_width - ((float)px[1] * xFactor);
    _offset_y = (int16_t)_height - ((float)py[1] * yFactor);
}
















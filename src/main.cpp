/**
 * @file encoder.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief M5Dial Encoder Test
 * @version 0.1
 * @date 2023-09-26
 *
 *
 * @Hardwares: M5Dial
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#include "M5Dial.h"
#include "ui/ui.h"
#include "esp_task_wdt.h"
#include "lvgl.h"
#include <WiFi.h>

const char *ssid = "xxxxxxxxxxx"; // 替换为您的WiFi网络名称
const char *password = "xxxxxxxxxxxx"; // 替换为您的WiFi网络密码

const char *host = "192.168.8.223"; // 替换为您要访问的API地址
const int httpPort = 8123;

#define MY_DISP_HOR_RES (240) // w
#define MY_DISP_VER_RES (240) // h
#define BUFF_LEN 5
SemaphoreHandle_t lvgl_xHandler;
static lv_disp_draw_buf_t draw_buf_dsc_1;
static lv_color_t buf_1[MY_DISP_HOR_RES * BUFF_LEN];
static lv_disp_drv_t disp_drv;
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5Dial.Display.drawBitmap(area->x1, area->y1, w, h, &color_p->full);
    lv_disp_flush_ready(disp_drv);
}
void ui_init_fun()
{
    lv_init();
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * BUFF_LEN);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc_1;
    lv_disp_drv_register(&disp_drv);
    ui_init();
}
void ui_Task(void *ptParam)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
        xSemaphoreTake(lvgl_xHandler, portMAX_DELAY);
        lv_timer_handler();
        xSemaphoreGive(lvgl_xHandler);
    }
}

long oldPosition = -999;
long oldPosition1 = -999;
long ha_flage = -999;

char post_request[1024];
char post_request1[100];
void homeassistant_Task(void *ptParam)
{
    WiFi.begin(ssid, password);
    // while (WiFi.status() != WL_CONNECTED)
    // {
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
    while (1)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            if (ha_flage != oldPosition)
            {
                WiFiClient client;
                if (!client.connect(host, httpPort))
                {
                    // Serial.println("无法连接到服务器");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    continue;
                }

                int req_len = sprintf(post_request1, "{\"entity_id\": \"light.m5stack_atom_matrix_light\", \"brightness\": %d}", oldPosition);
                char *str_p = post_request;
                str_p += sprintf(str_p, "POST /api/services/light/turn_on HTTP/1.1\r\n");
                str_p += sprintf(str_p, "Host: 192.168.8.223:8123\r\n");
                str_p += sprintf(str_p, "User-Agent: curl/7.81.0\r\n");
                str_p += sprintf(str_p, "Accept: */*\r\n");
                str_p += sprintf(str_p, "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiIzOWFlYWQzM2RiMGQ0NWE1YTk0M2JhMmQ5ODEzMjU5OSIsImlhdCI6MTcxMDMyNTc3NSwiZXhwIjoyMDI1Njg1Nzc1fQ.yrI99gY13FvDjAIVt3o9vwXRMjHL26BINY37aIEggIg\r\n");
                str_p += sprintf(str_p, "Content-Type: application/json\r\n");
                str_p += sprintf(str_p, "Content-Length: %d\r\n", req_len);
                str_p += sprintf(str_p, "\r\n");
                str_p += sprintf(str_p, "%s", post_request1);
                client.write(post_request, (int)(str_p - post_request));

                // while (client.connected())
                // {
                //     client.read();
                // }
                vTaskDelay(pdMS_TO_TICKS(100));
                client.stop();
                ha_flage = oldPosition;
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void setup()
{
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    lvgl_xHandler = xSemaphoreCreateMutex();
    ui_init_fun();
    xTaskCreatePinnedToCore(ui_Task, "ui Task", 1024 * 4, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(homeassistant_Task, "homeassistant Task", 1024 * 4, NULL, 1, NULL, 1);
}

char light_str[32];
int light_flage = 0;
int old_light_flage = 0;
void loop()
{
    M5Dial.update();
    long newPosition = M5Dial.Encoder.read();
    if (newPosition != oldPosition)
    {
        M5Dial.Speaker.tone(8000, 20);
        if (newPosition > 255)
        {
            M5Dial.Encoder.write(255);
            newPosition = 255;
        }
        if (newPosition < 0)
        {
            M5Dial.Encoder.write(0);
            newPosition = 0;
        }

        if (newPosition == 0)
        {
            if (light_flage == 1)
            {
                lv_obj_set_style_bg_color(ui_Arc2, lv_color_hex(0xEB1111), LV_PART_MAIN | LV_STATE_DEFAULT);
                light_flage = 0;
            }
        }
        else
        {
            if (light_flage == 0)
            {
                lv_obj_set_style_bg_color(ui_Arc2, lv_color_hex(0x29F61A), LV_PART_MAIN | LV_STATE_DEFAULT);
                light_flage = 1;
            }
        }

        sprintf(light_str, "%d %%", newPosition * 100 / 255);
        xSemaphoreTake(lvgl_xHandler, portMAX_DELAY);
        lv_arc_set_value(ui_Arc2, newPosition);
        lv_label_set_text(ui_Label1, light_str);
        xSemaphoreGive(lvgl_xHandler);

        oldPosition = newPosition;
    }
    if (M5Dial.BtnA.wasPressed())
    {
        if (M5Dial.Encoder.read() != 0)
        {
            oldPosition1 = M5Dial.Encoder.read();
            M5Dial.Encoder.readAndReset();
        }
        else
        {
            M5Dial.Encoder.write(oldPosition1);
        }
    }
    if (M5Dial.BtnA.pressedFor(3000))
    {
        M5Dial.Encoder.write(255);
    }
}
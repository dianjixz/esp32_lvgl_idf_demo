// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <inttypes.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "driver/spi_master.h"
// #include "driver/gpio.h"
// #include "freertos/semphr.h"
// #include "esp_task_wdt.h"
// #include <nvs_flash.h>
// #include <esp_event.h>
// #include <esp_wifi.h>
// #include <esp_log.h>
// #include <esp_netif.h>
// #include <esp_netif_ip_addr.h>


// SemaphoreHandle_t xHandler;

// void taskA(void *ptParam)
// {
// 	while (1)
// 	{
// 		vTaskDelay(pdMS_TO_TICKS(5));
// 		xSemaphoreTake(xHandler, portMAX_DELAY);
// 		xSemaphoreGive(xHandler);
// 	}
// }

// void taskB(void *ptParam)
// {
// 	int mkasd = 0;
// 	while (1)
// 	{
// 		vTaskDelay(pdMS_TO_TICKS(100));
// 		// lv_timer_handler();
// 		xSemaphoreTake(xHandler, portMAX_DELAY);
// 		xSemaphoreGive(xHandler);
// 		mkasd += 5;
// 		if (mkasd >= 100)
// 		{
// 			mkasd = 0;
// 		}
// 		printf("ui_Arc2 this value:%d\n", mkasd);
// 	}
// }

// void app_main(void)
// {
// 	xHandler = xSemaphoreCreateMutex();
// 	xTaskCreatePinnedToCore(taskA, "Task A", 1024 * 4, NULL, 1, NULL, 1); // 最后一个参数为核心0
// 	xTaskCreatePinnedToCore(taskB, "Task B", 1024 * 4, NULL, 1, NULL, 1); // 最后一个参数为核心0
// 	esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));
// }






#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

static char *TAG = "wifi station";

SemaphoreHandle_t sem;

// 状态机事件处理：static void event_handler(void * arg, esp_event_base_t event_base,
//                                           int32_t event_id, void * event_data)
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START ||
                                       event_id == WIFI_EVENT_STA_DISCONNECTED))
    {
        ESP_LOGI(TAG, "begin to connect the AP");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xSemaphoreGive(sem);
    }
}

void app_main(void)
{
    sem = xSemaphoreCreateBinary();

    // nvs初始化：nvs_flash_init()
    ESP_ERROR_CHECK(nvs_flash_init());

    // 事件循环初始化:sp_event_loop_create_default();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 事件处理函数注册
    //  esp_err_t esp_event_handler_register(esp_event_base_t event_base, int32_t event_id,
    //                                       esp_event_handler_t event_handler, void * event_handler_arg)
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL));

    // 初始化阶段：esp_netif_init()；sp_netif_create_default_wifi_sta();esp_wifi_init()；
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    // 配置阶段:esp_wifi_set_mode();esp_wifi_set_config();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t cfg = {
        .sta = {
            //用户名和密码根据实际情况修改
            .ssid = "10",
            .password = "1234567890",
        }};
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));

    // 启动阶段:esp_wifi_start()
    ESP_ERROR_CHECK(esp_wifi_start());

    while (1)
    {
        if (xSemaphoreTake(sem, portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI(TAG, "connected to ap!");
        }
    }
}


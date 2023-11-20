#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "ST_7789_drv.h"
#include "esp_task_wdt.h"
#include "lvgl.h"
#include "ui/ui.h"

#define MY_DISP_HOR_RES (240) // w
#define MY_DISP_VER_RES (320) // h
#define BUFF_LEN 5

static lv_disp_draw_buf_t draw_buf_dsc_1;
static lv_color_t buf_1[MY_DISP_HOR_RES * BUFF_LEN];
static lv_disp_drv_t disp_drv;
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);
	// M5.Lcd.drawBitmap(area->x1, area->y1, w, h, &color_p->full);
	lcd_show_image(area->x1, area->y1, w, h, &color_p->full);

	lv_disp_flush_ready(disp_drv);
}

SemaphoreHandle_t xHandler;

void taskA(void *ptParam)
{
	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(5));
		xSemaphoreTake(xHandler, portMAX_DELAY);
		lv_timer_handler();
		xSemaphoreGive(xHandler);
	}
}

void taskB(void *ptParam)
{
	int mkasd = 0;
	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(100));
		// lv_timer_handler();

		xSemaphoreTake(xHandler, portMAX_DELAY);
		lv_arc_set_value(ui_Arc2, mkasd);
		char nihaoasd[10];
		sprintf(nihaoasd, "%d", mkasd);
		lv_label_set_text(ui_Label2, nihaoasd);
		xSemaphoreGive(xHandler);
		mkasd += 5;
		if (mkasd >= 100)
		{
			mkasd = 0;
		}
		printf("ui_Arc2 this value:%d\n", mkasd);
	}
}

void app_main(void)
{
	lcd_init();
	lcd_clear();

	lv_init();
	lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * BUFF_LEN);
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = MY_DISP_HOR_RES;
	disp_drv.ver_res = MY_DISP_VER_RES;
	disp_drv.flush_cb = disp_flush;
	disp_drv.draw_buf = &draw_buf_dsc_1;
	lv_disp_drv_register(&disp_drv);
	ui_init();
	xHandler = xSemaphoreCreateMutex();
	xTaskCreatePinnedToCore(taskA, "Task A", 1024 * 4, NULL, 1, NULL, 1); // 最后一个参数为核心0
	xTaskCreatePinnedToCore(taskB, "Task B", 1024 * 4, NULL, 1, NULL, 1); // 最后一个参数为核心0

	esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));
}

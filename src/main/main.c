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
#include "esp_task_wdt.h"
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

#include <lwip/sockets.h>


#include "driver/i2s.h"
#include "esp_system.h"

#include"output_audio_file.h"

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

static int sock;

void taskA(void *ptParam)
{
	xSemaphoreTake(sem, portMAX_DELAY);
	ESP_LOGI(TAG, "connected to ap!");
	while (1)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);
		// vTaskDelay(pdMS_TO_TICKS(5));
		if (sock < 0)
		{
			ESP_LOGE(TAG, "create socket failed!");
			return;
		}
		ESP_LOGI(TAG, "create socket successfully!");

		// 初始化server的地址结构体sockaddr_in
		struct sockaddr_in destaddr = {};
		destaddr.sin_family = AF_INET;
		destaddr.sin_port = htons(8080);					  // 填写网络调试助手服务端实际端口
		destaddr.sin_addr.s_addr = inet_addr("192.168.2.21"); // 填写网络调试助手服务端实际IP地址

		// 建立socket连接：
		socklen_t len = sizeof(struct sockaddr);
		if (connect(sock, (struct sockaddr *)&destaddr, len) < 0)
		{
			ESP_LOGE(TAG, "connect to server failed!");
			close(sock);
			return;
		}
		ESP_LOGI(TAG, "connect to server successfully!");
		int count = 0;
		while (1)
		{
			char buff[512] = "hello, I am tcp_client!";
			sprintf(buff, "hello, I am tcp_client! %d\n", count++);
			send(sock, buff, strlen(buff), 0);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
}
#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAK_I2S_NUMBER I2S_NUM_0

void play_i2s_init(void)
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LOWMED,
        .dma_buf_count = 2,
        .dma_buf_len = 300,
        .bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT};
    i2s_pin_config_t pin_config = {
        .mck_io_num = -1,
        .bck_io_num = CONFIG_I2S_BCK_PIN,   // IIS_SCLK
        .ws_io_num = CONFIG_I2S_LRCK_PIN,    // IIS_LCLK
        .data_out_num = CONFIG_I2S_DATA_PIN, // IIS_DSIN
        .data_in_num = -1         // IIS_DOUT
    };
    i2s_driver_install(SPEAK_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_set_pin(SPEAK_I2S_NUMBER, &pin_config);
    i2s_zero_dma_buffer(SPEAK_I2S_NUMBER);
}


typedef struct wav_header {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    uint32_t wav_size;   // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"

    // Format Header
    char fmt_header[4];  // Contains "fmt " (includes trailing space)
    uint32_t fmt_chunk_size; // Should be 16 for PCM
    uint16_t audio_format;   // Should be 1 for PCM. 3 for IEEE Float
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;  // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    uint16_t sample_alignment; // num_channels * Bytes Per Sample
    uint16_t bit_depth;  // Number of bits per sample

    // Data
    char data_header[4]; // Contains "data"
    uint32_t data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    uint8_t bytes[];    // Remaining bytes are audio data
} wav_header;

void wav_play_main(void *pvParameters)
{
	// i2s_config_t i2s_config = {
	// 	.mode = I2S_MODE_MASTER | I2S_MODE_TX, // 作为主设备发送
	// 	.sample_rate = 44100, // 采样率
	// 	.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // 每个样本的位数
	// 	.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 立体声
	// 	.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
	// 	.intr_alloc_flags = 0, // 默认中断标志
	// 	.dma_buf_count = 8, // DMA 缓冲区数量
	// 	.dma_buf_len = 64,  // DMA 缓冲区长度
	// };
	// i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

	// i2s_pin_config_t pin_config = {
	// 	.bck_io_num = 33, // 时钟引脚
	// 	.ws_io_num = 19, // 字选择引脚
	// 	.data_out_num = 22, // 数据输出引脚
	// 	.data_in_num = I2S_PIN_NO_CHANGE // 不使用数据输入
	// };
	// i2s_set_pin(I2S_NUM_0, &pin_config);


	// i2s_driver_uninstall(SPEAK_I2S_NUMBER);	
    // i2s_config_t i2s_config = {
    //     .mode = (i2s_mode_t)(I2S_MODE_MASTER),
    //     .sample_rate = 16000,
    //     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    //     .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
    //     .communication_format = I2S_COMM_FORMAT_I2S,
    //     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    //     .dma_buf_count = 6,
    //     .dma_buf_len = 60,
    // };
	// i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
	// i2s_config.use_apll = false;
	// i2s_config.tx_desc_auto_clear = true;
	// i2s_driver_install(SPEAK_I2S_NUMBER, &i2s_config, 0, NULL);

	// i2s_pin_config_t tx_pin_config;
    // tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    // tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    // tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    // tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
	// i2s_set_pin(SPEAK_I2S_NUMBER, &tx_pin_config);
	// i2s_zero_dma_buffer(SPEAK_I2S_NUMBER);
	// i2s_set_clk(SPEAK_I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
	// size_t bytesRead = fread(&header, 1, sizeof(header), wavFile);
	play_i2s_init();
    wav_header *header = (wav_header *)output_audio_file_wav;
    // size_t bytesRead = output_audio_file_wav;
    // if (bytesRead > 0) {
	printf("RIFF header: %c%c%c%c\n", header->riff_header[0], header->riff_header[1], header->riff_header[2], header->riff_header[3]);
	printf("WAV size: %lu\n", header->wav_size);
	printf("WAVE header: %c%c%c%c\n", header->wave_header[0], header->wave_header[1], header->wave_header[2], header->wave_header[3]);
	printf("fmt header: %c%c%c%c\n", header->fmt_header[0], header->fmt_header[1], header->fmt_header[2], header->fmt_header[3]);
	printf("Format chunk size: %lu\n", header->fmt_chunk_size);
	printf("Audio format: %u\n", header->audio_format);
	printf("Number of channels: %u\n", header->num_channels);
	printf("Sample rate: %lu\n", header->sample_rate);
	printf("Byte rate: %lu\n", header->byte_rate);
	printf("Sample alignment: %u\n", header->sample_alignment);
	printf("Bit depth: %u\n", header->bit_depth);
	printf("Data header: %c%c%c%c\n", header->data_header[0], header->data_header[1], header->data_header[2], header->data_header[3]);
	printf("Data bytes: %lu\n", header->data_bytes);
	// header->data_bytes = 1024;



	int header_data_bytes = 256;
	i2s_set_clk(SPEAK_I2S_NUMBER, header->sample_rate, header->bit_depth, I2S_CHANNEL_MONO);
	uint8_t *audio_data = header->bytes;
	while (1)
	{
		size_t bytes_written;
		i2s_write(SPEAK_I2S_NUMBER, audio_data, header_data_bytes, &bytes_written, portMAX_DELAY);
		audio_data += header_data_bytes;
		if(audio_data - output_audio_file_wav > output_audio_file_wav_len)
		{
			audio_data = header->bytes;
			// vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
	




	// i2s_zero_dma_buffer(SPEAK_I2S_NUMBER);
	// i2s_stop(SPEAK_I2S_NUMBER);


    
}




















void app_main(void)
{
	wav_play_main(NULL);
	// sem = xSemaphoreCreateBinary();

	// // nvs初始化：nvs_flash_init()
	// ESP_ERROR_CHECK(nvs_flash_init());

	// // 事件循环初始化:sp_event_loop_create_default();
	// ESP_ERROR_CHECK(esp_event_loop_create_default());

	// // 事件处理函数注册
	// //  esp_err_t esp_event_handler_register(esp_event_base_t event_base, int32_t event_id,
	// //                                       esp_event_handler_t event_handler, void * event_handler_arg)
	// ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL));
	// ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL));

	// // 初始化阶段：esp_netif_init()；sp_netif_create_default_wifi_sta();esp_wifi_init()；
	// ESP_ERROR_CHECK(esp_netif_init());
	// esp_netif_create_default_wifi_sta();
	// wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
	// ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

	// // 配置阶段:esp_wifi_set_mode();esp_wifi_set_config();
	// ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	// wifi_config_t cfg = {
	// 	.sta = {
	// 		// 用户名和密码根据实际情况修改
	// 		.ssid = "M5-R&D",
	// 		.password = "echo\"password\">/dev/null",
	// 	}};
	// ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));

	// // 启动阶段:esp_wifi_start()
	// ESP_ERROR_CHECK(esp_wifi_start());
	// xTaskCreatePinnedToCore(taskA, "Task A", 1024 * 4, NULL, 1, NULL, 1);
}

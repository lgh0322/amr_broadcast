/* Record amr file to SD Card

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_log.h"
#include "audio_element.h"
#include "audio_common.h"
#include "audio_hal.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "amrwb_encoder.h"
#include "amrnb_encoder.h"
#include "board.h"
#include "udp_stream.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

static const char *TAG = "REC_AMR_SDCARD";
#define RECORD_TIME_SECONDS (10)
#define RING_BUFFER_SIZE    (8192)





esp_err_t audio_element_event_handler(audio_element_handle_t self, audio_event_iface_msg_t *event, void *ctx)
{
    ESP_LOGI(TAG, "Audio event %d from %s element", event->cmd, audio_element_get_tag(self));
    if (event->cmd == AEL_MSG_CMD_REPORT_STATUS) {
        switch ((int) event->data) {
            case AEL_STATUS_STATE_RUNNING:
                ESP_LOGI(TAG, "AEL_STATUS_STATE_RUNNING");
                break;
            case AEL_STATUS_STATE_STOPPED:
                ESP_LOGI(TAG, "AEL_STATUS_STATE_STOPPED");
                break;
            case AEL_STATUS_STATE_FINISHED:
                ESP_LOGI(TAG, "AEL_STATUS_STATE_FINISHED");
                break;
            default:
                ESP_LOGI(TAG, "Some other event = %d", (int) event->data);
        }
    }
    return ESP_OK;
}






#define WIFI_SSID				"vaca"			
#define WIFI_PAS				"22345678"			
#define UDP_PORT				8888				
#define UDP_ADRESS				"192.168.6.110"	
													
                                                    
#define LED_GPIO				4					

EventGroupHandle_t udp_event_group;                 

#define WIFI_CONNECTED_BIT BIT0
struct sockaddr_in client_addr;						
static unsigned int socklen = sizeof(client_addr);	
int connect_socket=222;								




void wifi_init_sta();// WIFI作为STA的初始化
void close_socket();// 关闭socket
void recv_data(void *pvParameters);// 接收数据任务
esp_err_t create_udp_client();// 建立udp client
static void udp_connect(void *pvParameters);// 建立UDP连接并从UDP接收数据

int show_socket_error_reason(const char *str, int socket);// 获取socket错???原因
int get_socket_error_code(int socket);// 获取socket错???代码



// wifi 事件
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:        //STA模式
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED: //STA模式-????
		esp_wifi_connect();
		xEventGroupClearBits(udp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_CONNECTED:    //STA模式-连接成功
		xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:       //STA模式-获取IP
		ESP_LOGI(TAG, "got ip:%s\n",ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}


// WIFI作为STA的初始化
void wifi_init_sta()
{
	udp_event_group = xEventGroupCreate();
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PAS},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s password:%s \n",WIFI_SSID, WIFI_PAS);
}

// 关闭socket
void close_socket()
{
    close(connect_socket);
}
// 接收数据任务
void recv_data(void *pvParameters)
{
	int len = 0;            //长度
	char databuff[1024];    //缓存
	while (1){
		memset(databuff, 0x00, sizeof(databuff));//清空缓存
		//读取接收数据
		len = recvfrom(connect_socket, databuff, sizeof(databuff), 0,(struct sockaddr *) &client_addr, &socklen);
		if (len > 0){
			//打印接收到的数组
			ESP_LOGI(TAG, "UDP Client recvData: %s", databuff);
			//接收数据回发
		//	sendto(connect_socket, databuff, strlen(databuff), 0,(struct sockaddr *) &client_addr, sizeof(client_addr));
		}else{
			//打印错???信息
			show_socket_error_reason("UDP Client recv_data", connect_socket);
			break;
		}
	}
	close_socket();
	vTaskDelete(NULL);
}
// 建立udp client
esp_err_t create_udp_client()
{
	ESP_LOGI(TAG, "will connect gateway ssid : %s port:%d",UDP_ADRESS, UDP_PORT);

    vTaskDelay(100);

	//新建socket
	connect_socket = socket(AF_INET, SOCK_DGRAM, 0);                         /*参数和TCP不同*/
	if (connect_socket < 0){
		//打印报错信息
		show_socket_error_reason("create client", connect_socket);
		//新建失败后，关闭新建的socket，等待下次新??
		close(connect_socket);
		return ESP_FAIL;
	}
	vTaskDelay(100);
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(UDP_PORT);
	client_addr.sin_addr.s_addr = inet_addr(UDP_ADRESS);

    struct sockaddr_in Loacl_addr; 
    Loacl_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Loacl_addr.sin_family = AF_INET;
    Loacl_addr.sin_port = htons(UDP_PORT); 
    uint8_t res = 0;
    res = bind(connect_socket,(struct sockaddr *)&Loacl_addr,sizeof(Loacl_addr));
    if(res != 0){
        printf("bind error\n");
 
    }
vTaskDelay(100);


	int len = 0;            //长度
	char databuff[102] = "Hello Server,Please ack!!";    //缓存
	//测试udp server

// for(int k=0;k<10;k++){
// 	sendto(connect_socket, databuff, 1024, 0, (struct sockaddr *) &client_addr,sizeof(client_addr));
//     vTaskDelay(50);
// }
		

	
	return ESP_OK;
}


static void udp_connect(void *pvParameters)
{
	
	xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(TAG, "start udp connected");
	vTaskDelay(3000 / portTICK_RATE_MS);
	ESP_LOGI(TAG, "create udp Client");
	int socket_ret = create_udp_client();




    




















    while(1){
        vTaskDelay(100);
    }



	// if (socket_ret == ESP_FAIL){
	// 	ESP_LOGI(TAG, "create udp socket error,stop...");
	// 	vTaskDelete(NULL);
	// }else{
	// 	ESP_LOGI(TAG, "create udp socket succeed...");            
	// 	//建立UDP接收数据任务
	// 	// if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, NULL)){
	// 	// 	ESP_LOGI(TAG, "Recv task create fail!");
	// 	// 	vTaskDelete(NULL);
	// 	// }else{
	// 	// 	ESP_LOGI(TAG, "Recv task create succeed!");
	// 	// }
	// }
	 vTaskDelete(NULL);
}
// 获取socket错???原因
int show_socket_error_reason(const char *str, int socket)
{
	int err = get_socket_error_code(socket);
	if (err != 0){
		ESP_LOGW(TAG, "%s socket error reason %d %s", str, err, strerror(err));
	}
	return err;
}

int get_socket_error_code(int socket)
{
	int result;
	u32_t optlen = sizeof(int);
	int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);
	if (err == -1){
		//WSAGetLastError();
		ESP_LOGE(TAG, "socket error code:%d", err);
		ESP_LOGE(TAG, "socket error code:%s", strerror(err));
		return -1;
	}
	return result;
}
void app_main(void)
{
    ESP_LOGI(TAG, "APP Start......");






audio_element_handle_t i2s_stream_reader, amr_encoder, fatfs_stream_writer;
    ringbuf_handle_t ringbuf1, ringbuf2;

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);



    ESP_LOGI(TAG, "[2.0] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[3.0] Create fatfs stream to write data to sdcard");
    udp_stream_cfg_t fatfs_cfg = UDP_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_WRITER;
    fatfs_cfg.host=""; 

    fatfs_stream_writer = udp_stream_init(&fatfs_cfg);

    ESP_LOGI(TAG, "[3.1] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
#ifdef CONFIG_CHOICE_AMR_WB
    i2s_cfg.i2s_config.sample_rate = 16000;
#elif defined CONFIG_CHOICE_AMR_NB
    i2s_cfg.i2s_config.sample_rate = 8000;
#endif
    i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_cfg.type = AUDIO_STREAM_READER;
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    i2s_cfg.i2s_port = 1;
#endif
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[3.2] Create amr encoder to encode amr format");
#ifdef CONFIG_CHOICE_AMR_WB
    amrwb_encoder_cfg_t amr_enc_cfg = DEFAULT_AMRWB_ENCODER_CONFIG();
    amr_encoder = amrwb_encoder_init(&amr_enc_cfg);
#elif defined CONFIG_CHOICE_AMR_NB
    amrnb_encoder_cfg_t amr_enc_cfg = DEFAULT_AMRNB_ENCODER_CONFIG();
    amr_encoder = amrnb_encoder_init(&amr_enc_cfg);
#endif
    ESP_LOGI(TAG, "[3.3] Create a ringbuffer and insert it between i2s reader and amr encoder");
    ringbuf1 = rb_create(RING_BUFFER_SIZE, 1);
    audio_element_set_output_ringbuf(i2s_stream_reader, ringbuf1);
    audio_element_set_input_ringbuf(amr_encoder, ringbuf1);

    ESP_LOGI(TAG, "[3.4] Create a ringbuffer and insert it between amr encoder and fatfs_stream_writer");
    ringbuf2 = rb_create(RING_BUFFER_SIZE, 1);
    audio_element_set_output_ringbuf(amr_encoder, ringbuf2);
    audio_element_set_input_ringbuf(fatfs_stream_writer, ringbuf2);

    ESP_LOGI(TAG, "[3.5] Set up  uri (file as fatfs_stream, amr as amr encoder)");
#ifdef CONFIG_CHOICE_AMR_WB
    audio_element_set_uri(fatfs_stream_writer, "/sdcard/rec.Wamr");
#elif defined CONFIG_CHOICE_AMR_NB
   // audio_element_set_uri(fatfs_stream_writer, "/sdcard/rec.amr");
#endif

    ESP_LOGI(TAG, "[4.0] Set callback function for audio_elements");
    /**
     * Event handler used here is quite generic and simple one.
     * It just reports state changes of different audio_elements
     * Note that, it is not mandatory to set event callbacks.
     * One may remove entire step [4.0]. Example will still run fine.
     */
    audio_element_set_event_callback(amr_encoder, audio_element_event_handler, NULL);
    audio_element_set_event_callback(i2s_stream_reader, audio_element_event_handler, NULL);
    audio_element_set_event_callback(fatfs_stream_writer, audio_element_event_handler, NULL);

    ESP_LOGI(TAG, "[5.0] Start audio elements");
    audio_element_run(i2s_stream_reader);
    audio_element_run(amr_encoder);
    audio_element_run(fatfs_stream_writer);

    audio_element_resume(i2s_stream_reader, 0, 0);
    audio_element_resume(amr_encoder, 0, 0);
    audio_element_resume(fatfs_stream_writer, 0, 0);
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	wifi_init_sta();// WIFI作为STA的初始化
	xTaskCreate(&udp_connect, "udp_connect", 4096, NULL, 1, NULL);
}





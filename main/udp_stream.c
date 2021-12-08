/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
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

#include <stdio.h>
#include <string.h>

#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_mem.h"
#include "udp_stream.h"

static const char *TAG = "UDP_STREAM";
#define CONNECT_TIMEOUT_MS        100
extern struct sockaddr_in client_addr;						
extern int connect_socket;	

typedef struct udp_stream {
    esp_transport_handle_t        t;
    audio_stream_type_t           type;
    int                           sock;
    int                           port;
    char                          *host;
    bool                          is_open;
    int                           timeout_ms;
    udp_stream_event_handle_cb    hook;
    void                          *ctx;
} udp_stream_t;

static int _get_socket_error_code_reason(const char *str, int sockfd)
{
    uint32_t optlen = sizeof(int);
    int result;
    int err;

    ESP_LOGE(TAG, "fuck66");
    
    return result;
}

static esp_err_t _dispatch_event(audio_element_handle_t el, udp_stream_t *udp, void *data, int len, udp_stream_status_t state)
{
    if (el && udp && udp->hook) {
        udp_stream_event_msg_t msg = { 0 };
        msg.data = data;
        msg.data_len = len;
        msg.sock_fd = udp->t;
        msg.source = el;
        return udp->hook(&msg, state, udp->ctx);
    }
    return ESP_FAIL;
}

static esp_err_t _udp_open(audio_element_handle_t self)
{
    AUDIO_NULL_CHECK(TAG, self, return ESP_FAIL);

    udp_stream_t *udp = (udp_stream_t *)audio_element_getdata(self);
    if (udp->is_open) {
        ESP_LOGE(TAG, "Already opened");
        return ESP_FAIL;
    }

    udp->is_open = true;
  
     ESP_LOGE(TAG, "fuck55");
    return ESP_OK;
}

static esp_err_t _udp_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    ESP_LOGE(TAG, "fuck44");
 
    return 0;
}

static esp_err_t _udp_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
     if(connect_socket==222){
          ESP_LOGE(TAG, "aafuck888");
       return 100;
   }
    sendto(connect_socket, buffer, len, 0, (struct sockaddr *) &client_addr,sizeof(client_addr));
    return len;
}

static esp_err_t _udp_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
     ESP_LOGE(TAG, "fuck4488       %d",in_len);
    int r_size = audio_element_input(self, in_buffer, in_len);
        ESP_LOGE(TAG, "fuck44889       %d",r_size);
    int w_size = 0;
    if (r_size > 0) {
        w_size = audio_element_output(self, in_buffer, r_size);
          ESP_LOGE(TAG, "fuck448892       %d",w_size);
        if (w_size > 0) {
            audio_element_update_byte_pos(self, r_size);
        }
    } else {
        w_size = r_size;
    }
       ESP_LOGE(TAG, "fuck44       %d",w_size);
    return w_size;
}

static esp_err_t _udp_close(audio_element_handle_t self)
{
   ESP_LOGE(TAG, "fuck33");
    return ESP_OK;
}

static esp_err_t _udp_destroy(audio_element_handle_t self)
{
   ESP_LOGE(TAG, "fuck22");
    return ESP_OK;
}

audio_element_handle_t udp_stream_init(udp_stream_cfg_t *config)
{
    AUDIO_NULL_CHECK(TAG, config, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    audio_element_handle_t el;
    cfg.open = _udp_open;
    cfg.close = _udp_close;
    cfg.process = _udp_process;
    cfg.destroy = _udp_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.stack_in_ext = config->ext_stack;
    cfg.tag = "udp_client";
    if (cfg.buffer_len == 0) {
        cfg.buffer_len = UDP_STREAM_BUF_SIZE;
    }

    udp_stream_t *udp = audio_calloc(1, sizeof(udp_stream_t));
    AUDIO_MEM_CHECK(TAG, udp, return NULL);

    udp->type = config->type;
    udp->port = config->port;
    udp->host = config->host;
    udp->timeout_ms = config->timeout_ms;
    if (config->event_handler) {
        udp->hook = config->event_handler;
        if (config->event_ctx) {
            udp->ctx = config->event_ctx;
        }
    }

    if (config->type == AUDIO_STREAM_WRITER) {
           ESP_LOGE(TAG, "fuck222");
        cfg.write = _udp_write;
    } else {
        cfg.read = _udp_read;
    }

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _udp_init_exit);
    audio_element_setdata(el, udp);

    return el;
_udp_init_exit:
    audio_free(udp);
    return NULL;
}

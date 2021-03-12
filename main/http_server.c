/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include <esp_http_server.h>

#include "cmd.h"

static const char *TAG = "HTTP";

extern QueueHandle_t xQueueHttp;

extern char *MOUNT_POINT;

static fpos_t SPIFFS_FileSize(char *filePath) {
	fpos_t fsize = 0;

	FILE *fp = fopen(filePath,"rb");
	ESP_LOGI(__FUNCTION__,"filePath=[%s] fp=%p", filePath, fp);
	if (fp == NULL) return 0;
	fseek(fp,0,SEEK_END);
	fgetpos(fp,&fsize);
	fclose(fp);
	ESP_LOGI(__FUNCTION__,"filePath=[%s] fsize=%ld", filePath, fsize);
	return fsize;
}

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;

	/* Get header value string length and allocate memory for length + 1,
	 * extra byte for null termination */
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		/* Copy null terminated value string into buffer */
		if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
			ESP_LOGI(TAG, "Found header => Host: %s", buf);
		}
		free(buf);
	}

	buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
			ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
		}
		free(buf);
	}

	buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
			ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
		}
		free(buf);
	}

	/* Read URL query string length and allocate memory for length + 1,
	 * extra byte for null termination */
	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(TAG, "Found URL query => %s", buf);
			char param[32];
			/* Get value of expected key from query string */
			if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
			}
			if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
			}
			if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
			}
		}
		free(buf);
	}

	/* Set some custom headers */
	httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
	httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

	const char* user_ctx = (const char*) req->user_ctx;
	ESP_LOGI(TAG, "user_ctx=[%s]", user_ctx);

	// Open file for reading
	ESP_LOGI(TAG, "Reading stdout");
	char fileName[128];
	char line[64];
	sprintf(fileName, "%s/%s", MOUNT_POINT, user_ctx);
	ESP_LOGI(TAG, "fileName=%s", fileName);
	fpos_t fsize = SPIFFS_FileSize(fileName);
	char *buffer = malloc(fsize+1);
	if (buffer == NULL) {
		ESP_LOGE(TAG, "Failed to malloc");
	} else {
		memset(buffer, 0, fsize+1);
		FILE* f = fopen(fileName, "r");
		if (f == NULL) {
			ESP_LOGW(TAG, "Failed to open file for reading");
		} else {
			while (fgets(line, sizeof(line), f) != NULL) {
				size_t linesz = strlen(line);
				//remove EOL (CR or LF)
				for (int i=linesz;i>0;i--) {
					if (line[i-1] == 0x0a) {
						line[i-1] = 0;
					} else if (line[i-1] == 0x0d) {
						line[i-1] = 0;
					} else {
						break;
					}
				}
				ESP_LOGD(TAG, "line=[%s]", line);
				strcat(buffer, line);
			}
		}
		fclose(f);
	}

	httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);
	free(buffer);

#if 0
	/* Send response with custom headers and body set as the
	 * string passed in user context*/
	const char* resp_str = (const char*) req->user_ctx;
	ESP_LOGI(TAG, "resp_str=[%s] HTTPD_RESP_USE_STRLEN=%d", resp_str, HTTPD_RESP_USE_STRLEN);
	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
#endif

	/* After sending the HTTP response the old HTTP request
	 * headers are lost. Check if HTTP request headers can be read now. */
	if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
		ESP_LOGI(TAG, "Request headers lost");
	}
	return ESP_OK;
}

static const httpd_uri_t root = {
	.uri	   = "/",
	.method    = HTTP_GET,
	.handler   = root_get_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	//.user_ctx  = "Hello World!"
	.user_ctx  = "stdout.txt"
};


static httpd_handle_t start_webserver(int port)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.lru_purge_enable = true;
	//config.server_port = 8080;
	config.server_port = port;

	// Start the httpd server
	//ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) == ESP_OK) {
		// Set URI handlers
		ESP_LOGI(TAG, "Registering URI handlers");
		httpd_register_uri_handler(server, &root);
		return server;
	}

	ESP_LOGI(TAG, "Error starting server!");
	return NULL;
}



void http_task(void *pvParameters)
{
	char *task_parameter = (char *)pvParameters;
	ESP_LOGI(pcTaskGetName(NULL), "Start task_parameter=%s", task_parameter);
	char url[64];
	int port = 8080;
	sprintf(url, "http://%s:%d", task_parameter, port);
	ESP_LOGI(TAG, "Starting server on %s", url);
	//static httpd_handle_t server = NULL;
	//server = start_webserver(port);
	start_webserver(port);

	HTTP_t httpBuf;
	while(1) {
		//Waiting for HTTP event.
		if (xQueueReceive(xQueueHttp, &httpBuf, portMAX_DELAY) == pdTRUE) {
			ESP_LOGI(TAG, "httpBuf.command=%d", httpBuf.command);
			ESP_LOGI(TAG, "You can check stdout on %s", url);
		}
	}

	ESP_LOGI(TAG, "finish");
	vTaskDelete(NULL);
}

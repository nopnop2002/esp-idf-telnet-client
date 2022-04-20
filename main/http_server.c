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

static esp_err_t file2html(httpd_req_t *req, char * filename) {
	ESP_LOGI(TAG, "Reading %s", filename);
	FILE* fhtml = fopen(filename, "r");
	if (fhtml == NULL) {
		return ESP_FAIL;
	} else {
		char line[128];
		while (fgets(line, sizeof(line), fhtml) != NULL) {
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
			httpd_resp_sendstr_chunk(req, line);
		}
		fclose(fhtml);
	}
	return ESP_OK;
}

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	// Get HTTPD global user context
	const char* user_ctx = (const char*) req->user_ctx;
	ESP_LOGI(TAG, "user_ctx=[%s]", user_ctx);

	// Send html
	char fileName[128];
	sprintf(fileName, "%s/%s", MOUNT_POINT, user_ctx);
	ESP_LOGI(TAG, "fileName=%s", fileName);
	file2html(req, fileName);

	/* Send empty chunk to signal HTTP response completion */
	httpd_resp_sendstr_chunk(req, NULL);

	return ESP_OK;
}

/* favicon get handler */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "favicon_get_handler req->uri=[%s]", req->uri);
	return ESP_OK;
}

static esp_err_t start_server(int port)
{
	// Generate default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Empty handle to http_server
	httpd_handle_t server = NULL;

	// Purge “Least Recently Used” connection
	config.lru_purge_enable = true;

	// TCP Port number for receiving and transmitting HTTP traffic
	//config.server_port = 8080;
	config.server_port = port;

	// Start the httpd server
	//ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start file server!");
		return ESP_FAIL;
	}

	/* URI handler for get */
	httpd_uri_t root = {
		.uri		 = "/",
		.method		 = HTTP_GET,
		.handler	 = root_get_handler,
		.user_ctx  = "stdout.txt"
	};
	httpd_register_uri_handler(server, &root);

	/* URI handler for favicon.ico */
	httpd_uri_t _favicon_get_handler = {
		.uri		 = "/favicon.ico",
		.method		 = HTTP_GET,
		.handler	 = favicon_get_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};
	httpd_register_uri_handler(server, &_favicon_get_handler);

	return ESP_OK;
}

void http_task(void *pvParameters)
{
	char *task_parameter = (char *)pvParameters;
	ESP_LOGI(TAG, "Start task_parameter=%s", task_parameter);
	char url[64];
	int port = 8080;
	sprintf(url, "http://%s:%d", task_parameter, port);
	ESP_LOGI(TAG, "Starting server on %s", url);
	//server = start_server(port);
	ESP_ERROR_CHECK(start_server(port));

	HTTP_t httpBuf;
	while(1) {
		//Waiting for HTTP event.
		if (xQueueReceive(xQueueHttp, &httpBuf, portMAX_DELAY) == pdTRUE) {
			ESP_LOGD(TAG, "httpBuf.command=%d", httpBuf.command);
			ESP_LOGI(TAG, "You can check stdout on %s", url);
		}
	}

	// Never reach here
	ESP_LOGI(TAG, "finish");
	vTaskDelete(NULL);
}

/* This is an alternative process to STDIN

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/param.h>
#include <sys/select.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "cmd.h"

static const char *TAG = "STDIN";

extern QueueHandle_t xQueueSocket;
extern QueueHandle_t xQueueStdin;

extern char *MOUNT_POINT;

#define TIMEOUT 500

long getLine(FILE* f, char * line, size_t lineSize) {
	line[0] = 0;
	char buffer[64];
	long wait = 0;
	while(1) {
		fgets(buffer, lineSize, f);
		if (feof(f)) return -1;
		ESP_LOGI(TAG, "buffer=[%s]", buffer);
		if (buffer[0] == '#') continue;
		if (buffer[0] == '>') {
			for(int i=1; i<strlen(buffer); i++) {
				if (buffer[i] == 0x0a) continue;
				if (buffer[i] == 0x0d) continue;
				line[i-1] = buffer[i];
				line[i] = 0;
			}
			ESP_LOGI(TAG, "line=[%s]",line);
			break;
		} else {
			return -1;
		}
	}

	char work[64];
	while(1) {
		fgets(buffer, lineSize, f);
		if (feof(f)) return -1;
		ESP_LOGI(TAG, "buffer=[%s]", buffer);
		if (buffer[0] == '#') continue;
		if (buffer[0] == '<') {
			for(int i=1; i<strlen(buffer); i++) {
				if (buffer[i] == 0x0a) continue;
				if (buffer[i] == 0x0d) continue;
				work[i-1] = buffer[i];
				work[i] = 0;
			}
			ESP_LOG_BUFFER_HEXDUMP(TAG, work, strlen(work), ESP_LOG_INFO);
			wait = strtol(work, NULL, 10);
			break;
		} else {
			return -1;
		}
	}
	ESP_LOGI(TAG, "wait=%ld", wait);
	return wait;
}

void stdin_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Start");
	esp_log_level_set(TAG, ESP_LOG_WARN);

	SOCKET_t socketBuf;
	STDIN_t stdinBuf;
	stdinBuf.taskHandle = xTaskGetCurrentTaskHandle();
	int stdinStatus = 0;
	//char LF[2] = {0x0a, 0x00};
	char CRLF[3] = {0x0d, 0x0a, 0x00};

	// Open file for reading
	ESP_LOGI(TAG, "Reading file");
	char fileName[64];
	sprintf(fileName, "%s/command.txt", MOUNT_POINT);
	ESP_LOGI(TAG, "fileName=%s", fileName);
	FILE* f = fopen(fileName, "r");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for reading");
		while(1) { vTaskDelay(1); }
	}


	while(1) {
		//Waiting for SOCKET receive event.
		if (xQueueReceive(xQueueSocket, &socketBuf, 0) == pdTRUE) {
			if (socketBuf.command == CMD_SOCKET) {
				ESP_LOGD(TAG, "data=%c", socketBuf.data);
			} else if (socketBuf.command == CMD_STDIN) {
				ESP_LOGI(TAG, "NODATA stdinStatus=%d", stdinStatus);
				if (stdinStatus == 0) {
					ESP_LOGI(TAG, "Send User %s", CONFIG_TELNET_USER);
					strcpy(stdinBuf.data, CONFIG_TELNET_USER);
					strcat(stdinBuf.data, CRLF);
					stdinBuf.length = strlen(stdinBuf.data);
					stdinBuf.wait = 2;
					xQueueSend(xQueueStdin, &stdinBuf, 0);
					stdinStatus++;
				} else if (stdinStatus == 1) {
					ESP_LOGI(TAG, "Send Pass %s", CONFIG_TELNET_PASSWORD);
					strcpy(stdinBuf.data, CONFIG_TELNET_PASSWORD);
					strcat(stdinBuf.data, CRLF);
					stdinBuf.length = strlen(stdinBuf.data);
					stdinBuf.wait = 2;
					xQueueSend(xQueueStdin, &stdinBuf, 0);
					stdinStatus++;
#if 0
				} else if (stdinStatus == 2) {
					ESP_LOGI(TAG, "Send Cmd");
					strcpy(stdinBuf.data, "ls");
					strcat(stdinBuf.data, CRLF);
					stdinBuf.length = strlen(stdinBuf.data);
					xQueueSend(xQueueStdin, &stdinBuf, 0);
					stdinStatus++;
#endif
				} else if (stdinStatus == 2) {
					char line[64];
					long wait = getLine(f, line, sizeof(line));
					ESP_LOGI(TAG, "wait=%ld", wait);
					if (wait < 0) {
						ESP_LOGI(TAG, "Send Exit");
						strcpy(stdinBuf.data, "exit");
						strcat(stdinBuf.data, CRLF);
						stdinBuf.length = strlen(stdinBuf.data);
						stdinBuf.wait = 2;
					} else {
						strcpy(stdinBuf.data, line);
						strcat(stdinBuf.data, CRLF);
						stdinBuf.length = strlen(stdinBuf.data);
						stdinBuf.wait = wait;
					}
					xQueueSend(xQueueStdin, &stdinBuf, 0);
				}

			}
		}
		vTaskDelay(1);
	}

}

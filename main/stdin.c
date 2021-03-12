/* This is an alternative process to STDIN

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "cmd.h"

static const char *TAG = "STDIN";

extern QueueHandle_t xQueueSocket;
extern QueueHandle_t xQueueStdin;
extern QueueHandle_t xQueueStdout;

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
	socketBuf.taskHandle = xTaskGetCurrentTaskHandle();
	STDIN_t stdinBuf;
	int stdinStatus = 0;
	//char LF[2] = {0x0a, 0x00};
	char CRLF[3] = {0x0d, 0x0a, 0x00};
	STDOUT_t stdoutBuf;
	stdoutBuf.taskHandle = xTaskGetCurrentTaskHandle();

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
		//Waiting for STDIN event.
		if (xQueueReceive(xQueueStdin, &stdinBuf, portMAX_DELAY) == pdTRUE) {
			if (stdinBuf.command == CMD_CLOSE) break;
			ESP_LOGI(TAG, "stdinStatus=%d", stdinStatus);
			if (stdinStatus == 0) {
				ESP_LOGI(TAG, "Send User %s", CONFIG_TELNET_USER);
				socketBuf.command = CMD_USER;
				strcpy(socketBuf.data, CONFIG_TELNET_USER);
				strcat(socketBuf.data, CRLF);
				socketBuf.length = strlen(socketBuf.data);
				socketBuf.wait = 2;
				xQueueSend(xQueueSocket, &socketBuf, 0);

				// start underline mode to stdout
				stdoutBuf.command = CMD_STARTU;
				xQueueSend(xQueueStdout, &stdoutBuf, 0);

				// send user name to stdout
				stdoutBuf.command = CMD_WRITE;
				for(int i=0;i<strlen(socketBuf.data);i++) {
					stdoutBuf.data = socketBuf.data[i];
					xQueueSend(xQueueStdout, &stdoutBuf, 0);
				}

				// stop underline mode to stdout
				stdoutBuf.command = CMD_STOPU;
				xQueueSend(xQueueStdout, &stdoutBuf, 0);

				stdinStatus++;
			} else if (stdinStatus == 1) {
				ESP_LOGI(TAG, "Send Pass %s", CONFIG_TELNET_PASSWORD);
				socketBuf.command = CMD_PASS;
				strcpy(socketBuf.data, CONFIG_TELNET_PASSWORD);
				strcat(socketBuf.data, CRLF);
				socketBuf.length = strlen(socketBuf.data);
				socketBuf.wait = 2;
				xQueueSend(xQueueSocket, &socketBuf, 0);
				stdinStatus++;
#if 0
			} else if (stdinStatus == 2) {
				ESP_LOGI(TAG, "Send Cmd");
				socketBuf.command = CMD_REMOTE;
				strcpy(socketBuf.data, "ls");
				strcat(socketBuf.data, CRLF);
				socketBuf.length = strlen(socketBuf.data);
				xQueueSend(xQueueSocket, &socketBuf, 0);
				stdinStatus++;
#endif
			} else if (stdinStatus == 2) {
				char line[64];
				long wait = getLine(f, line, sizeof(line));
				ESP_LOGI(TAG, "wait=%ld", wait);
				if (wait < 0) {
					ESP_LOGI(TAG, "Send Exit");
					socketBuf.command = CMD_EXIT;
					strcpy(socketBuf.data, "exit");
					strcat(socketBuf.data, CRLF);
					socketBuf.length = strlen(socketBuf.data);
					socketBuf.wait = 2;

					// send underline mode to stdout
					stdoutBuf.command = CMD_STARTU;
					xQueueSend(xQueueStdout, &stdoutBuf, 0);
				} else {
					socketBuf.command = CMD_REMOTE;
					strcpy(socketBuf.data, line);
					strcat(socketBuf.data, CRLF);
					socketBuf.length = strlen(socketBuf.data);
					socketBuf.wait = wait;

					// send underline mode to stdout
					stdoutBuf.command = CMD_STARTU;
					xQueueSend(xQueueStdout, &stdoutBuf, 0);
				}
				xQueueSend(xQueueSocket, &socketBuf, 0);
			}
		}
	} // end while

    ESP_LOGW(TAG, "finish");
    vTaskDelete(NULL);
}

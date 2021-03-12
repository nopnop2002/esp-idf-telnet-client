/* This is an alternative process to STDOUT

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

static const char *TAG = "STDOUT";
static const char *READ = "READ";

extern QueueHandle_t xQueueSocket;
extern QueueHandle_t xQueueStdin;
extern QueueHandle_t xQueueStdout;

extern char *MOUNT_POINT;


static void readFile(char * fileName) {
	// Open file for reading
	ESP_LOGI(READ, "Reading %s", fileName);
	char line[128];
	FILE* f = fopen(fileName, "r");
	if (f == NULL) {
		ESP_LOGW(READ, "Failed to open file for reading");
	} else {
		while (fgets(line, sizeof(line), f) != NULL) {
			//ESP_LOGI(READ, "line=[%s]", line);
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
			ESP_LOGI(READ, "line=[%s]", line);
		}
	}
	fclose(f);
}

void stdout_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Start");
	esp_log_level_set(TAG, ESP_LOG_WARN);
	esp_log_level_set(READ, ESP_LOG_WARN);

	STDOUT_t stdoutBuf;

	// Open file for writing
	char fileName[128];
	int isOpened = 0;
	sprintf(fileName, "%s/stdout.txt", MOUNT_POINT);
	ESP_LOGI(TAG, "fileName=%s", fileName);
	FILE* f = NULL;
	int underLine = 0;

	while(1) {
		//Waiting for STDOUT event.
		if (xQueueReceive(xQueueStdout, &stdoutBuf, portMAX_DELAY) == pdTRUE) {
			if (stdoutBuf.command == CMD_OPEN) {
				f = fopen(fileName, "w");
				if (f == NULL) {
					ESP_LOGE(TAG, "Fail to open file for writing");
				} else {
					ESP_LOGI(TAG, "Success to open file for writing");
					isOpened = 1;
					fprintf(f, "<!DOCTYPE html>\n");
					fprintf(f, "<html lang=\"en\">\n");
					fprintf(f, "<head>\n");
					fprintf(f, "<title>stdout</title>\n");
					fprintf(f, "</head>\n");
					fprintf(f, "<body>\n");
					fprintf(f, "<h1>stdout</h1>\n");
				}

			} else if (stdoutBuf.command == CMD_WRITE) {
				if (!isOpened) continue;
				if (stdoutBuf.data == 0x0d) {
					ESP_LOGI(TAG, "CR");
					if (underLine) {
						fputs("</u>", f);
						underLine = 0;
					}
					fputs("<br>", f);
				} else	if (stdoutBuf.data == 0x0a) {
					ESP_LOGI(TAG, "LF");
				}
				fputc(stdoutBuf.data, f);

			} else if (stdoutBuf.command == CMD_STARTU) {
				if (!isOpened) continue;
				fputs("<u>", f);
				underLine = 1;

			} else if (stdoutBuf.command == CMD_STOPU) {
				if (!isOpened) continue;
				fputs("</u>", f);
				underLine = 0;

			} else if (stdoutBuf.command == CMD_CLOSE) {
				ESP_LOGI(TAG, "close file");
				if (isOpened) {
					fprintf(f, "</body>\n");
					fprintf(f, "</html>\n");
					fclose(f);
				}
				readFile(fileName);
				break;
			}

		}
	} // end while

	ESP_LOGW(TAG, "finish");
	vTaskDelete(NULL);
}

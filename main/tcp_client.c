/* BSD Socket API Example

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

#define BUFFER_SZ 1000

static const char *TAG = "SOCKET";

extern QueueHandle_t xQueueSocket;
extern QueueHandle_t xQueueStdin;

void handle_data_just_read(int sck_fd, char *data, int num_bytes){
	char out_buf[BUFFER_SZ];
	int out_ptr = 0;

	char IAC = (char)255; //IAC Telnet byte consists of all ones

	SOCKET_t socketBuf;
	socketBuf.command = CMD_SOCKET;
	socketBuf.taskHandle = xTaskGetCurrentTaskHandle();

	for(int i = 0; i < num_bytes; i++){
		if(data[i] == IAC){
			//command sequence follows
			ESP_LOGI(TAG, "IAC detected");
			out_buf[out_ptr++] = IAC;
			i++;
			switch(data[i]){
				case ((char)251):
					//WILL as input
					ESP_LOGI(TAG, "WILL detected");
					out_buf[out_ptr++] = (char)254; //DON'T, rejecting WILL
					i++;
					out_buf[out_ptr++] = data[i];
					break;
				case ((char)252):
					//WON'T as input
					ESP_LOGI(TAG, "getting won't as input, weird!");
					break;
				case ((char)253):
					//DO as input
					ESP_LOGI(TAG, "DO detected");
					out_buf[out_ptr++] = (char)(char)252; //WON'T, rejecting DO
					i++;
					out_buf[out_ptr++] = data[i];
					break;
				case ((char)254):
					//DON'T as input
					ESP_LOGI(TAG, "getting don't as input, weird!");
					break;
				default:
					//not a option negotiation, a genereal TELNET command like IP, AO, etc.
					ESP_LOGI(TAG, "general TELNET command %x", (int)data[i]);
					break;
			}
		}else{
			//is data
			socketBuf.data = data[i];
			xQueueSend(xQueueSocket, &socketBuf, 0);
			printf("%c", data[i]);
		}
	}

	fflush(stdout); //need to flush output here as printf doesn't do so unless '\n' is encountered, and we're printing char-by-char the real data

	if (out_ptr) {
		if(write(sck_fd, out_buf, out_ptr) == -1){
			ESP_LOGE(TAG, "error write()'ing");
			while(1) { vTaskDelay(1); }
		}else {
			ESP_LOGI(TAG, "packet sent! out_ptr=%d", out_ptr);
		}
	}

#if 0
	if (out_ptr) {
		if(DEBUG){ printf("packet sent! out_ptr=%d\n\n", out_ptr); }
	}
#endif
}

#define WAITSEC 5

void socket_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Start TELNET_SERVER=%s", CONFIG_TELNET_SERVER);
	char host_ip[] = CONFIG_TELNET_SERVER;
	int addr_family = 0;
	int ip_protocol = 0;
	esp_log_level_set(TAG, ESP_LOG_WARN);

	struct sockaddr_in dest_addr;
	dest_addr.sin_addr.s_addr = inet_addr(host_ip);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(CONFIG_TELNET_PORT);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;

	int sock =	socket(addr_family, SOCK_STREAM, ip_protocol);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		while(1) { vTaskDelay(1); }
	}
	ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, CONFIG_TELNET_PORT);

	int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
	if (err != 0) {
		ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
		while(1) { vTaskDelay(1); }
	}
	ESP_LOGI(TAG, "Successfully connected");

	char inp_buf[BUFFER_SZ];
	STDIN_t stdinBuf;
	SOCKET_t socketBuf;
	socketBuf.command = CMD_STDIN;
	socketBuf.taskHandle = xTaskGetCurrentTaskHandle();
	int waitSec = WAITSEC;

	while (1) {
		fd_set fds;
		ESP_LOGI(TAG, "waitSec=%d", waitSec);
		struct timeval tv = {
			.tv_sec = waitSec,
			.tv_usec = 0,
		};

		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		int s = select(sock+1, &fds, NULL, NULL, &tv); //IO Multiplexing
		ESP_LOGI(TAG, "s=%d", s);
		if (s < 0) {
			ESP_LOGE(TAG, "Select failed: errno %d", errno);
			while(1) { vTaskDelay(1); }
		} else if (s == 1) {
		//if(FD_ISSET(sock, &fds)){
			//read from TCP socket
			int num_read = read(sock, inp_buf, BUFFER_SZ);
			ESP_LOGI(TAG, "reading from socket(%d bytes read)", num_read);
			if(num_read == -1){
				ESP_LOGI(TAG, "error read()'ing");
				while(1) { vTaskDelay(1); }
			}else if(num_read == 0){
				ESP_LOGW(TAG, "EOF reached, connection closed");
				while(1) { vTaskDelay(1); }
			}
			handle_data_just_read(sock, inp_buf, num_read);
		} else {
			ESP_LOGI(TAG, "Timeout has been reached and nothing has been received");
			xQueueSend(xQueueSocket, &socketBuf, 0);
			xQueueReceive(xQueueStdin, &stdinBuf, portMAX_DELAY);
			//read from standard input
			ESP_LOGI(TAG, "reading from stdin(%d bytes read)", stdinBuf.length);
			waitSec = stdinBuf.wait;
			write(sock, stdinBuf.data, stdinBuf.length);
		}
	}
	vTaskDelete(NULL);
}



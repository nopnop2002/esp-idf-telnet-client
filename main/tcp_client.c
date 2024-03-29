/* BSD Socket API Example

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
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
extern QueueHandle_t xQueueStdout;
extern QueueHandle_t xQueueHttp;

void handle_data_just_read(int sck_fd, char *data, int num_bytes){
	char out_buf[BUFFER_SZ];
	int out_ptr = 0;

	char IAC = (char)255; //IAC Telnet byte consists of all ones

	STDOUT_t stdoutBuf;
	stdoutBuf.taskHandle = xTaskGetCurrentTaskHandle();

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
			stdoutBuf.command = CMD_WRITE;
			stdoutBuf.data = data[i];
			xQueueSend(xQueueStdout, &stdoutBuf, 0);
			printf("%c", data[i]);
		}
	}

	//need to flush output here as printf doesn't do so unless '\n' is encountered, and we're printing char-by-char the real data
	fflush(stdout);

	if (out_ptr) {
		if(write(sck_fd, out_buf, out_ptr) == -1){
			ESP_LOGE(TAG, "error write()'ing");
			while(1) { vTaskDelay(1); }
		}else {
			ESP_LOGI(TAG, "packet sent! out_ptr=%d", out_ptr);
		}
	}

}

#define WAITSEC 5

void socket_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Start");
	ESP_LOGI(TAG, "Your TELNET SERVER is %s", CONFIG_TELNET_SERVER);
	esp_log_level_set(TAG, ESP_LOG_WARN);

	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(CONFIG_TELNET_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(CONFIG_TELNET_SERVER);
	ESP_LOGI(TAG, "dest_addr.sin_addr.s_addr=0x%"PRIx32, dest_addr.sin_addr.s_addr);
	if (dest_addr.sin_addr.s_addr == 0xffffffff) {
		struct hostent *hp;
		hp = gethostbyname(CONFIG_TELNET_SERVER);
		if (hp == NULL) {
			ESP_LOGE(TAG, "FTP Client Error: Connect to %s", CONFIG_TELNET_SERVER);
			while(1) { vTaskDelay(1); }
		}
		struct ip4_addr *ip4_addr;
		ip4_addr = (struct ip4_addr *)hp->h_addr;
		dest_addr.sin_addr.s_addr = ip4_addr->addr;
	}
	
	int sock =	socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		while(1) { vTaskDelay(1); }
	}
	ESP_LOGI(TAG, "Socket created, connecting to %s:%d", CONFIG_TELNET_SERVER, CONFIG_TELNET_PORT);

	int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
	if (err != 0) {
		ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
		while(1) { vTaskDelay(1); }
	}
	ESP_LOGI(TAG, "Successfully connected to server");

	STDOUT_t stdoutBuf;
	stdoutBuf.taskHandle = xTaskGetCurrentTaskHandle();
	stdoutBuf.command = CMD_OPEN;
	xQueueSend(xQueueStdout, &stdoutBuf, 0);

	char inp_buf[BUFFER_SZ];
	STDIN_t stdinBuf;
	stdinBuf.taskHandle = xTaskGetCurrentTaskHandle();

	SOCKET_t socketBuf;

	HTTP_t httpBuf;
	httpBuf.taskHandle = xTaskGetCurrentTaskHandle();
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
				stdinBuf.command = CMD_CLOSE;
				xQueueSend(xQueueStdin, &stdinBuf, 0);
				stdoutBuf.command = CMD_CLOSE;
				xQueueSend(xQueueStdout, &stdoutBuf, 0);
				httpBuf.command = CMD_CLOSE;
				xQueueSend(xQueueHttp, &httpBuf, 0);
				break;
			}
			handle_data_just_read(sock, inp_buf, num_read);
		} else {
			ESP_LOGI(TAG, "Timeout has been reached and nothing has been received");
			//send to STDIN
			stdinBuf.command = CMD_READ;
			xQueueSend(xQueueStdin, &stdinBuf, 0);
			//receive from STDIN
			xQueueReceive(xQueueSocket, &socketBuf, portMAX_DELAY);
			ESP_LOGI(TAG, "reading from stdin(%d bytes read)", socketBuf.length);
			waitSec = socketBuf.wait;
			write(sock, socketBuf.data, socketBuf.length);
		}
	}
	
	ESP_LOGW(TAG, "finish");
	vTaskDelete(NULL);
}

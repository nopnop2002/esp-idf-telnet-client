#define CMD_SOCKET	100
#define CMD_STDIN	200

typedef struct {
	uint16_t command;
	char data;
	TaskHandle_t taskHandle;
} SOCKET_t;

typedef struct {
	char data[64];
	size_t length;
	long wait;
	TaskHandle_t taskHandle;
} STDIN_t;

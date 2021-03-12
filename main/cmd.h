#define CMD_OPEN	100

#define CMD_WRITE	200
#define CMD_STARTU	220
#define CMD_STOPU	240

#define CMD_USER	300
#define CMD_PASS	320
#define CMD_REMOTE	340
#define CMD_EXIT	360

#define	CMD_READ	400

#define CMD_CLOSE	900

// Message to STDIN
typedef struct {
	uint16_t command;
	TaskHandle_t taskHandle;
} STDIN_t;


// Message to STDOUT
typedef struct {
	uint16_t command;
	char data;
	TaskHandle_t taskHandle;
} STDOUT_t;

// Message to SCOKET
typedef struct {
	uint16_t command;
	char data[64];
	size_t length;
	long wait;
	TaskHandle_t taskHandle;
} SOCKET_t;

// Message to HTTP
typedef struct {
	uint16_t command;
	TaskHandle_t taskHandle;
} HTTP_t;

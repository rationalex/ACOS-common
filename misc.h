#define DAEMON_NAME "my_awesome_daemon"
#define DAEMON_USERNAME "daemonuser"
#define NUMSEMS 2
#define SHMEMPATH "/bin/ls"
#define SHMEMKEY '1'
#define BUF_SIZE 100
#define RES_FREE 2

enum packet_types {PK_SEND_DATA,
                   PK_SEND_FILENAME,
                   PK_FILENAME_OK,
                   PK_ERROR,
                   PK_EOF,
                   PK_OK,
                   PK_END};

enum server_status {ST_UNKNOWN,
                    ST_READ,
                    ST_SEND};

typedef struct {
    enum packet_types pk_type;
/** not actually a text content but bytes */
    char data[BUF_SIZE];
/** how many bytes were read actually */
    size_t read_len;
} mem_t;

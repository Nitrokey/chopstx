#define BUFSIZE 64

struct serial;

struct serial *serial_open (uint8_t num);
void serial_wait_configured (struct serial *);
void serial_wait_connection (struct serial *);
int serial_send (struct serial *s, const char *buf, int count);
int serial_recv (struct serial *s, char *buf, uint32_t *timeout);

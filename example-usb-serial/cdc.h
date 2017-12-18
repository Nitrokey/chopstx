#define BUFSIZE 64

struct cdc;

void
cdc_init (uint16_t prio, uintptr_t stack_addr, size_t stack_size,
	  void (*sendbrk_callback) (uint8_t dev_no, uint16_t duration),
	  void (*config_callback) (uint8_t dev_no,
				   uint32_t bitrate, uint8_t format,
				   uint8_t paritytype, uint8_t databits));
void cdc_wait_configured (void);

struct cdc *cdc_open (uint8_t num);
void cdc_wait_connection (struct cdc *);
int cdc_send (struct cdc *s, const char *buf, int count);
int cdc_recv (struct cdc *s, char *buf, uint32_t *timeout);
int cdc_ss_notify (struct cdc *s, uint16_t state_bits);

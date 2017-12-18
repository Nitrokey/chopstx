#define BUFSIZE 64

struct cdc;

void cdc_init (void);
void cdc_wait_configured (void);

struct cdc *cdc_open (uint8_t num);
void cdc_wait_connection (struct cdc *);
int cdc_send (struct cdc *s, const char *buf, int count);
int cdc_recv (struct cdc *s, char *buf, uint32_t *timeout);
int cdc_ss_notify (struct cdc *s, uint16_t state_bits);

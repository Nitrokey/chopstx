struct bb {
  chopstx_mutex_t mutex;
  chopstx_cond_t cond;
  uint32_t items;
  uint32_t max_item;
};

void bb_init (struct bb *bb, uint32_t max_item);
void bb_get (struct bb *bb);
void bb_put (struct bb *bb);

void bb_prepare_poll (struct bb *bb, chopstx_poll_cond_t *p, int full);

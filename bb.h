struct bb {
  chopstx_mutex_t mutex;
  chopstx_cond_t cond;
  uint32_t items;
};

void bb_init (struct bb *bb, uint32_t items);
void bb_get (struct bb *bb);
void bb_put (struct bb *bb);

void bb_prepare_poll (struct bb *bb, chopstx_poll_cond_t *p);

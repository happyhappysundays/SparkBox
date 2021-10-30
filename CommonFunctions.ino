


void uint_to_bytes(unsigned int i, uint8_t *h, uint8_t *l) {
  *h = (i & 0xff00) / 256;
  *l = i & 0xff;
}

void bytes_to_uint(uint8_t h, uint8_t l,unsigned int *i) {
  *i = (h & 0xff) * 256 + (l & 0xff);
}

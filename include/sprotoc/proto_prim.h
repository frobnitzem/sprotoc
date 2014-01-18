// varint-size primitives
static inline unsigned size_uint32(uint32_t v) {
  if (v < (1<<7)) return 1;
  else if (v < (1<<14)) return 2;
  else if (v < (1<<21)) return 3;
  else if (v < (1<<28)) return 4;
  else return 5;
}
static inline unsigned size_int32(int32_t v) {
  if (v < 0) return 10;
  else if (v < (1<<7)) return 1;
  else if (v < (1<<14)) return 2;
  else if (v < (1<<21)) return 3;
  else if (v < (1<<28)) return 4;
  else return 5;
}
/* return the zigzag-encoded 32-bit unsigned int from a 32-bit signed int */
static uint32_t zigzag32(int32_t v) {
  if (v < 0)
    return ((uint32_t)(-v)) * 2 - 1;
  else
    return v * 2;
}
static inline unsigned size_sint32(int32_t v) {
  return size_uint32(zigzag32(v));
}
static inline unsigned size_uint64(uint64_t v) {
  uint32_t upper_v = (uint32_t )(v>>32);
  if (upper_v == 0) return size_uint32((uint32_t)v);
  else if (upper_v < (1<<3)) return 5;
  else if (upper_v < (1<<10)) return 6;
  else if (upper_v < (1<<17)) return 7;
  else if (upper_v < (1<<24)) return 8;
  else if (upper_v < (1U<<31)) return 9;
  else return 10;
}
/* return the zigzag-encoded 64-bit unsigned int from a 64-bit signed int */
static inline uint64_t zigzag64(int64_t v)
{
  if (v < 0)
    return ((uint64_t)(-v)) * 2 - 1;
  else
    return v * 2;
}
static inline unsigned size_sint64(int64_t v) {
  return size_uint64(zigzag64(v));
}

// varint-write primitives
static inline unsigned write_uint32(SWriter *s, uint32_t value) {
  unsigned char out[5];
  unsigned rv = 0;
  if (value >= 0x80)
    {
      out[rv++] = value | 0x80;
      value >>= 7;
      if (value >= 0x80)
        {
          out[rv++] = value | 0x80;
          value >>= 7;
          if (value >= 0x80)
            {
              out[rv++] = value | 0x80;
              value >>= 7;
              if (value >= 0x80)
                {
                  out[rv++] = value | 0x80;
                  value >>= 7;
                }
            }
        }
    }
  /* assert: value<128 */
  out[rv++] = value;
  s->write(s->stream, out, rv);
  return rv;
}

/* Pack a 32-bit integer in zigzag encoding. */
static inline unsigned write_sint32(SWriter *s, int32_t value) {
    return write_uint32(s, zigzag32(value));
}

/* Pack a 32-bit signed integer, returning the number of bytes needed.
   Negative numbers are packed as twos-complement 64-bit integers. */
static inline unsigned write_int32(SWriter *s, int32_t value) {
    unsigned char out[10];
    if (value >= 0) {
        return write_uint32(s, value);
    }
    out[0] = value | 0x80;
    out[1] = (value>>7) | 0x80;
    out[2] = (value>>14) | 0x80;
    out[3] = (value>>21) | 0x80;
    out[4] = (value>>28) | 0x80;
    out[5] = out[6] = out[7] = out[8] = 0xff;
    out[9] = 0x01;
    s->write(s->stream, out, 10);
    return 10;
}

/* Pack a 64-bit unsigned integer that fits in a 64-bit uint,
   using base-128 encoding. */
static inline unsigned write_uint64(SWriter *s, uint64_t value) {
  unsigned char out[10];
  uint32_t hi = (uint32_t )(value>>32);
  uint32_t lo = (uint32_t )value;
  unsigned rv;
  if (hi == 0)
    return write_uint32(s, (uint32_t)lo);
  out[0] = (lo) | 0x80;
  out[1] = (lo>>7) | 0x80;
  out[2] = (lo>>14) | 0x80;
  out[3] = (lo>>21) | 0x80;
  if (hi < 8) {
      out[4] = (hi<<4) | (lo>>28);
      return 5;
  } else {
      out[4] = ((hi&7)<<4) | (lo>>28) | 0x80;
      hi >>= 3;
  }
  rv = 5;
  while (hi >= 128) {
      out[rv++] = hi | 0x80;
      hi >>= 7;
  }
  out[rv++] = hi;
  s->write(s->stream, out, rv);
  return rv;
}
static inline unsigned write_sint64(SWriter *s, int64_t value) {
  return write_uint64(s, zigzag64(value));
}
/* Pack a 32-bit signed integer, returning the number of bytes needed.
   Negative numbers are packed as twos-complement 64-bit integers. */
static inline unsigned write_int64(SWriter *s, int64_t value) {
    unsigned char out[10];
    if (value >= 0) {
        return write_uint64(s, value);
    }
    out[0] = value | 0x80;
    out[1] = (value>>7) | 0x80;
    out[2] = (value>>14) | 0x80;
    out[3] = (value>>21) | 0x80;
    out[4] = (value>>28) | 0x80;
    out[5] = (value>>35) | 0x80;
    out[6] = (value>>42) | 0x80;
    out[7] = (value>>49) | 0x80;
    out[8] = (value>>56) | 0x80;
    out[9] = 0x01;
    s->write(s->stream, out, 10);
    return 10;
}

/* Pack a 32-bit value, little-endian.
   Used for fixed32, sfixed32, float) */
static inline unsigned write_fixed32(SWriter *s, uint32_t value) {
  union {
      unsigned char out[4];
      uint32_t tv;
  } v;
  v.tv = value;
  if((value & 255) != v.out[0] || (value >> 8 & 255) != v.out[1]) {
     // big-endian system AND non-palindrome
     v.out[2] = v.out[1];
     v.out[3] = v.out[0];
     v.out[0] = value & 255;
     v.out[1] = value >> 8 & 255;
  }
  s->write(s->stream, v.out, 4);
  return 4;
}
static inline unsigned write_fixed64(SWriter *s, uint64_t value) {
  write_fixed32(s, value);
  write_fixed32(s, value>>32);
  return 8;
}
static inline unsigned write_bool(SWriter *s, unsigned value) {
  unsigned char out = value ? 1 : 0;
  s->write(s->stream, &out, 1);
  return 1;
}

// varint-read primitives
static inline unsigned read_fuint32(uint32_t *n, const uint8_t *data, size_t sz) {
  if(sz < 4) return sz > 0 ? sz : 0;
#if !defined(WORDS_BIGENDIAN)
  uint32_t t;
  memcpy (n, data, 4);
#else
  *n = data[0] | ((uint32_t)(data[1]) << 8) | ((uint32_t)(data[2]) << 16)
         | ((uint32_t)(data[3]) << 24);
#endif
  return 4;
}
static inline unsigned read_uint32(uint32_t *n, const uint8_t *data, size_t sz) {
  *n = data[0] & 0x7f;
  if(sz >= 5) { // don't worry about len
      if(data[0] < 0x7f)
          return 1;
      *n |= ((uint32_t)(data[1] & 0x7f) << 7);
      if(data[1] < 0x7f)
          return 2;
      *n |= ((uint32_t)(data[2] & 0x7f) << 14);
      if(data[2] < 0x7f)
          return 3;
      *n |= ((uint32_t)(data[3] & 0x7f) << 21);
      if(data[3] < 0x7f)
          return 4;
      *n |= ((uint32_t)(data[4] & 0x7f) << 28);
      return 5;
  }
  if(data[0] < 0x7f || sz < 2)
          return 1;
  *n |= ((uint32_t)(data[1] & 0x7f) << 7);
  if(data[1] < 0x7f || sz < 3)
          return 2;
  *n |= ((uint32_t)(data[2] & 0x7f) << 14);
  if(data[2] < 0x7f || sz < 4)
          return 3;
  *n |= ((uint32_t)(data[3] & 0x7f) << 21);
  return 4;
}
static inline unsigned read_sint32(int32_t *n, const uint8_t *data, size_t sz) {
    uint32_t v;
    unsigned k = read_uint32(&v, data, sz);
    if (v&1)
        *n = -(v>>1) - 1;
    else
        *n = v>>1;
    return k;
}
static inline unsigned read_fuint64(uint64_t *n, const uint8_t *data, size_t sz) {
  if(sz < 8) return sz > 0 ? sz : 0;
#if !defined(WORDS_BIGENDIAN)
  memcpy(&n, data, 8);
  return 8;
#else
  {   uint32_t t, u;
      read_fuint32(&t, data, sz);
      read_fuint32(&u, data+4, sz-4);
      *n = (uint64_t)t | (uint64_t)u << 32;
  }
  return 8;
#endif
}
static inline unsigned read_uint64(uint64_t *n, const uint8_t *data, size_t sz) {
  unsigned shift, i;
  uint32_t rv;
  i = read_uint32(&rv, data, sz);
  *n = rv;
  if(i < 5) return i;
  shift = 5*7;
  sz = sz > 10 ? 10 : sz;
  for(; i < sz && data[i-1] >= 128; i++) {
      *n |= (((uint64_t)(data[i]&0x7f)) << shift);
      shift += 7;
  }
  return i;
}
static inline unsigned read_sint64(int64_t *n, const uint8_t *data, size_t sz) {
    uint64_t v;
    unsigned k = read_uint64(&v, data, sz);
    if (v&1)
        *n = -(v>>1) - 1;
    else
        *n = v>>1;
    return k;
}
static inline unsigned read_bool(unsigned *n, const uint8_t *data, size_t sz) {
  unsigned i;
  *n = data[0] > 0;
  sz = sz > 10 ? 10 : sz;
  for(i=0; i < sz && data[i] >= 128; i++);
  return i;
}

static inline size_t skip_len(uint32_t tag, const uint8_t *buf, size_t sz) {
    uint64_t n;
    size_t k;

    switch(tag & 7) {
        case 0: // int
            k = read_int64(&n, buf, sz);
            return k;
        case 2: // len-delimited
            k = read_uint64(&n, buf, sz);
            return n + k;
        case 1: // 64-bit
            return 8;
        case 5: // 32-bit
            return 4;
        default: // unknown type -> done
            //DEBUG_MSG("Cannot skip unknown tag type!\n");
            return sz;
        }
}


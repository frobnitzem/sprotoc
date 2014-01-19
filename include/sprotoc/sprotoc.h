#include <stdint.h>
#include "rbtree.h"

// Generic writer
typedef struct {
    void (*write)(void *stream, const void *buf, size_t len);
    void *stream;
} SWriter;
void _swrite_to_string(void *posin, const void *buf, size_t len);
void _swrite_to_file(void *fin, const void *buf, size_t len);

// For caching recursive object sizes
struct _fszmap;
struct _fszmap {
    uint32_t field; // WARNING: highest-pri. bit used for r/b tree color
    struct _fszmap *left, *right;
    struct _fszmap *sub; // binary tree of children (sub-messages)
    struct _fszmap *next; // linked-list for repeated msg
    size_t len; // current node size
};

// Simple linked-list of memory regions for managing recursive objects.
// The max size that can be allocated in the first heap
// is ALLOCATOR_SIZE - sizeof(Allocator)
#define ALLOCATOR_SIZE (4096)
typedef struct allocator Allocator;
struct allocator {
    void *sp;
    size_t avail;
    Allocator *next;
};
Allocator *allocator_ctor();
void allocator_dtor(Allocator **);
void *allocate(Allocator *, size_t);

// Primitive sizing functions
static inline unsigned size_uint32(uint32_t);
static inline unsigned size_int32(int32_t);
static inline unsigned size_sint32(int32_t);
static inline unsigned size_uint64(uint64_t);
#define size_int64(v) size_uint64((uint64_t)(v))
static inline unsigned size_sint64(int64_t);

// Primitive writing functions
static inline unsigned write_uint32(SWriter *, uint32_t);
static inline unsigned write_sint32(SWriter *, int32_t);
static inline unsigned write_int32(SWriter *, int32_t);
static inline unsigned write_uint64(SWriter *, uint64_t);
static inline unsigned write_sint64(SWriter *, int64_t);
static inline unsigned write_int64(SWriter *, int64_t);
static inline unsigned write_fixed32(SWriter *, uint32_t);
static inline unsigned write_fixed64(SWriter *, uint64_t);
static inline unsigned write_bool(SWriter *, unsigned);

// Primitive reading functions
#define read_fixed32(n, data, sz) read_fuint32((uint32_t *)n, data, sz)
static inline unsigned read_fuint32(uint32_t *, const uint8_t *, size_t);
static inline unsigned read_uint32(uint32_t *, const uint8_t *, size_t);
static inline unsigned read_sint32(int32_t *, const uint8_t *, size_t);
#define read_int32(n, data, sz) read_uint32((uint32_t *)n, data, sz)
#define read_fixed64(n, data, sz) read_fuint64((uint64_t *)n, data, sz)
static inline unsigned read_fuint64(uint64_t *, const uint8_t *, size_t);
static inline unsigned read_uint64(uint64_t *, const uint8_t *, size_t);
static inline unsigned read_sint64(int64_t *, const uint8_t *, size_t);
#define read_int64(n, data, sz) read_uint64((uint64_t *)n, data, sz)
static inline unsigned read_bool(unsigned *, const uint8_t *, size_t);

static inline size_t skip_len(uint32_t tag, const uint8_t *, size_t);

#include "proto_prim.h"

// Primitive sizes (defined using varint-encoding sizes)
/*
#define protosz_int32(wd, i)  (size_uint32((wd) << 3) +  size_int32(i))
#define protosz_uint32(wd, i) (size_uint32((wd) << 3) + size_uint32(i))
#define protosz_sint32(wd, i) (size_uint32((wd) << 3) + size_sint32(i))
#define protosz_int64(wd, i)  (size_uint32((wd) << 3) +  size_int32(i))
#define protosz_uint64(wd, i) (size_uint32((wd) << 3) + size_uint64(i))
#define protosz_sint64(wd, i) (size_uint32((wd) << 3) + size_sint64(i))

#define protosz_enum(wd, i)   protosz_uint32(wd, i)
#define protosz_float(wd, x)  (size_uint32((wd) << 3) + 4)
#define protosz_double(wd, x) (size_uint32((wd) << 3) + 8)
static inline size_t protosz_string(uint32_t wd, char *s) {
    uint64_t slen = strlen(s);
    return size_uint32(wd << 3) + size_uint64(slen) + slen;
}*/
#define size_fixed32(a) (4)
#define size_fixed64(a) (8)
#define size_bool(a) (1)

// Primitive writers
#define write_msg_int32(s, wd, i)  { write_uint32(s, (wd) << 3); \
                                         write_int32(s, i); }
#define write_msg_uint32(s, wd, i) { write_uint32(s, (wd) << 3); \
                                        write_uint32(s, i); }
#define write_msg_sint32(s, wd, i) { write_uint32(s, (wd) << 3); \
                                        write_sint32(s, i); }
#define write_msg_int64(s, wd, i)  { write_uint32(s, (wd) << 3); \
                                         write_int64(s, i); }
#define write_msg_uint64(s, wd, i) { write_uint32(s, (wd) << 3); \
                                        write_uint64(s, i); }
#define write_msg_sint64(s, wd, i) { write_uint32(s, (wd) << 3); \
                                        write_sint64(s, i); }

#define write_msg_bool(s, wd, i)     { write_uint32(s, (wd) << 3); \
                                        write_bool(s, i); }
#define write_msg_fixed32(s, wd, x)  { write_uint32(s, (wd) << 3 | 5); \
                                        write_fixed32(s, x); }
#define write_msg_fixed64(s, wd, x) { write_uint32(s, (wd) << 3 | 1); \
                                        write_fixed64(s, x); }

// Wrappers for field operations
// Sizing primitives
// optional fields are just expected to be preceded by if(r.has_ name)
#define SIZE_PRIM(type, name, wsz) \
    sz += wsz + size_ ## type(r.name);
#define SIZE_REP_PRIM(type, name, wsz) \
    if(r.name != NULL) { \
        int i; \
        for(i=0; i<r.n_ ## name; i++) { \
            sz += wsz + size_ ## type(r.name[i]); \
        } \
    }
#define SIZE_REP_PACKED(type, name, wsz) \
    if(r.name != NULL) { \
        int i; \
        size_t psz = 0; \
        for(i=0; i<r.n_ ## name; i++) { \
            psz += size_ ## type(r.name[i]); \
        } \
        sz += wsz + size_uint64(psz) + psz; \
    }
#define SIZE_STRING(name, wsz) \
    sz += wsz + size_uint64(r.len_ ## name) + r.len_ ## name;
#define SIZE_REP_STRING(name, wsz) \
    if(r.name != NULL && r.len_ ## name != NULL) { \
        int i; \
        sz += (wsz) * r.n_ ## name; \
        for(i=0; i<r.n_ ## name; i++) { \
            sz += size_uint64(r.len_ ## name[i]) + r.len_ ## name[i]; \
        } \
    }
#define SIZE_MSG(type, name, wd, wsz) { \
        c = size_ ## type(l, r.name); \
        c->field = wd; \
        sz += c->len + wsz + size_uint64(c->len); \
        add_node((void **)(&f->sub), c, &fszops); \
    }
// stores sizes of repeated messages in reverse order
#define SIZE_REP_MSG(type, name, wd, wsz) \
    if(r.name) { \
        int i; \
        struct _fszmap *next; \
        for(i=0; i<r.n_ ## name; i++) { \
            c = size_ ## type(l, r.name[i]); \
            c->field = wd; \
            sz += c->len + wsz + size_uint64(c->len); \
            next = add_node((void **)(&f->sub), c, &fszops); \
            c->next = next; \
        } \
    }

// Write primitives.
#define WRITE_PRIM(type, cast, name, wd) \
    write_msg_ ## type(s, wd, cast r.name);
#define WRITE_REP_PRIM(type, cast, name, wd) \
    if(r.name != NULL) { \
        int i; \
        for(i=0; i<r.n_ ## name; i++) { \
            write_msg_ ## type(s, wd, cast r.name[i]); \
        } \
    }
#define WRITE_REP_PACKED(type, cast, name, wd) \
    if(r.name != NULL) { \
        int i; \
        size_t psz = 0; \
        for(i=0; i<r.n_ ## name; i++) { \
            psz += size_ ## type(r.name[i]); \
        } \
        write_uint32(s, wd << 3 | 2); \
        write_uint64(s, psz); \
        for(i=0; i<r.n_ ## name; i++) { \
            write_ ## type(s, cast r.name[i]); \
        } \
    }
#define WRITE_STRING(name, wd) { \
        write_uint32(s, (wd) << 3 | 2); \
        write_uint64(s, r.len_ ## name); \
        s->write(s->stream, r.name, r.len_ ## name); \
    }
#define WRITE_REP_STRING(name, wd) \
    if(r.name != NULL && r.len_ ## name != NULL) { \
        int i; \
        for(i=0; i<r.n_ ## name; i++) { \
            write_uint32(s, (wd) << 3 | 2); \
            write_uint64(s, r.len_ ## name[i]); \
            s->write(s->stream, r.name[i], r.len_ ## name[i]); \
        } \
    }
#define WRITE_MSG(type, name, wd) { \
        { uint32_t i = wd; \
        c = lookup_node(f->sub, &i, &fszops); }; \
        if(c == fszops.nil) goto err; \
        write_uint32(s, wd << 3 | 2); \
        write_uint64(s, c->len); \
        if(write_ ## type(s, c, r.name)) return 1; \
    }
// Writes repeated messages by reversing the size list in-place first.
#define WRITE_REP_MSG(type, name, wd) \
    if(r.name) { \
        uint32_t i = wd; \
        type *next, *ptr = NULL; \
        c = lookup_node(f->sub, &i, &fszops); \
        for(i=0; i < r.n_ ## name; i++) { \
            next = c->next; \
            c->next = ptr; \
            ptr = c; \
            c = next; \
        } \
        c = ptr; \
        for(i=0; i < r.n_ ## name; i++) { \
            if(c == fszops.nil) goto err; \
            write_uint32(s, wd << 3 | 2); \
            write_uint64(s, c->len); \
            if(write_ ## type(s, c, r.name[i])) return 1; \
            c = c->next; \
        } \
    }


// Read primitives.
#define READ_PRIM(type, name) { \
        k = read_ ## type(&r.name, buf, sz); \
        buf += k; sz -= k; \
    }
#define READ_REP_PRIM(type, name) \
    if(r.n_ ## name < MAX_REPEATED) { \
        k = read_ ## type(&r.name[r.n_ ## name], buf, sz); \
        buf += k; sz -= k; \
        r.n_ ## name ++; \
    } else { \
        DEBUG_MSG("name reached MAX_REPEATED elements\n"); \
        goto skip; \
    }
#define READ_REP_PACKED(type, name) { \
        k = read_uint64(&n, buf, sz); \
        buf += k; sz -= k; \
        n = sz - n; \
        if(n < 0) sz = n = 0; \
        while(sz > n) { \
            READ_REP_PRIM(type, name); \
        } \
    }
#define READ_STRING(name) { \
    k = read_uint64(&n, buf, sz); \
    buf += k; sz -= k; \
    r.name = (char *)buf; \
    r.len_ ## name = n > sz ? sz : n; \
    buf += n; sz -= n; \
    }
#define READ_REP_STRING(name) \
    if(r.n_ ## name < MAX_REPEATED) { \
        k = read_uint64(&n, buf, sz); \
        buf += k; sz -= k; \
        r.name[r.n_ ## name] = (char *)buf; \
        r.len_ ## name[r.n_ ## name] = n > sz ? sz : n; \
        buf += n; sz -= n; \
        r.n_ ## name ++; \
    } else { \
        DEBUG_MSG("name reached MAX_REPEATED elements\n"); \
        goto skip; \
    }
#define READ_MSG(type, name) { \
        k = read_uint64(&n, buf, sz); \
        buf += k; sz -= k; \
        if(n <= sz) \
            r.name = read_ ## type(buf, n, info); \
        buf += n; sz -= n; \
    }
#define READ_REP_MSG(type, name) \
    if(r.n_ ## name < MAX_REPEATED) { \
        k = read_uint64(&n, buf, sz); \
        buf += k; sz -= k; \
        if(n <= sz && (r.name[r.n_ ## name] = read_ ## type(buf, n, info)) \
                    != NULL) { \
            r.n_ ## name ++; \
        } \
        buf += n; sz -= n; \
    } else { \
        DEBUG_MSG("name reached MAX_REPEATED elements\n"); \
        goto skip; \
    }

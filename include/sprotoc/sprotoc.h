/*    Copyright (C) David M. Rogers, 2015
 *    
 *    David M. Rogers <predictivestatmech@gmail.com>
 *    Nonequilibrium Stat. Mech. Research Group
 *    Department of Chemistry
 *    University of South Florida
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#ifndef _SPROTOC_INC
#define _SPROTOC_INC

#include <stdint.h>
#include "rbtree.h"

// Generic writer
typedef struct {
    void (*write)(void *stream, const void *buf, size_t len);
    void *stream;
} SWriter;
static inline void *_stream_tell(void **spos) {
    return *spos;
}
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
    sz += (wsz) + size_uint64(r.len_ ## name) + r.len_ ## name;
#define SIZE_REP_STRING(name, wsz) \
    if(r.name != NULL && r.len_ ## name != NULL) { \
        int i; \
        sz += (wsz) * r.n_ ## name; \
        for(i=0; i<r.n_ ## name; i++) { \
            sz += size_uint64(r.len_ ## name[i]) + r.len_ ## name[i]; \
        } \
    }
#define SIZE_MSG(type, name, wd, wsz) { \
        c = size_ ## type(l, r.name, user_info); \
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
            c = size_ ## type(l, r.name[i], user_info); \
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
#define WRITE_BYTES(name, wd) { \
        void *_cur_pos; \
        write_uint32(s, (wd) << 3 | 2); \
        write_uint64(s, r.len_ ## name); \
        _cur_pos = _stream_tell(s->stream); \
        r.write_ ## name(s, r.name, r.len_ ## name); \
        if(r.len_ ## name != _stream_tell(s->stream) - _cur_pos) { \
            DEBUG_MSG("Incorrect number of bytes written by %s (expected %d, got %ld)!\n", \
                        #name, r.len_ ## name, _stream_tell(s->stream) - _cur_pos); \
            *(void **)s->stream = _cur_pos + r.len_ ## name; \
        } \
    }
#define WRITE_REP_BYTES(name, wd) \
    if(r.name != NULL && r.write_ ## name != NULL) { \
        int i; \
        void *_cur_pos, *_next; \
        for(i=0; i<r.n_ ## name; i++) { \
            write_uint32(s, (wd) << 3 | 2); \
            write_uint64(s, r.len_ ## name[i]); \
            _cur_pos = _stream_tell(s->stream); \
            r.write_ ## name(s, r.name[i], r.len_ ## name[i]); \
            _next = _stream_tell(s->stream); \
            if(r.len_ ## name[i] != _next - _cur_pos) { \
                DEBUG_MSG("Incorrect number of bytes written by %s[%d] " \
                        "(expected %d, got %ld)!\n", \
                        #name, i, r.len_ ## name[i], _next - _cur_pos); \
                *(void **)s->stream = _cur_pos + r.len_ ## name[i]; \
            } \
            _cur_pos = _next; \
        } \
    }

#define WRITE_MSG(type, name, wd) { \
        { uint32_t i = wd; \
        c = lookup_node(f->sub, &i, &fszops); }; \
        if(c == fszops.nil) goto err; \
        write_uint32(s, wd << 3 | 2); \
        write_uint64(s, c->len); \
        if(write_ ## type(s, c, r.name, user_info)) return 1; \
    }
// Writes repeated messages by reversing the size list in-place first.
#define WRITE_REP_MSG(type, name, wd) \
    if(r.name) { \
        uint32_t i = wd; \
        void *next, *ptr = NULL; \
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
            if(write_ ## type(s, c, r.name[i], user_info)) return 1; \
            c = c->next; \
        } \
    }

// Read primitives.
// (start with resize utility on repeated messages)
#define CK_RESIZE(type, ptr, num, do_err) \
    if((num) >= MAX_REPEATED && (num)%MAX_REPEATED == 0) { \
        int p2 = (num) / MAX_REPEATED; \
        if(((p2-1)&p2) == 0) { /* power of 2 */ \
            type *u = malloc(((num)<<1)*sizeof(type)); \
            if(!u) { DEBUG_MSG("memory error\n"); \
                     do_err; } \
            memcpy(u, ptr, (num)*sizeof(type)); \
            if((num) != MAX_REPEATED) free(ptr); \
            ptr = u; \
        } \
    }

#define READ_PRIM(type, ctype, name) { \
        k = read_ ## type(&r.name, *buf, sz); \
        *buf += k; sz -= k; \
    }
#define READ_REP_PRIM(type, ctype, name) { \
        CK_RESIZE(ctype, r.name, r.n_ ## name, goto skip); \
        k = read_ ## type(&r.name[r.n_ ## name], *buf, sz); \
        *buf += k; sz -= k; \
        r.n_ ## name ++; \
    }

#define READ_REP_PACKED(type, ctype, name) { \
        k = read_uint64(&n, *buf, sz); \
        *buf += k; sz -= k; \
        n = sz - n; \
        if(n < 0) sz = n = 0; \
        while(sz > n) { \
            READ_REP_PRIM(type, ctype, name); \
        } \
    }
#define READ_STRING(name) { \
        k = read_uint64(&n, *buf, sz); \
        *buf += k; sz -= k; \
        r.name = (char *)*buf; \
        r.len_ ## name = n > sz ? sz : n; \
        *buf += n; sz -= n; \
    }
#define READ_REP_STRING(name) { \
        CK_RESIZE(int,    r.len_ ## name, r.n_ ## name, goto skip); \
        CK_RESIZE(void *, r.name,         r.n_ ## name, goto skip); \
        k = read_uint64(&n, *buf, sz); \
        *buf += k; sz -= k; \
        r.name[r.n_ ## name] = (char *)*buf; \
        r.len_ ## name[r.n_ ## name] = n > sz ? sz : n; \
        *buf += n; sz -= n; \
        r.n_ ## name ++; \
    }
#define READ_MSG(type, name) { \
        k = read_uint64(&n, *buf, sz); \
        *buf += k; sz -= k; \
        if(n <= sz) \
            r.name = read_ ## type(buf, n, info); \
        sz -= n; \
    }
#define READ_REP_MSG(type, name) { \
        CK_RESIZE(MY_ ## type *, r.name, r.n_ ## name, goto skip); \
        k = read_uint64(&n, *buf, sz); \
        *buf += k; sz -= k; \
        if(n <= sz && (r.name[r.n_ ## name] = read_ ## type(buf, n, info)) \
                    != NULL) { \
            r.n_ ## name ++; \
        } \
        sz -= n; \
    }

#endif

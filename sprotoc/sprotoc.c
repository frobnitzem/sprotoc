//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "sprotoc/sprotoc.h"

/*
// API helper functions (avoid caching sizes directly)
int $full_name$_to_file(FILE *file, MY_$full_name$ *a) {
    Allocator *l = allocator_ctor();
    struct _fszmap *f;
    SWriter s = { .f = _swrite_to_file, .ptr = file };
    int ret;

    f = protosz_$full_name$(l, MY_$full_name$ *a);
    s.ptr = buf;
    ret = write_msg_$full_name$(&s, f, a);

    allocator_dtor(&l);
    return ret;
}
char *$full_name$_to_string(size_t *len, MY_$full_name$ *a) {
    Allocator *l = allocator_ctor();
    char *buf;
    struct _fszmap *f;
    SWriter s = { .f = _swrite_to_string };

    f = protosz_$full_name$(l, MY_$full_name$ *a);
    *len = f->len;
    if( (buf = malloc(*len)) == NULL) {
        DEBUG_MSG("Memory allocation error.\n");
        allocator_dtor(&l);
        return NULL;
    }
    s.ptr = buf;
    if(write_msg_$full_name$(&s, f, a)) {
        free(buf);
        buf = NULL;
    }
    
    allocator_dtor(&l);
    return buf;
}*/

// helper functions (readers, writers, allocator)
void _swrite_to_file(void *fin, const void *buf, size_t len) {
    int fd = *(int *)fin;
    ssize_t ret;
    do {
        if( (ret = write(fd, buf, len)) >= 0) {
            len -= ret;
        }
    } while((ret <= 0 && (errno == EAGAIN || errno == EINTR))
                    || len > 0);
}

/*
void _swrite_to_string(void *lstin, const char *buf, size_t len) {
    struct string_info *s = lstin;
    size_t r;
    char *nsp;
    if(s->pos == NULL) {
        return;
    }
    if(len > s->avail) {
        r = 256*((s->len + len-s->avail+32)/256+1);
        if( (nsp = realloc(s->str, r)) == NULL) {
            goto err;
        }
        s->avail += r - s->len;
        s->pos = s->pos - s->str + nsp;
        s->str = nsp;
        s->len = r;
    }
    memcpy(s->pos, buf, len);
    s->pos += len;
    s->avail -= len;
}*/

// Silently overflows buffers and writes on subway walls (and tenement halls).
// Use only when you're sure of the length of all writes beforehand
// (like when you've accurately counted them)
void _swrite_to_string(void *posin, const void *buf, size_t len) {
    void **pos = posin;
    /*int i;
    for(i=0; i<len; i++)
        printf("%d ", ((uint8_t *)buf)[i]);
    printf("\n");*/
    memcpy(*pos, buf, len);
    *pos += len;
}

Allocator *allocator_ctor() {
    Allocator *l;
    if( (l = malloc(ALLOCATOR_SIZE)) == NULL) {
        return NULL;
    }
    l->sp = (void *)l + sizeof(Allocator);
    l->avail = ALLOCATOR_SIZE - sizeof(Allocator);
    l->next = NULL;
    return l;
}

void allocator_dtor(Allocator **ll) {
    Allocator *next, *l = *ll;
    while(l != NULL) {
        next = l->next;
        free(l);
        l = next;
    }
    *ll = NULL;
}

// a holds save-object
void save_allocator(Allocator *a, Allocator *b) {
    while(b->next != NULL) {
        b = b->next;
    }
    memcpy(a, b, sizeof(Allocator));
    a->next = b;
}

// b holds save-object
void restore_allocator(Allocator *a, Allocator *b) {
    while(a->next != NULL && a != b->next) {
        a = a->next;
    }
    if(a != b->next) {
        //DEBUG_MSG("Error restoring allocator - saved segment not found\n");
        return;
    }
    allocator_dtor(&a->next); // hack off extra allocations
    memcpy(a, b, sizeof(Allocator));
    a->next = NULL;
}

void *allocate(Allocator *l, size_t len) {
    void *b;
    while(l->next != NULL) {
        l = l->next;
    }
    if(len > l->avail) {
        size_t need = len > ALLOCATOR_SIZE - sizeof(Allocator) ?
                      len + sizeof(Allocator) : ALLOCATOR_SIZE;
        if( (l->next = malloc(need)) == NULL) {
            return NULL;
        }
        l = l->next;
        need -= sizeof(Allocator);
        l->sp = (void *)l + need;
        l->avail = need;
        l->next = NULL;
    }
    b = l->sp;
    l->sp += len; l->avail -= len;
    return b;
}

int lint32_cmp(const void *a, const void *b) {
    int32_t i = *(uint32_t *)a & 0x7fffffff;
    int32_t j = *(uint32_t *)b & 0x7fffffff;
    return i-j;
}

// Assumes system is little-endian (else boff = 0 + ...)
static const struct _fszmap dex;
const rbop_t fszops = {
    .cmp = lint32_cmp,
    .coff = (void *)&(dex.left) - (void *)&dex,
    .boff = 3 + (void *)&(dex.field) - (void *)&dex,
    .mask = 0x80,
    .nil = NULL,
};


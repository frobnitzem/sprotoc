#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "test_b.pb-c.h"

int main(int argc, char **argv) {
    Allocator *l = allocator_ctor();
    MY_foo_Person test = {
        .name = {6, "Sinbad"},
        .history = {9, "Privateer"},
        .args = {{4, "rrrr"},
                 {7, "AARRRR!"}},
        .n_args = 2,
    };
    MY_foo_Person *ret;
    size_t len;
    uint8_t *buf, *pos;
    int i;

    if( (buf = foo_Person_to_string(&len, &test)) == NULL) {
        fprintf(stderr, "Error writing test data.\n");
        return 1;
    }
    pos = buf; // running parser position
    ret = read_foo_Person(&pos, len, l);
    free(buf);
    if(ret == NULL) {
        return 1;
    }

    printf("Read %.*s ", ret->name.len, ret->name.data);
    if(ret->history.len)
        printf("(%.*s):", ret->history.len, ret->history.data);
    for(i=0; i<ret->n_args; i++) {
        printf(" %.*s", ret->args[i].len, ret->args[i].data);
    }
    printf("\n");

    allocator_dtor(&l);
    return 0;
}

// -------------------------------------------------------------------

void simple_writer(SWriter *s, void *data, size_t len) {
    s->write(s->stream, data, len);
}

void protowr_foo_Person(foo_Person *out, MY_foo_Person *a) {
    int i;
    out->write_name = &simple_writer;
    out->len_name = a->name.len;
    out->name = a->name.data;

    out->n_args = a->n_args;
    out->write_args = &simple_writer;
    for(i=0; i<a->n_args; i++) {
        out->len_args[i] = a->args[i].len;
        out->args[i] = a->args[i].data;
    }

    out->has_history = a->history.len > 0;
    if(out->has_history) {
        out->len_history = a->history.len;
        out->write_history = &simple_writer;
        out->history = a->history.data;
    }
}
MY_foo_Person *protord_foo_Person(void *info, foo_Person *r) {
    Allocator *l = info;
    MY_foo_Person *out;
    size_t len = sizeof(MY_foo_Person);
    int i;

    if( (out = allocate(l, len)) == NULL) return NULL;

    out->name.len = r->len_name;
    memcpy(out->name.data, r->name, r->len_name);
    out->n_args = r->n_args;
    for(i=0; i<r->n_args; i++) {
        size_t sz = r->len_args[i];
        out->args[i].len = sz;
        memcpy(out->args[i].data, r->args[i], sz);
    }
    if(r->has_history) {
        out->history.len = r->len_history;
        memcpy(out->history.data, r->history, r->len_history);
    } else {
        out->history.len = 0;
    }

    return out;
}

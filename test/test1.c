#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "test1.pb-c.h"

int main(int argc, char **argv) {
    Allocator *l = allocator_ctor();
    PhoneNumber phone[] = {
        { .num = 5551212,
          .type = foo_Person_HOME },
        { .num = 5552121,
          .type = foo_Person_WORK },
    };
    Person bob = {
        .name = "bob the user",
        .id = 31337,
        .email = NULL,
        .phone = phone,
        .phones = 2,
    };
    Person *ret;
    size_t len;
    uint8_t *buf;
    
    if( (buf = foo_Person_to_string(&len, &bob)) == NULL) {
        fprintf(stderr, "Error writing bob.\n");
        return 1;
    }
    ret = read_foo_Person(buf, len, l);
    free(buf);
    if(ret == NULL) {
        return 1;
    }

    printf("Read name = \"%s\"\n", ret->name);
    printf("     id = %d\n", ret->id);
    if(ret->email != NULL)
        printf("     email = \"%s\"\n", ret->email);
    printf("     phones = %d\n", ret->phones);

    allocator_dtor(&l);
    return 0;
}

void protowr_foo_Person(foo_Person *out, Person *a) {
    int i;
    out->name = a->name;
    out->len_name = strlen(a->name);
    out->id = a->id;
    if(a->email != NULL) {
        out->email = a->email;
        out->len_email = strlen(a->email);
        out->has_email = 1;
    }
    out->n_phone = a->phones;
    for(i=0; i<a->phones; i++) {
        out->phone[i] = a->phone+i;
    }
}
Person *protord_foo_Person(void *info, foo_Person *r) {
    Allocator *l = info;
    Person *a;
    size_t len = sizeof(Person);

    len += r->len_name + 1;
    if(r->has_email) len += r->len_email + 1;
    len += r->n_phone*sizeof(PhoneNumber *);
    a = allocate(l, len);

    len = sizeof(Person); // running total of used-self-space
    a->name = (void *)a + len; len += r->len_name+1;
    memcpy(a->name, r->name, r->len_name);
    a->name[r->len_name] = 0;
    a->id = r->id;
    if(r->has_email) {
        a->email = (void *)a + len; len += r->len_email+1;
        memcpy(a->email, r->email, r->len_email);
        a->email[r->len_email] = 0;
    } else {
        a->email = NULL;
    }
    a->phones = r->n_phone;
    a->phone = (void *)a + len; len += r->n_phone*sizeof(PhoneNumber *);
    memcpy(a->phone, r->phone, r->n_phone*sizeof(PhoneNumber *));

    return a;
}

void protowr_foo_Person_PhoneNumber(foo_Person_PhoneNumber *out,
                         PhoneNumber *a) {
    static char num[16];
    snprintf(num, sizeof(num), "%llu", a->num);
    out->number = num;
    out->len_number = strlen(num);
    out->type = a->type;
    out->has_type = 1;
}
PhoneNumber *protord_foo_Person_PhoneNumber(void *info,
                         foo_Person_PhoneNumber *r) {
    Allocator *l = info;
    PhoneNumber *a;
    size_t len = sizeof(PhoneNumber);
    static char num[16];

    a = allocate(l, len);
    memcpy(num, r->number, r->len_number);
    num[r->len_number] = 0;
    a->num = atoi(num);
    a->type = r->type;
    return a;
}


void protowr_foo_LookupResult(foo_LookupResult *out, Person *a) {
    out->person = a;
    out->has_person = a != NULL;
}
Person *protord_foo_LookupResult(void *info, foo_LookupResult *r) {
    if(r->has_person) return r->person;
    return NULL;
}

void protowr_foo_Name(foo_Name *out, char *a) {
    out->name = a;
    out->len_name = strlen(a);
}
char *protord_foo_Name(void *info, foo_Name *r) {
    if(r->has_name) return r->name;
    return NULL;
}


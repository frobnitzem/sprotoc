#include <stdint.h>

typedef struct {
    int len;
    char data[16];
} String;

typedef struct {
    String name;
    String history;
    String args[5];
    int n_args;
} MY_foo_Person;


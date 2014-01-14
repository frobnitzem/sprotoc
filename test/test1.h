#include <stdint.h>

#define MY_foo_Person Person
#define MY_foo_Person_PhoneNumber PhoneNumber
#define MY_foo_LookupResult Person
#define MY_foo_Name char

typedef struct PhoneNumber PhoneNumber;
typedef struct Person Person;

struct PhoneNumber {
    uint64_t num;
    int type;
};
struct Person {
    char *name;
    char *email;
    int id;
    int phones;
    PhoneNumber *phone;
};


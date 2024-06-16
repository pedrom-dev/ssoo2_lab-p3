#ifndef UTILS_H
#define UTILS_H

#include <mqueue.h>
#include <string>
#include "User.h"
#include "definitions.h"

void create_shared_memory_user(User* user, const char *shared_memory_name);
void get_shared_memory_user(User *user, const char *shared_memory_name);

#endif // UTILS_H
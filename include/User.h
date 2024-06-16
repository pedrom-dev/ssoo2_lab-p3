#ifndef USER_H
#define USER_H

#include <string>
#include "definitions.h"

class User {
public:

    int id;
    int balance;
    UserType type;
    User(int id, int balance, UserType type);
    User();

    int getId();
    void setId(int id);
    int getBalance();
    void setBalance(int balance);
    UserType getType();
    void setType(UserType type);
    void decreaseBalance();
};

#endif // USER_H
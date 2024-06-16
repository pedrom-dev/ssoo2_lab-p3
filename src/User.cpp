#include "User.h"

User::User(int id, int balance, UserType type) {
    this->id = id;
    this->balance = balance;
    this->type = type;
}

User::User() {};

int User::getId() {
    return id;
}

void User::setId(int id) {
    this->id = id;
}

int User::getBalance() {
    return balance;
}

void User::setBalance(int balance) {
    this->balance = balance;
}

void User::decreaseBalance() {
    --balance;
}

UserType User::getType() {
    return type;
}

void User::setType(UserType type) {
    this->type = type;
}

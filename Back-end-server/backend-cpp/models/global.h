#pragma once

#include <string>

struct User
{
    std::string username;
    std::string password;
    User(std::string name = "", std::string passwd = "") : username(name), password(passwd) {}
};

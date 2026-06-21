#pragma once

#include <string>

struct User
{
    int id;
    std::string username;
    std::string password_hash;
    std::string password_salt;
    std::string password_algo;
    int password_iterations;
    std::string role;
    bool is_active;

    User()
        : id(0),
          password_iterations(0),
          is_active(false)
    {
    }
};
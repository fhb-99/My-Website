#include "third_party/httplib.h"
#include "third_party/json.hpp"
#include <iostream>
#include <ctime>

using json = nlohmann::json;

int main()
{
    httplib::Server server;

    server.Get("/api/health", [](const httplib::Request& req, httplib::Response& res){
        json root;
        root["status"] = "ok";
        root["time"] = std::time(nullptr);
        res.set_content(root.dump(), "application/json; charset=utf-8");
    });

    std::cout << "Blog server running at http://0.0.0.0:8080" << std::endl;
    server.listen("0.0.0.0", 8080);

    return 0; 
}
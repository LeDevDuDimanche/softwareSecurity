#ifndef ACTIVEUSERTABLE_HPP
#define ACTIVEUSERTABLE_HPP
#include <set>
#include <string>
#include <mutex>
#include <vector>

class ActiveUserTable
{
private:
    std::mutex mtx;
    std::set<std::string> currentUsers;
public:
    ActiveUserTable();
    ~ActiveUserTable();
    void AddUser(std::string username);
    void removeUser(std::string username);
    std::string getAllLoggedInUsers();
};

#endif
#include <server/ActiveUserTable.hpp>
#include <parsing.hpp>
#include <set>
#include <mutex>
#include <string>
#include <vector>


ActiveUserTable::ActiveUserTable(/* args */)
{
    std::set<std::string> s;
    this->currentUsers = s;
}

ActiveUserTable::~ActiveUserTable()
{
}

void ActiveUserTable::AddUser(std::string username) {
    this->mtx.lock();
    this->currentUsers.insert(username);
    this->mtx.unlock();
}

void ActiveUserTable::removeUser(std::string username) {
    this->mtx.lock();
    this->currentUsers.erase(username);
    this->mtx.unlock();
}
std::string ActiveUserTable::getAllLoggedInUsers() {
    std::vector<std::string> v;
    for (std::string username: this->currentUsers) {
        v.push_back(username);
    }
    std::string temp = Parsing::join_vector(v, Parsing::new_line);
    return temp;
}

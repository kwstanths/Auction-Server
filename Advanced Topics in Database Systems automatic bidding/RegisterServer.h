#include <iostream>
#include <vector>
#include <string>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>


struct user{
      std::string name;
      int socket, interested,participate;
};

class RegisterServer{
private:
      std::vector<user> Users;

public:
      int addUser(int AuctioneerID , std::string new_user_name, int socket, int interested ,int participate);
      int removeUser(int Auctioneer_id , std::string);
      int changeUserInterest(std::string user_name);
      void clearUserInerests();
      void participate_in_current_round();
      void printUsers();
      std::vector<user> getUsers();
};

#include <iostream>
#include <vector>
#include <string>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>


struct user{
      /*
      * For each connected user hold his name, the socket that he uses for communication and the interested, participate fields
      */
      std::string name;
      int socket, interested,participate;
};

class RegisterServer{
      /*
      * This is a Server used by the auctioneers to hold the users information connected to them. Each auctioneer has its own image of a RegisterServer
      */
private:
      /*
      * The vetor Users holds the users connected at every time to the server. All changes (connects, disconnects, interests) happen in that vector
      */
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

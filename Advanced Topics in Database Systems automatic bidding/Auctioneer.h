#include <iostream>
#include <vector>
#include <string>
#include "lib.h"

#include "RegisterServer.h"
#include "DBServer.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>


class Auctioneer{
private:
      int AuctioneerID,auctioneer_socket;
      RegisterServer myRegisterServer;
      DBServer myDBServer;
      std::vector<int> otherAuctioneers;

public:
      Auctioneer(int id, int number_of_auctioneers);
      int getID();
      void printUsers();
      void broadcast_to_users(std::string message);
      void broadcast_to_interested_users(std::string message);
      void broadcast_to_auctioneers(std::string message);
      int item_auction(int item_id, std::string item_name, int initial_price, int Ltime, struct global_information *global_info);
      void Auctioneer_listen(struct global_information *global_info);
};

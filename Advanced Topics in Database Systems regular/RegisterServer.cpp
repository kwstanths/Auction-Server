#include <iostream>
#include <vector>
#include <string>

#include "RegisterServer.h"
#include "DBServer.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>


int RegisterServer::addUser(int AuctioneerID , std::string new_user_name, int socket,int interested,int participate){
      /*
      * Add the user with name new_user_name to the vector.
      */
      for (std::vector<user>::iterator it = Users.begin(); it!= Users.end(); ++it){
            if (new_user_name.compare(it->name) == 0) {
                  #ifdef DEBUG
                  std::cout << "addUser: User already exists" << std::endl;
                  #endif
                  return 0;
            }
      }
      struct user newUser;
      newUser.name = new_user_name;
      newUser.socket = socket;
      newUser.interested = interested;
      newUser.participate = participate;
      Users.push_back(newUser);


      return 1;
}

int RegisterServer::removeUser(int Auctioneer_id , std::string user_name){
      /*
      * Remove a user from the vector Users
      */
      int i=0;
      for(std::vector<user>::iterator it = Users.begin(); it != Users.end(); ++it){
            if (user_name.compare(it->name) == 0){
                  #ifdef DEBUG
                  std::cout << "Removing user: " << it->name << std::endl;
                  #endif
                  Users.erase(Users.begin()+i);
                  return 1;
            }
	    i++;
      }
      return -1;
}

void RegisterServer::participate_in_current_round(){
      /*
      * The participate filed holds the information needed in order to halde user connects during the asking if interested section
      */
	 for(std::vector<user>::iterator it = Users.begin(); it != Users.end(); ++it){
		if (it->participate == 0)
			it->participate = 1;

	}
}


int RegisterServer::changeUserInterest(std::string user_name){
      /*
      * Make the user user_name interested
      */
      for (std::vector<user>::iterator it = Users.begin(); it!= Users.end(); ++it){
            if (it->name.compare(user_name) == 0) it->interested = 1;
      }
}

void RegisterServer::clearUserInerests(){
      /*
      * Make all usernames to 0
      */
      for (std::vector<user>::iterator it = Users.begin(); it!= Users.end(); ++it){
            it->interested = 0;
      }
}

void RegisterServer::printUsers(){
      /*
      * Print your users
      */
      std::cout << "User\tSocket\tInterested" << std::endl;
      for (std::vector<user>::iterator it = Users.begin(); it!= Users.end(); ++it){
            std::cout << it->name << "\t" << it->socket << "\t";
            if (it->interested) std::cout << "yes" << std::endl;
            else std::cout << "no" << std::endl;
      }
}

std::vector<user> RegisterServer::getUsers(){
      /*
      * Return the users vector. This is needed for the communication, given that the Users vector is private
      */
      return Users;
}

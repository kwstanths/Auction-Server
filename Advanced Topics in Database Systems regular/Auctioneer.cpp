#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <functional>
#include <thread>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <utility>
#include <sys/time.h>
#include <fstream>

#include "Auctioneer.h"
#include "defines.h"
#include "lib.h"
#include "DBServer.h"
#include "LogServer.hpp"


#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#define Y 14
#define y 15

using namespace std;

Auctioneer::Auctioneer(int id, int number_of_auctioneers){
	/*
	 * Initialize an auctioneer. myRegisterServer holds auctioneer's users
	 * otherAuctioneer's holds the id's of the other auctioneer's
	 */
	AuctioneerID = id;
	RegisterServer myRegisterServer;

	for (int i = 0; i < number_of_auctioneers; i++)
		if (i != id)
			otherAuctioneers.push_back(i);
}

int Auctioneer::getID(){
	/*
	* Return Auctioneer's ID
	*/
	return AuctioneerID;
}

void Auctioneer::printUsers(){
	/*
	* Print the users that this auctioneer is responsible for. Why not use the RegisterServer's function ?
	*/
	myRegisterServer.printUsers();
}

void Auctioneer::broadcast_to_users(std::string message){
	/*
	 * Send the message message to all your users
	 */
	std::vector<user> Users = myRegisterServer.getUsers();
	for(std::vector<user>::iterator it=Users.begin(); it != Users.end(); ++it)
		if (it->participate == 1)
			if (write(it->socket,message.c_str(),message.length()) != message.length())
				perror("write");

}

void Auctioneer::broadcast_to_interested_users(std::string message){
	/*
	 * Send the message message to all interested ones only
	 */
	std::vector<user> Users = myRegisterServer.getUsers();
	for(std::vector<user>::iterator it=Users.begin(); it != Users.end(); ++it)
		if(it->interested == 1 && it->participate == 1)
			if (write(it->socket,message.c_str(),message.length()) != message.length())
				perror("write");
}

void Auctioneer::broadcast_to_auctioneers(std::string message){
	/*
	 * Send the message message to all auctioneers
	 */
	std::string hostname="localhost";
	for(int i=0; i<otherAuctioneers.size(); i++)
		forward_message(message.c_str(),hostname.c_str(), otherAuctioneers.at(i)+PORT_BASE);

}

void Auctioneer::Auctioneer_listen(struct global_information *global_info){
	/*
	 * This is the fucntion each thread executes. The magic start here
	 */
	fd_set readfds;
	int newsd,max_socket,stop=0;
	struct sockaddr_in sa;
	socklen_t len = sizeof(struct sockaddr_in);
	char addrstr[INET_ADDRSTRLEN];
	ssize_t ret;
	char buffer[700];
	command my_command;

	/*
	* Get your Log server instance
	*/
#ifdef KEEP_LOG
	LogServer *log = LogServer::getInstance("log_file.txt");
#endif
	/*
	* Initialize your well known socket
	*/
	if ((auctioneer_socket=socket(PF_INET, SOCK_STREAM,0)) < 0){
		perror("socket");
		return ;
	}
	/*
	* The flag SO_REUSEADDR is very usefull for quick use of the same ports
	*/
	int yes=1;
	if (setsockopt(auctioneer_socket,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int))==-1)
		perror("setsockopt");

again:
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(PORT_BASE+AuctioneerID);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(auctioneer_socket, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		usleep(800000);
		perror("bind");
		goto again;
	}

	if (listen(auctioneer_socket, TCP_BACKLOG) < 0){
		perror("listen");
		return ;
	}
	std::cout << "Hello from Auctioneer: " << AuctioneerID << "! Listening to port " << PORT_BASE + AuctioneerID << std::endl;

#ifdef KEEP_LOG
      log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> Waiting connections");
#endif



#ifdef KEEP_DB
	myDBServer.create_connection();
	myDBServer.create_register_table(AuctioneerID);
#endif


	while(1){

		if (stop == 1)
			break;
		/*
		* Create a set from all your users as well as your well known socket
		*/
		std::vector<user> Users = myRegisterServer.getUsers();
		FD_ZERO(&readfds);
		FD_SET(auctioneer_socket, &readfds);
		max_socket = auctioneer_socket;
		for(std::vector<user>::iterator it=Users.begin(); it!=Users.end(); ++it){
			FD_SET(it->socket, &readfds);
			if (it->socket > max_socket) max_socket = it->socket;
		}
		/*
		* Wait for anything to happen in those sockets
		*/
		select(max_socket+1,&readfds,NULL,NULL,NULL);
		/*
		* Depending on the socket hanle the message
		*/

		if (FD_ISSET(auctioneer_socket, &readfds)) {
			/*
			* Message on the well known socket (driver maybe for new auction? :O or maybe a new user :O)
			*/
			if ((newsd = accept(auctioneer_socket, (struct sockaddr *)&sa, &len)) < 0){
				perror("accept");
				return ;
			}

			if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr))) {
				perror("could not format client's IP address");
				return ;
			}

			ret = read(newsd,buffer,sizeof(buffer)-1);
			if (buffer[ret-1]=='\n') buffer[ret-1] = '\0';
			else buffer[ret]='\0';
#ifdef DEBUG
			std::cout << "Auctioneer: " << AuctioneerID << " [ " << buffer << " ] " << std::endl;
#endif
			/*
			* Parse the command
			*/
			my_command = parse_command(buffer,0);
			if(my_command.type == -1) {
				/*
				* Please dont send your junk
				*/
				if(close(newsd)){
					perror("close");
				}
				continue;
			}

			switch (my_command.type) {
				case auct_connect:
					if (my_command.buffer.empty()) {
						/*
						* auct_connect with no name? please....
						*/
						if(close(newsd)){
							perror("close");
						}
						break;
					}

#ifdef DEBUG
					std::cout << "Auctionneer: " << AuctioneerID << " connect request from user " << my_command.buffer << std::endl;
#endif
					/*
					* If everything is ok then register your new user!
					*/
					if (!myRegisterServer.addUser(AuctioneerID,my_command.buffer,newsd,0,1)) {
						if (write(newsd,"User already exists",19) != 19)
							perror("write");
						if(close(newsd)){
							perror("close");
						}
					}
#ifdef KEEP_LOG
                              log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+my_command.buffer+" connected");
#endif
					/*
					* Register him everywhere
					*/
#ifdef KEEP_DB
					myDBServer.insert_register_table(AuctioneerID,my_command.buffer);
#endif

					myRegisterServer.printUsers();
					break;

				case auct_start_new_item:
					/*
					* father sent us new item to auction!
					*/

#ifdef DEBUG
					std::cout << "Auctionneer: " << AuctioneerID << " new item auction for item: " << my_command.item_id << " Name: " << my_command.buffer << " starting at: " << my_command.initial_price << std::endl;
#endif
#ifdef KEEP_LOG
                              log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> New item auction: '"+my_command.buffer+"' starting at: "+to_string(my_command.initial_price));
#endif
					if(close(newsd)){
						perror("close");
					}
					/*
					* In the item auction we do the auction. All auctioneers go there.. lets say "auction mode" begin
					*/
					if(item_auction(my_command.item_id,my_command.buffer,my_command.initial_price,25,global_info)){
						myRegisterServer.participate_in_current_round();
					}
#ifdef KEEP_LOG
                              log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> Item auction ended");
#endif
					break;
				case auct_stop:
					/*
					* Just break;
					*/
					stop = 1;
					break;
			}
		}
		else {
			/*
			* If the auctioneer_socket was no set then it is gotta be some of my users
			* Check them one by one
			*/
			std::vector<std::string> users_exit_vector;
			Users = myRegisterServer.getUsers();
			for(std::vector<user>::iterator it = Users.begin(); it!=Users.end(); ++it) {
				if (FD_ISSET(it->socket, &readfds)){
					ret = read(it->socket,buffer,sizeof(buffer)-1);
					if (ret == 0) {
						std::cout << "User: " << it->name << " disconnected abnormally... :(" << std::endl;
						if(close(it->socket)){
							perror("close");
						}
						users_exit_vector.push_back(it->name);
#ifdef KEEP_LOG
                                    log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+it->name+" disconnected abnormally");
#endif
					}
					if (buffer[ret-1]=='\n') buffer[ret-1] = '\0';
					else buffer[ret]='\0';
#ifdef DEBUG
					std::cout << "Auctioneer: " << AuctioneerID << " [ " << buffer << " ] " << std::endl;
#endif

					my_command = parse_command(buffer,0);
					if(my_command.type == -1) {
						if(close(newsd)){
							perror("close");
						}
					}
					else if (my_command.type == quit) {
#ifdef KEEP_LOG
                              log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+it->name+" disconnected");
#endif
						users_exit_vector.push_back(it->name);
						if(close(it->socket)){
							perror("close");
						}

					}
				}
			}

			/*
			* Bug fix: Cant delete from a vector when we are looping in that vector!!!
			*/
			for(std::vector<std::string>::iterator it =users_exit_vector.begin(); it!= users_exit_vector.end(); ++it){
				if (myRegisterServer.removeUser(AuctioneerID,*it) == -1) {
#ifdef DEBUG
					cout << "Unable to delete user" << endl;
#endif
				};
#ifdef KEEP_DB
				myDBServer.delete_register_table(AuctioneerID,*it);
#endif
			}
		}
	}
#ifdef KEEP_LOG
      log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> Auction ended. Returning");
#endif

}

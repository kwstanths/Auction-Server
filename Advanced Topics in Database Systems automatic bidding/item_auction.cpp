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


int Auctioneer::item_auction(int item_id, std::string item_name, int initial_price, int Ltime,struct global_information *global_info){
	/*
	 * Start a new auction. Here are implemented most of the auctioneer's procedures
	 */

	fd_set readfds;
	int newsd,max_socket,ret,is_someone_interested,someone_bidded, current_bid, no_bid_number, current_winner_auct_id;
	std::string message,current_winner;
	struct timeval timeout;
	struct sockaddr_in sa;
	char addrstr[INET_ADDRSTRLEN];
	socklen_t len = sizeof(struct sockaddr_in);
	std::vector<user> Users ;
	char buffer[700];
	command my_command;

#ifdef KEEP_LOG
	LogServer *log  =LogServer::getInstance("log_file.txt");
#endif
	//Ask the users if they are interested. Ltime seconds timeout
	timeout.tv_sec = Ltime;
	timeout.tv_usec = 0;
#ifdef KEEP_LOG
	log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> Starting the 'asking the users if they are interested' section");
#endif
	message = "auct_new_item|" + std::to_string(item_id) + "|" + item_name + "|" + std::to_string(initial_price) + "\n";
	broadcast_to_users(message);

	global_info->is_someone_interested=0;
	while(1){
		FD_ZERO(&readfds);
		FD_SET(auctioneer_socket, &readfds);
		max_socket = auctioneer_socket;
		Users = myRegisterServer.getUsers();
		for(std::vector<user>::iterator it=Users.begin(); it!=Users.end(); ++it){
			FD_SET(it->socket, &readfds);
			if (it->socket > max_socket) max_socket = it->socket;
		}
		select(max_socket+1,&readfds,NULL,NULL,&timeout);
		if (FD_ISSET(auctioneer_socket, &readfds)){
			if ((newsd = accept(auctioneer_socket, (struct sockaddr *)&sa, &len)) < 0){
				perror("accept");
			}

			if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr))) {
				perror("could not format client's IP address");
			}

			ret = read(newsd,buffer,sizeof(buffer)-1);
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
				continue;
			}

			switch (my_command.type) {
				case auct_connect:
					if (my_command.buffer.empty()) {
						if(close(newsd)){
							perror("close");
						}
						break;
					}

#ifdef DEBUG
					std::cout << "Auctionneer: " << AuctioneerID << " connect request from user " << my_command.buffer << std::endl;
#endif
					if (!myRegisterServer.addUser(AuctioneerID,my_command.buffer,newsd,0,0)) {
						if (write(newsd,"User already exists",19) != 19)
							perror("write");
						if(close(newsd)){
							perror("close");
						}
					}
#ifdef KEEP_LOG
					log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+my_command.buffer+" connected");
#endif
#ifdef KEEP_DB
					myDBServer.insert_register_table(AuctioneerID,my_command.buffer);
#endif
			}

		}else if(timeout.tv_sec == 0){
			break;
		}else{

			std::vector<std::string> users_exit_vector;
			Users = myRegisterServer.getUsers();
			for(std::vector<user>::iterator it = Users.begin(); it!=Users.end(); ++it){
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
					my_command = parse_command(buffer,0);
					if ( (my_command.type == auct_i_am_interested) && (my_command.item_id == item_id)){

						myRegisterServer.changeUserInterest(it->name);
						global_info->is_someone_interested=1;
#ifdef KEEP_LOG
						log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+it->name+" is interested");
#endif
					}
					else if (my_command.type == quit) {
						users_exit_vector.push_back(it->name);
						if(close(it->socket)){
							perror("close");
						}
#ifdef KEEP_LOG
						log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+it->name+" disconnected");
#endif
					}

				}

			}

			for(std::vector<std::string>::iterator it =users_exit_vector.begin(); it!= users_exit_vector.end(); ++it){
				myRegisterServer.removeUser(AuctioneerID,*it);
#ifdef KEEP_DB
				myDBServer.delete_register_table(AuctioneerID,*it);
#endif
			}
		}
	}
#ifdef KEEP_LOG
	log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> The 'asking the users if they are interested' section ended");
#endif
	if (global_info->is_someone_interested == 0){
#ifdef DEBUG
		std::cout << "No one is interested for this item :(" << std::endl;
#endif
	}
	//Lets sleep 300ms in case of out of order late messages from bidders.
	//Beacuse an auctioneer might just change the global info and others auctioneers might not see it and leave
	usleep(300000);

	if ( global_info->is_someone_interested != 0) {

#ifdef KEEP_LOG
		log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> Statring the 'bidding' section with interested users");
#endif
		//Start bidding procedure with interested users
		myRegisterServer.printUsers();
		message="auct_start_bidding";
		broadcast_to_interested_users(message);

		current_winner="";
		current_winner_auct_id=-1;
		timeout.tv_sec = Ltime;
		timeout.tv_usec = 0;
		someone_bidded=0;
		no_bid_number=0;
		current_bid=initial_price;
		while(1){
			FD_ZERO(&readfds);
			FD_SET(auctioneer_socket, &readfds);
			max_socket = auctioneer_socket;
			Users = myRegisterServer.getUsers();
			for(std::vector<user>::iterator it=Users.begin(); it!=Users.end(); ++it){
				if (it->interested==1){
					FD_SET(it->socket, &readfds);
					if (it->socket > max_socket) max_socket = it->socket;
				}
			}
			select(max_socket+1,&readfds,NULL,NULL,&timeout);
			if (FD_ISSET(auctioneer_socket, &readfds)){
				if ((newsd = accept(auctioneer_socket, (struct sockaddr *)&sa, &len)) < 0){
					perror("accept");
				}

				if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr))) {
					perror("could not format client's IP address");
				}

				ret = read(newsd,buffer,sizeof(buffer)-1);
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
					continue;
				}

				switch (my_command.type) {
					case auct_connect:
						if (my_command.buffer.empty()) {
							if(close(newsd)){
								perror("close");
							}
							break;
						}

#ifdef DEBUG
						std::cout << "Auctionneer: " << AuctioneerID << " connect request from user " << my_command.buffer << std::endl;
#endif
						if (!myRegisterServer.addUser(AuctioneerID,my_command.buffer,newsd,0,0)) {
							if (write(newsd,"User already exists",19) != 19)
								perror("write");
							if(close(newsd)){
								perror("close");
							}
						}
#ifdef KEEP_LOG
						log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+my_command.buffer+" connected");
#endif
#ifdef KEEP_DB
						myDBServer.insert_register_table(AuctioneerID,my_command.buffer);
#endif

						break;
					case auct_new_high_bid:
#ifdef KEEP_LOG
						log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> New high bid: "+to_string(my_command.high_bid));
#endif
						current_winner=my_command.winner_name;
						current_winner_auct_id=my_command.winner_auct_id;
						broadcast_to_interested_users(buffer);
						current_bid = my_command.high_bid;
						timeout.tv_sec = Ltime;
						timeout.tv_usec = 0;
						someone_bidded=1;
						break;
				}
			}else if(timeout.tv_sec == 0){
#ifdef DEBUG
				cout <<"\n" << AuctioneerID <<"  Time passed !" << endl;
#endif
				if (someone_bidded == 0) {
					no_bid_number++;
					if (no_bid_number == 5){
#ifdef DEBUG
						std::cout << "No one bidded five times :(" << std::endl;
#endif
#ifdef KEEP_LOG
						log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> Five times with no bid");
#endif
						break;
					}else{
						current_bid = 0.9 * current_bid;
						message="auct_new_high_bid|" + std::to_string(current_bid);
						broadcast_to_interested_users(message);
						timeout.tv_sec = Ltime;
						timeout.tv_usec = 0;
#ifdef KEEP_LOG
						log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> Lower the price to:"+to_string(current_bid));
#endif
					}
				}else{
#ifdef DEBUG
					std::cout << "Auctionneer: " << AuctioneerID << " end of auction. Winner: " << current_winner << std::endl;
#endif
					break;
				}
			}else{


				std::vector<std::string> users_exit_vector;

				for(std::vector<user>::iterator it =Users.begin(); it!= Users.end(); ++it){
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
						my_command = parse_command(buffer,0);
						if (my_command.type == bid){
							if(it->interested==1){
								if (my_command.new_bid > current_bid) {
									current_winner=it->name;
									current_winner_auct_id=AuctioneerID;
									timeout.tv_sec = Ltime;
									timeout.tv_usec = 0;
									someone_bidded=1;
									current_bid = my_command.new_bid;
									message="auct_new_high_bid|" + std::to_string(my_command.new_bid)+"|"+it->name+"|"+std::to_string(AuctioneerID);
#ifdef KEEP_LOG
									log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+it->name+" bidded: "+to_string(my_command.new_bid));
#endif
									broadcast_to_auctioneers(message);
									broadcast_to_interested_users(message);
								}
							}
						}
						else if (my_command.type == quit){
							users_exit_vector.push_back(it->name);
							if(close(it->socket)){
								perror("close");
							}
#ifdef KEEP_LOG
							log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> User: "+it->name+" disconnected");
#endif

						}
					}

				}

				for(std::vector<std::string>::iterator it =users_exit_vector.begin(); it!= users_exit_vector.end(); ++it){
					myRegisterServer.removeUser(AuctioneerID,*it);
#ifdef KEEP_DB
					myDBServer.delete_register_table(AuctioneerID,*it);
#endif
				}



			}
		}


		if (someone_bidded==0){
			if (AuctioneerID == 0)
				log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> No one bidded for: "+item_name);
		}else {
			message="stop_bidding|"+current_winner+"|"+std::to_string(current_bid)+"|"+std::to_string(current_winner_auct_id);
			broadcast_to_users(message);
			if (current_winner_auct_id == AuctioneerID){
				log->append("Item (" + item_name + ") winner: " + current_winner + " at " + std::to_string(current_bid) + " from server: " + std::to_string(current_winner_auct_id));
			}
		}

	}else {
		if (AuctioneerID == 0)
			log->append("[Auctioneer "+to_string(AuctioneerID)+"] -> No one interested for: " + item_name);
	}

	myRegisterServer.clearUserInerests();
	return 1;
}

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
#include <ctime>
#include <boost/tokenizer.hpp>
#include <netdb.h>

#include "lib.h"
#include "defines.h"
#include "DBServer.h"

#define Y 14
#define y 15


command parse_command(std::string initial_message, int read_from_file ){
	/*
	 * Parse the buffer initial_message and a return a struct which holds the message info for each command
	 */
	int i=0;
	command my_command;

	my_command.type = -1;
	boost::char_separator<char> sep("|");
	boost::tokenizer<boost::char_separator<char>> tokens(initial_message,sep);

	for(const auto& t : tokens){
	  if (!read_from_file) {
		if(i==0){
			if (t.compare("auct_connect") == 0) my_command.type = auct_connect;
			if (t.compare("auct_start_new_item") == 0) my_command.type = auct_start_new_item;
			if (t.compare("auct_new_item") == 0) my_command.type = auct_new_item;
			if (t.compare("auct_i_am_interested") == 0) my_command.type = auct_i_am_interested;
			if (t.compare("auct_start_bidding") == 0) my_command.type = auct_start_bidding;
			if (t.compare("auct_new_high_bid") == 0) my_command.type = auct_new_high_bid;
			if (t.compare("bid") == 0) my_command.type = bid;
			if (t.compare("my_bid") == 0) my_command.type = my_bid;
			if (t.compare("stop_bidding") == 0) my_command.type = stop_bidding;
			if (t.compare("list_high_bid") == 0) my_command.type = list_high_bid;
			if (t.compare("list_description") == 0) my_command.type = list_description;
			if (t.compare("Y") == 0) my_command.type = Y;
			if (t.compare("y") == 0) my_command.type = y;
			if (t.compare("quit") == 0) my_command.type = quit;
			if (t.compare("auct_stop") == 0) my_command.type = auct_stop;
		}
		if(my_command.type == auct_connect){
			if (i==1) my_command.buffer = t;
		}else if(my_command.type == auct_start_new_item){
			if (i==1) my_command.item_id = std::stoi(t); // stoi = string to integer
			if (i==2) my_command.buffer = t;
			if (i==3) my_command.initial_price = std::stoi(t);
		}else if (my_command.type == auct_new_item) {
			if (i==1) my_command.item_id = std::stoi(t);
			if (i==2) my_command.buffer = t;
			if (i==3) my_command.initial_price = std::stoi(t);
		}else if(my_command.type == auct_i_am_interested){
			if (i==1) my_command.item_id = std::stoi(t);
		}else if (my_command.type ==auct_new_high_bid) {
			if (i==1) my_command.high_bid = std::stoi(t);
			if (i==2) my_command.winner_name = t;
			if (i==3) my_command.winner_auct_id = std::stoi(t);
		}else if (my_command.type == bid){
			if (i==1) my_command.new_bid = std::stoi(t);
		}
		else if (my_command.type == my_bid){
			if (i==1) my_command.new_bid = std::stoi(t);
		}else if (my_command.type == stop_bidding){
			if (i==1) my_command.winner_name = t;
			if (i==2) my_command.winner_bid = std::stoi(t);
			if (i==3) my_command.winner_auct_id = std::stoi(t);
		}else if (my_command.type == quit){
			if (i==1) my_command.name = t;
		}

		i++;
	 }
	 else {
			if (i==1) my_command.buffer = t;
			if (i==0) my_command.initial_price = std::stoi(t);
		i++;
	 }
	}

	return my_command;
}

void forward_message(const char *message ,const char * hostname, int port){
	/*
	 * Send the message message to hostname:port
	 */
	int sd;
	struct hostent *hp;
	struct sockaddr_in sa;
	if (hostname == NULL){
#ifdef DEBUG
		std::cout << "forward_message: Please specify hostname" << std::endl;
#endif
		return;
	}

	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	if ( !(hp = gethostbyname(hostname))) {
#ifdef DEBUG
		std::cout << "forward_message: DNS lookup failed for host: " << hostname << std::endl;
#endif
		return;
	}

again:
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
	if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		usleep(200000);
		//perror("connect");
		goto again;
	}

	if (write(sd,message,strlen(message)) != strlen(message))
		perror("write");

	if (close(sd) < 0)
		perror("close");

};

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <sys/signal.h>
#include <fstream>
#include <boost/tokenizer.hpp>

#include "defines.h"
#include "lib.h"
#include "DBServer.h"
#include "colormod.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>


#define Y 14
#define y 15

using namespace std;

int randomNumberBetween(int min,int max){
	return rand()%(max-min +1) + min;
}

pair <int,int> parseConfigFile(string fileName){
	ifstream ReadFile;
	string line;
	int maxfrequency,maxbidmultiplier;
	typedef boost::tokenizer<boost::char_separator<char>> tokenizer;

	ReadFile.open(fileName);
	if (ReadFile.is_open()){
		while (!ReadFile.eof()){
			ReadFile >> line;
			if (line=="") break;
			boost::char_separator<char> sep("=");
			tokenizer tokens(line,sep);
			tokenizer::iterator tokens_iterator=tokens.begin();
			if (tokens_iterator->compare("maxfrequency") == 0){
				tokens_iterator++;
				maxfrequency=stoi(*tokens_iterator);
			}else if (tokens_iterator->compare("maxbidmultiplier") == 0){
				tokens_iterator++;
				maxbidmultiplier=stoi(*tokens_iterator);
			}
			line.clear();
		}
	}
	return make_pair(maxfrequency,maxbidmultiplier);
}

int main(int argc, char ** argv){

	int sd , current_high_bid , ret , disconnected=0 , allow_bidding , my_last_bid ;
	int new_bid, allow_automatic_bidding, bid_made_by_me, current_winning_auct_id;
	struct hostent *hp;
	struct sockaddr_in sa;
	fd_set readfds;

	std::string current_item_name;
	int current_item_id , current_item_initial_price;

	string bidder_name;
	int serverID;

	pair<int,int> configuration;
	struct timeval timeout;

#ifdef KEEP_DB
	DBServer myDBServer;
	myDBServer.create_connection();
	myDBServer.create_bidder_table();
#endif 


	Color::Modifier blue(Color::BG_BLUE);
	Color::Modifier green_bg(Color::BG_GREEN);
	Color::Modifier green(Color::FG_GREEN);
	Color::Modifier red(Color::FG_RED);
	Color::Modifier red_bg(Color::BG_RED);
	Color::Modifier def_bg(Color::BG_DEFAULT);
	Color::Modifier def(Color::FG_DEFAULT);

	/*
	   cout << "\t\tWelcome to the auction!!!" << endl;
	   cout << "You need to provide us with some information..." << endl;
	   cout << "First things first... What is your name? " ;
	   getline(cin,bidder_name);
	   cout << "Great! Now, choose the server you want to connect to! Available servers: 0,1,2: ";
	   cin >> serverID;
	   if ((serverID != 0)&&(serverID != 1)&&(serverID != 2)) {
	   cout << "There is no such server available." << endl;
	   return -1;
	   }
	   cout << "We are all set up! You are " << bidder_name << " and you will connect to " << PORT_BASE + serverID << "!" <<endl;
#ifdef DEBUG
cout << bidder_name << ":" << PORT_BASE + serverID << endl;
#endif


cout << "\n ____________________________________" << endl;
cout << "|  COMMANDS YOU CAN USE :            |"<< endl;
cout << "|....................................|"<< endl;
cout << "|1) bid|number <enter>               |"<<endl;
cout << "|2) list_high_bid <enter>            |"<<endl;
cout << "|3) list_description <enter>         |"<<endl;
cout << "|4) y or Y <enter>                   |"<<endl;
cout << "|5) quit <enter>                     |"<<endl;
cout << "|....................................|"<<endl;
cout << "|Appropriate messages will direct you|"<<endl;
cout<<  "|about how and when you can use them.|"<<endl;
cout<<  "|You can quit at any time you want   |"<<endl;
cout<<  "|____________________________________|"<<endl;
	 */

	if (argc < 3) {
		cout << "Please provide the user name and the auctioneer ID(0 or 1 or 2)" << endl;
		cout <<"Example: ./bidder mpamphs 0" << endl;
		return -1;
	}
	bidder_name = argv[1];
	serverID = atoi(argv[2]);

	if ((serverID != 0)&&(serverID != 1)&&(serverID != 2)) {
		cout << "There is no such server available." << endl;
		return -1;
	}
	configuration=parseConfigFile("config.cfg");

	int maxfrequency=configuration.first;
	int maxbidmultiplier=configuration.second;
	int maxbidprice;
#ifdef DEBUG
	cout << bidder_name << ":" << PORT_BASE + serverID << endl;
#endif

	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}



	string hostname;
	hostname = "localhost";
	int port = PORT_BASE + serverID;

	if ( !(hp = gethostbyname(hostname.c_str()))) {
#ifdef DEBUG
		cout << "forward_message: DNS lookup failed for host: " << hostname << endl;
#endif
		return -1;
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

	/*
	 * Connect to the server you were told
	 */
	string message;
	message = "auct_connect|" + bidder_name;
#ifdef DEBUG
	cout << message << " to " << PORT_BASE + serverID << endl;
#endif

	if (write(sd,message.c_str(),strlen(message.c_str())) != strlen(message.c_str()))
		perror("write");



	char buffer[700];
	command my_command;

	timeout.tv_sec = randomNumberBetween(1,maxfrequency);
	timeout.tv_usec = 0;
	//timeout.tv_sec = 2;
	current_winning_auct_id=200;
	cout.flush();
	while(1){

		if (disconnected == 1)
			break;

		FD_ZERO(&readfds);
		FD_SET(sd,&readfds);
		FD_SET(0,&readfds);


		ret = select(sd+1,&readfds,NULL,NULL,&timeout);
		if (ret < 0) {
			fprintf(stderr, "Error furing select\n");
			exit(1);
		}

		if (FD_ISSET(sd,&readfds)) {

			ret  = read(sd,buffer,sizeof(buffer));
			if (buffer[ret-1]=='\n') buffer[ret-1] = '\0';
			else buffer[ret]='\0';

			if (ret == 0){
				cout << "Server closed connection..." << endl;
				return -1;
			}

			if (ret < 0) {
				fprintf(stderr,"error during read return value on socket fd\n");
				continue;
			}

#ifdef DEBUG
			cout << buffer <<endl;
#endif


			my_command = parse_command(buffer,0);
			switch (my_command.type) {

				case auct_stop:
					cout << "\nAuction Finished" << endl;
					disconnected =1;
					break;

				case auct_new_item :
					cout.flush();

					cout <<"--->New item auction\n " <<red <<"Are you interested in a " <<" " << my_command.buffer.c_str() << " starting at " << " " << my_command.initial_price << " ?"  << def << endl ;
					cout << "Press y or Y if you are interested" << endl;
					current_high_bid = my_command.initial_price;
					current_item_name = my_command.buffer.c_str();
					current_item_id   = my_command.item_id;
					current_item_initial_price = my_command.initial_price;

					maxbidprice=(randomNumberBetween(1,maxbidmultiplier)+randomNumberBetween(1,10)*0.1)*current_item_initial_price;
					//maxbidprice=maxbidmultiplier*current_item_initial_price;
					cout << "My max bid: " << maxbidprice << endl;

					break;
				case auct_start_bidding :

					allow_bidding = 1;
					allow_automatic_bidding = 1;

					timeout.tv_sec = randomNumberBetween(1,maxfrequency);
					timeout.tv_usec = 0;
					//timeout.tv_sec = 2;

					cout << "\n--->Bidding has started you can make your bid with bid|your_bid <enter> command at any time you want" << endl;
					cout << "\n--->You can use commands 2) , 3) also at any time you want" << endl;
					break;

				case auct_new_high_bid :

					timeout.tv_sec = randomNumberBetween(1,maxfrequency);
					timeout.tv_usec = 0;
					//timeout.tv_sec = 2;
					if ((my_command.high_bid > current_high_bid)||(my_command.high_bid == current_high_bid && my_command.winner_auct_id < current_winning_auct_id))
					current_high_bid = my_command.high_bid;
					current_winning_auct_id = my_command.winner_auct_id;
					if ((my_command.winner_name).compare(bidder_name) || my_command.winner_auct_id != serverID) bid_made_by_me=0;
					cout << blue << "New current high bid: " << current_high_bid << def_bg << endl;
					break;

				case stop_bidding :

					allow_bidding = 0;
					allow_automatic_bidding = 0;
					if (my_command.winner_name==bidder_name  && my_command.winner_bid==my_last_bid && my_command.winner_auct_id==serverID)
						cout << "\n--->" << green_bg << "CONGRATULATIONS YOU ARE THE WINNER" << def_bg << endl;
					else {

						cout << "--->WINNER IS ->" << my_command.winner_name << endl;
						cout <<	"--->WHO MAKES THE HIGHEST BID ->" << my_command.winner_bid << endl;
						cout <<	"--->AND BINDED IN AUCTIONEER SERVER ->" <<my_command.winner_auct_id <<endl;
					}


					break;
			}

		}

		else if (FD_ISSET(0,&readfds)) {

			ret = read(0,buffer,sizeof(buffer));

			if (buffer[ret-1]=='\n') buffer[ret-1] = '\0';
			else buffer[ret]='\0';



			if (ret < 0) {
				fprintf(stderr,"Error during read from stdin\n");
				continue;
			}




			my_command = parse_command(buffer,0);
			switch (my_command.type) {

				case bid :

#ifdef KEEP_DB 
			myDBServer.insert_bidder_table(bidder_name,current_item_id ,my_command.new_bid,serverID);
#endif
					if (allow_bidding == 1) {
						if (my_command.new_bid > current_high_bid) {
							current_high_bid = my_command.new_bid;
							my_last_bid = my_command.new_bid;
							message = "bid|" + std::to_string(my_command.new_bid);
							if (write(sd,message.c_str(),strlen(message.c_str())) != strlen(message.c_str()))
								perror("write");
						}
						else {
							cout << "--->Your bid is lower than the current high "<< endl;
							//message = "my_bid|" + std::to_string(my_command.new_bid);
							//if (write(sd,message.c_str(),strlen(message.c_str())) != strlen(message.c_str()))
							//perror("write");
						}
					}
					else
						cout << "\n--->You can't bid for the current item" << endl;
					break;

				case list_high_bid :

					cout << "\n--->Current high bid is" << " " << current_high_bid << endl;
					break;
				case list_description:

					cout << "\n--->Current item name = " << " " << current_item_name << endl;
					cout << "\n--->Current item id = " << " " << current_item_id << endl;
					cout << "\n--->Current item initial price = " << " " << current_item_initial_price << endl;

					break;
				case quit:

					if (my_last_bid == current_high_bid && allow_bidding == 1){
						cout << red_bg << "CAUTION" << def_bg << endl;
						cout <<red<< "You have the current high we assume you will be the winner if your bid remains the highest dispite of you are disconnected." << def << endl;
					}
					else
						cout << "You are disconnected: "<< bidder_name << endl;

					message = "quit|" + bidder_name;
					if (write(sd,message.c_str(),strlen(message.c_str())) != strlen(message.c_str()))
						perror("write");


					disconnected = 1;




					break;

				case Y:

					message = "auct_i_am_interested|" + to_string(current_item_id);
					if (write(sd,message.c_str(),strlen(message.c_str())) != strlen(message.c_str()))
						perror("write");
					break;

				case y:

					message = "auct_i_am_interested|" + to_string(current_item_id);
					if (write(sd,message.c_str(),strlen(message.c_str())) != strlen(message.c_str()))
						perror("write");
					break;

			}

		}else if (timeout.tv_sec == 0){
			timeout.tv_sec = randomNumberBetween(1,maxfrequency);
			timeout.tv_usec = 0;
			if (bid_made_by_me == 1) continue;
			new_bid=(randomNumberBetween(1,3)*0.1+1)*current_high_bid;
			//new_bid=1.1*current_high_bid;
			if (new_bid > maxbidprice) new_bid = maxbidprice;
			if (allow_automatic_bidding == 1 && new_bid > current_high_bid){
				my_last_bid = new_bid;
				bid_made_by_me=1;
				cout << "I will bid: " << new_bid << endl;
#ifdef KEEP_DB 
			myDBServer.insert_bidder_table(bidder_name,current_item_id , new_bid,serverID);
#endif
				message = "bid|" + std::to_string(new_bid);
				if (write(sd,message.c_str(),strlen(message.c_str())) != strlen(message.c_str()))
					perror("write");

			}
		}

	}

	return 0;
}

#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <list>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <fstream>

#include "defines.h"
#include "Auctioneer.h"
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

void broadcast_to_auctioneers(std::list<Auctioneer> Auctioneers, std::string message){
	/*
	 * Send the message message to all auctioneers.
	 */
	std::string host = "localhost";
	for(std::list<Auctioneer>::iterator it=Auctioneers.begin(); it != Auctioneers.end(); ++it){
		forward_message(message.c_str(),host.c_str(), PORT_BASE + it->getID());
	}
}

void print_auctioneers(std::list<Auctioneer> Auctioneers){
	/*
	 * Print the auctioneers and their users. Use the print provided by the register server
	 */
	for(std::list<Auctioneer>::iterator it=Auctioneers.begin(); it != Auctioneers.end(); ++it){
		std::cout << "Auctioneer: " << it->getID() << std::endl;
		it->printUsers(); std::cout << std::endl;
	}
};

/*
* Initializing the pointer to the log server instance
*/
#ifdef KEEP_LOG
LogServer *LogServer::instance = 0;
#endif

int main(int argc, char ** argv){
	/*
	* driver-auction start the auction. We create the auctioneers and start a thread for each one of them. Then we wait for user y's or Y's in order
	* to start new auction for a specific item
	*/
	int number_of_auctioneers=3 , num_products;
	struct global_information global_info;
	global_info.is_someone_interested = 0;
	command new_product;

#ifdef KEEP_DB
	DBServer myDBServer;
	myDBServer.create_connection();
	myDBServer.drop_tables();
#endif

#ifdef KEEP_LOG
	LogServer *log = LogServer::getInstance("log_file.txt");
#endif

	/*
	* Open  the file with the products
	*/
	std::string line;
	std::ifstream myfile ("products.txt");

	if (!myfile.is_open())
		exit(1);
	getline(myfile,line);

	/*
	 * Auctioneers holds a list of all the auctioneers. Its a list of classes
	 * For each element call the class's constructor
	 */
	std::list<Auctioneer> Auctioneers;
	for(int i=0; i<number_of_auctioneers; i++){
		Auctioneer newAuctioneer = Auctioneer(i, number_of_auctioneers);
		Auctioneers.push_back(newAuctioneer);
	}
	/*
	 * For debugging purposes
	 */
	print_auctioneers(Auctioneers);

#ifdef DEBUG
	printf("DEBUG ON\n");
#endif
	/*
	 * Create one thread of each auctioneer. Each thread executes the Auctioneer_listen of each class.
	 * The global_info struct holds information about interested users. WE DONT USE GLOBAL INFO FOR SYNCHRONIZATION
	 * Synchronizaion is achieved with messages between the auctioneers
	 */
	std::vector<std::thread> threads;
	for(std::list<Auctioneer>::iterator it=Auctioneers.begin(); it != Auctioneers.end(); ++it){
		threads.push_back(std::thread(&Auctioneer::Auctioneer_listen, it, &global_info));
	}
	usleep(200000);

	/*
	 * Wait for user to type y or Y and then send the new item to all the auctioneers.
	 */
#ifdef KEEP_LOG
	log->append("[driver] -> Created Auctioneer threads and waiting for y's");
#endif

	char buffer[100];
	std::string item_name,message;
	int item_id=0,item_initial_price,stop=0;
	while(1){
		if (stop == 1) break;
		std::cout << "Move to next item? [Y/n] : ";
		std::cin >> buffer;
		if (strcmp(buffer,"Y") == 0 || strcmp(buffer,"y") == 0) {

			if (getline(myfile,line)){
				std::cout << line << std::endl;
				item_id++;
				new_product = parse_command(line,1);
#ifdef DEBUG
				std::cout << "About to start auction for item: " << item_id << " with name: '" << new_product.buffer << "' and starting price at: " << new_product.initial_price << "$!" << std::endl;
#endif
#ifdef KEEP_LOG
                        log->append("[driver] -> Starting new auction for '"+new_product.buffer+"' starting at: " + std::to_string(new_product.initial_price));
#endif
				message = "auct_start_new_item|" + std::to_string(item_id) + "|" + new_product.buffer + "|" + std::to_string(new_product.initial_price);
				broadcast_to_auctioneers(Auctioneers,message);
			}

			else {
#ifdef KEEP_LOG
                        log->append("[driver] -> Auction finished");
#endif
				std::cout << "\nNo more products auction has finished ." << std::endl;
				message = "auct_stop";
				broadcast_to_auctioneers(Auctioneers,message);
				stop = 1;
				break;
			}
		}
		else

			cout << "\nWRONG INPUT PLEASE WRITE y OR Y " << endl;
	}

	/*
	* Wait for threads to end
	*/


	for(std::vector<std::thread>::iterator it=threads.begin(); it != threads.end(); ++ it){
		it->join();
	}

#ifdef KEEP_LOG
      log->append("[driver] -> Auctioneers returned");
      log->close();
#endif
#ifdef KEEP_DB
	myDBServer.sort_bidder_table();
#endif

	return 0;

}

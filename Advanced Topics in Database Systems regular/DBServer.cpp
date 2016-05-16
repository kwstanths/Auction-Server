#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <sstream>

#include "DBServer.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>




using namespace std;


void DBServer::create_connection (){
	/*
	 * Creates a connection to the mysql server located in the machine running the program (127.0.0.1) and uses username:root, password:root
	 * Also chooses the databse 'project_database'
	 */
	sql::Driver *driver;
	driver = get_driver_instance();
	con = driver->connect("tcp://127.0.0.1:mysql", "root", "root");
	con->setSchema("project_database");
}


void DBServer::create_register_table(int auct_id) {
	/*
	 * This fucntion is used to create the necessary users tables in the database. Call this once, or create them manually via the mysql command line
	 * We use 3 register tables in order not to destroy the illusion of a true distributed system
	 */
	sql::Statement *stmt;
	stmt = con->createStatement();
	if (auct_id == 0)
		stmt->execute("CREATE TABLE register_table_0(bidder_name VARCHAR(20))");
	if (auct_id == 1)
		stmt->execute("CREATE TABLE register_table_1(bidder_name VARCHAR(20))");
	if (auct_id == 2)
		stmt->execute("CREATE TABLE register_table_2(bidder_name VARCHAR(20))");
}


void DBServer::create_bidder_table () {
	/*
	 * This fucntion is used to create the necessary bidder tables in the database. Call this once, or create them manually via the mysql command line
	 */
	sql::Statement *stmt;
	stmt = con->createStatement();
	stmt->execute("CREATE TABLE IF NOT EXISTS bidders_table(bidder_name VARCHAR(20) , product_id INT, product_bid INT , auct_id_registered_in  INT)");

}


void DBServer::insert_register_table(int auct_id, std::string bidder_name){
	/*
	* This function is used in order to insert a new tupple to the appropriate users table
	*/
	sql::Statement *stmt;
	stmt = con->createStatement();
	ostringstream strstr;

	if (auct_id == 0)
		strstr <<"INSERT INTO register_table_0(bidder_name) VALUES ('"<< bidder_name <<"')";
	if (auct_id == 1)
		strstr <<"INSERT INTO register_table_1(bidder_name) VALUES ('"<< bidder_name <<"')";
	if (auct_id ==2)
		strstr <<"INSERT INTO register_table_2(bidder_name) VALUES ('"<< bidder_name <<"')";

	std::string str = strstr.str();
	stmt->execute(str.c_str());

}


void DBServer::insert_bidder_table(std::string bidder_name , int product_id , int product_bid , int auct_id_registered_in){
	/*
	* This function is used to in order to hold all the bids done by the bidders in one table. This destroys the illusion of a true distributed system
	*/
	sql::Statement *stmt;
	stmt = con->createStatement();
	std::string str_product_id = std::to_string(product_id) ;
	std::string str_product_bid = std::to_string(product_bid);
	std::string str_auct_id_registered_in = std::to_string(auct_id_registered_in);
	ostringstream strstr;
	strstr <<"INSERT INTO bidders_table(bidder_name,product_id,product_bid,auct_id_registered_in) VALUES ('"<< bidder_name <<"','"<<str_product_id<<"','"<<str_product_bid<<"','"<<str_auct_id_registered_in<<"')";
	std::string str = strstr.str();
	stmt->execute(str.c_str());

}


void DBServer::delete_register_table(int auct_id , std::string user_name){
	/*
	* This function is used to delete a tupple from a register table
	*/
	sql::ResultSet *res;
	sql::Statement *stmt;
	stmt = con->createStatement();
	ostringstream strstr;
	std::string column = "\"" + user_name + "\"";
	//std::string str_item_id = std::to_string(item_id);

	if (auct_id == 0) {
		strstr << "DELETE FROM register_table_0 WHERE bidder_name="<<column<<"";
	}
	if (auct_id == 1) {
		strstr << "DELETE FROM register_table_1 WHERE bidder_name="<<column<<"";
	}
	if (auct_id == 2) {
		strstr << "DELETE FROM register_table_2 WHERE bidder_name="<<column<<"";
	}

	std::string str = strstr.str();
	stmt->execute(str.c_str());
}




void  DBServer::drop_tables() {
	/*
	* This function is used in order to delete the tables from the database
	*/
	sql::Statement *stmt;
	stmt = con->createStatement();

	stmt->execute("DROP TABLE IF EXISTS bidders_table");
	stmt->execute("DROP TABLE IF EXISTS register_table_0");
	stmt->execute("DROP TABLE IF EXISTS register_table_1");
	stmt->execute("DROP TABLE IF EXISTS register_table_2");
}


void DBServer::sort_bidder_table(){
	/*
	* This function sorts the tupples in the bidders table
	*/
	sql::Statement *stmt;
	stmt = con->createStatement();

	stmt->execute("ALTER TABLE bidders_table ORDER BY product_id,product_bid DESC");
}


//DBServer::~DBServer () {
//
//	delete con;
//}

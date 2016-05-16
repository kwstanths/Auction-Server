#ifndef DATABASE_H__
#define DATABASE_H__

#include <iostream>
#include <string>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>



class DBServer {
      /*
      * This is a Server that handles all the accesses to the mysql database. Each and every one that wishes access the database uses this class's functions
      */
   private:
         /*
         * The connection to the database is initialised once in the class's constructor
         */
	sql::Connection *con;

   public:

	//~DBServer ();

	void create_connection();
	void create_register_table(int auct_id);
	void create_bidder_table();
	void insert_register_table(int auct_id, std::string bidder_name);
	void insert_bidder_table(std::string bidder_name , int product_id , int product_bid , int auct_id_registered_in);
	void delete_register_table(int auct_id , std::string user_name);
	void drop_tables();
	void sort_bidder_table();

};

#endif


//---------

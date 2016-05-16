#ifndef LIB_H__
#define LIB_H__
#include <fstream>

struct command {
      /*
      * Holds the necessary info for each command. The commands can be found at the defines.h file
      */
      int type;
      std::string buffer, winner_name,name;
      int initial_price, item_id,high_bid,new_bid, winner_bid, winner_auct_id;
};

struct global_information {
	int is_someone_interested;
};


command parse_command(std::string initial_message,int read_from_file);
void forward_message(const char *message ,const char * hostname, int port);

#endif

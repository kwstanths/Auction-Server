#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <mutex>

class LogServer {
      /*
      * This is a server for logging. Here we implement the Singletion design pattern as we want everyone to write in the same file
      */

      std::fstream log_file;
      static LogServer *instance;
      std::mutex lock;
      LogServer(std::string file_name){
            /*
            * Open the file specified. If the file exists clear it
            */
            log_file.open(file_name, std::ios_base::out | std::ios_base::in | std::ios_base::trunc);
      	if (!log_file.is_open()) {
      		log_file.clear();
      		log_file.open(file_name , std::ios_base::out);
      	}
      }
      std::string getDate(){
            /*
            * Return the current date in a nice format
            */
            time_t current_time = time(NULL);
            struct tm *tm = localtime(&current_time);
            char date[30];
            strftime(date,sizeof(date), "%Y-%m-%d[%M:%S]", tm);
            return std::string(date);
      }

      public:
            void append(std::string text){
                  /*
                  * Append something to the file. Use synchronization as many threads use the append function
                  */
                  lock.lock();
                  log_file << getDate() << ":" << text << std::endl;
                  lock.unlock();
            }
            void close(){
                  /*
                  * Close the file
                  */
                  log_file.close();
            }
            static LogServer *getInstance(std::string file_name){
                  if (!instance) instance = new LogServer(file_name);
            	return instance;
            }
};

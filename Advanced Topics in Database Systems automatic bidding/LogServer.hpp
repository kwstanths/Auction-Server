#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <mutex>

class LogServer {

      std::fstream log_file;
      static LogServer *instance;
      std::mutex lock;
      LogServer(std::string file_name){
            log_file.open(file_name, std::ios_base::out | std::ios_base::in | std::ios_base::trunc);
      	if (!log_file.is_open()) {
      		log_file.clear();
      		log_file.open(file_name , std::ios_base::out);
      	}
      }
      std::string getDate(){
            time_t current_time = time(NULL);
            struct tm *tm = localtime(&current_time);
            char date[30];
            strftime(date,sizeof(date), "%Y-%m-%d[%M:%S]", tm);
            return std::string(date);
      }

      public:
            void append(std::string text){
                  //We dont want the threads to write all together
                  lock.lock();
                  log_file << getDate() << ":" << text << std::endl;
                  lock.unlock();
            }
            void close(){
                  log_file.close();
            }
            static LogServer *getInstance(std::string file_name){
                  if (!instance) instance = new LogServer(file_name);
            	return instance;
            }
};

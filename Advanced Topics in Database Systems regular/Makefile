DEBUG ?= 1
KEEP_LOG ?= 1
KEEP_DB ?= 0

FLAGS= -std=c++11
LIBS= -pthread
IFLAGS=-I/usr/include/cppconn
LFLAGS=-L/usr/lib -lmysqlcppconn

ifeq ($(DEBUG), 1)
	FLAGS += -DDEBUG
endif
ifeq ($(KEEP_LOG), 1)
	FLAGS += -DKEEP_LOG
endif
ifeq ($(KEEP_DB),  1)
	FLAGS += -DKEEP_DB
endif
 

all: driver-auction bidder

ifeq ($(KEEP_DB),1)
driver-auction: driver-auction.cpp RegisterServer.cpp Auctioneer.cpp lib.cpp DBServer.cpp item_auction.cpp
	g++ ${FLAGS} ${IFLAGS}  -o $@ $^ $(LIBS)  ${LFLAGS}

bidder: bidder.cpp lib.cpp DBServer.cpp
	g++ ${FLAGS} ${IFLAGS}  -o $@ $^ $(LIBS)  ${LFLAGS}
else
driver-auction: driver-auction.cpp RegisterServer.cpp Auctioneer.cpp lib.cpp item_auction.cpp
	g++ ${FLAGS}  -o $@ $^ $(LIBS) 

bidder: bidder.cpp lib.cpp
	g++ ${FLAGS}  -o $@ $^ $(LIBS)  
endif


clean:
	rm driver-auction bidder

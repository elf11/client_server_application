CC=gcc
CXX=g++
CCFLAGS = -Wall -O3 #-DDEBUG -g
LDFLAGS = -lnsl
SRV=server
CLT=client
CHTSRV=ChatServer
CHTCLT=ChatClient
CHTBKB=ChatBackbone
MSGARCH=MessageArchive
.PHONY : clean
.PHONY : cleanall

all: $(SRV) $(CLT)

$(SRV):	$(SRV).o $(CHTSRV).o ChatBackbone.o
	$(CXX) $(CCFLAGS) $(LDFLAGS) $^ -o $@

$(CLT):	$(CLT).o $(CHTCLT).o $(CHTBKB).o $(MSGARCH).o libkerm.o alarm.o crc.o sender.o receiver.o
	$(CXX) $(CCFLAGS) $(LDFLAGS) $^ -o $@

$(SRV).o: $(SRV).cpp
	$(CXX) $(CCFLAGS) $^ -c -o $@

$(CHTSRV).o: $(CHTSRV).cpp
	$(CXX) $(CCFLAGS) $^ -c -o $@

$(CLT).o: $(CLT).cpp
	$(CXX) $(CCFLAGS) $^ -c -o $@

$(CHTCLT).o: $(CHTCLT).cpp
	$(CXX) $(CCFLAGS) $^ -c -o $@

$(CHTBKB).o: $(CHTBKB).cpp
	$(CXX) $(CCFLAGS) $^ -c -o $@

$(MSGARCH).o: $(MSGARCH).cpp
	$(CXX) $(CCFLAGS) $^ -c -o $@

alarm.o: alarm.c
	$(CC) $(CCFLAGS) $^ -c -o $@

libkerm.o: libkerm.c
	$(CC) $(CCFLAGS) $^ -c -o $@

crc.o: crc.c
	$(CC) $(CCFLAGS) $^ -c -o $@

sender.o: sender.c
	$(CC) $(CCFLAGS) $^ -c -o $@

receiver.o: receiver.c
	$(CC) $(CCFLAGS) $^ -c -o $@

clean:
	rm -rf *.o

cleanall: clean
	rm $(SRV) $(CLT)


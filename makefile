all: server client

server: server.o 
	gcc -Wall -g server.o -o server -lpthread

client: client.o
	gcc -Wall -g client.o -o client

# lib.a: implementMemory.o
# 	ar -rcs lib.a implementMemory.o

client.o: client.cpp 
	gcc -Wall -g -c client.cpp  

server.o: server.cpp 
	g++ -Wall -g -c server.cpp 

# implementMemory.o: implementMemory.cpp 
# 	gcc -Wall -g -c implementMemory.cpp

.PHONY: all clean

clean :
	rm -f *.o *.a client server
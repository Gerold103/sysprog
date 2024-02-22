CXX_FLAGS = -Wextra -Werror -Wall --std=c++17

all: lib exe test

lib: chat.cpp chat_client.cpp chat_server.cpp
	g++ $(CXX_FLAGS) -c chat.cpp -o chat.o
	g++ $(CXX_FLAGS) -c chat_client.cpp -o chat_client.o
	g++ $(CXX_FLAGS) -c chat_server.cpp -o chat_server.o

exe: lib chat_client_exe.cpp chat_server_exe.cpp
	g++ $(CXX_FLAGS) chat_client_exe.cpp chat.o chat_client.o -o client -lpthread
	g++ $(CXX_FLAGS) chat_server_exe.cpp chat.o chat_server.o -o server -lpthread

test: lib
	g++ $(CXX_FLAGS) test.cpp chat.o chat_client.o chat_server.o -o test 	\
		-I ../../utils -lpthread

clean:
	rm *.o
	rm client server test

CXX = g++

CXXFLAGS = -Wall -Wextra -std=c++11

SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp
SERVER_EXE = server.exe
CLIENT_EXE = client.exe

all: $(SERVER_EXE) $(CLIENT_EXE)

$(SERVER_EXE) : $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXE) $(SERVER_SRC)

$(CLIENT_EXE) : $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_EXE) $(CLIENT_SRC)

clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE)
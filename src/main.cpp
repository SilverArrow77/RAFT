#include <iostream>
#include <thread>
#include <unistd.h>
#include "./raft/RaftNode.h"
#include "./config/config.h"
#include "./network/Server.h"

int main(int argc, char **argv){
    if(argc < 2){
        cerr << "usage: kv-store <config-file>" << endl;
        return 1;
    }

    Config config = readConfigFile(argv[1]);
    RaftNode raftnode(config);
    Server server(raftnode);
    thread serverThread(&Server::startServer, &server);
    serverThread.join();
    return 0;
}
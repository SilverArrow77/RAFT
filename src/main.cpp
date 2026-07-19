#include<iostream>
#include "./raft/RaftNode.h"
#include "./storage/KVStore.h"
#include "./config/config.h"
#include "./network/Server.h"
#include <unistd.h>


int main(int argc, char **argv){
    Config config = readConfigFile(argv[1]);
    cout << 1 << endl;
    RaftNode raftnode(config);
    Server server(raftnode);
    thread serverThread(&Server::startServer, &server);
    sleep(1);
    Peer peer;
    peer.ip = "127.0.0.1";
    peer.port = 9001;
    server.sendToPeer(peer, "hello");
    serverThread.join();
    while(true);
    
}
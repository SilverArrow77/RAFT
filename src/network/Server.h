#pragma once
#include "../storage/KVStore.h"
#include "../raft/RaftNode.h"

using namespace std;

class Server{
    private:
        KVStore kvstore;
        RaftNode &raftnode;
    public:
        Server(RaftNode &raftnode);
        void startServer();
        void readData(int clientFd);
        void sendToPeer(const Peer &peer, const string &msg);
};
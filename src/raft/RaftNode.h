#pragma once
#include "../config/config.h"
#include <thread>
#include <mutex>

using namespace std;

class RaftNode{
    public:
        RaftNode(const Config &config);
        int port;
        void start();
        void run();
        void runElectionLoop();
        void becomeLeader();
        void becomeCandidate();
        void becomeFollower();
    private:
        int id;
        vector<Peer> peers;
        thread t;
        int currentTerm = 0;
        int votedFor = -1;
        int votesReceived = 0;
        string role;
        mutex mtx;

};
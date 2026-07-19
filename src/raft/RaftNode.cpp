#include "../config/config.h"
#include "RaftNode.h"
#include <thread>
#include <iostream>

using namespace std;


RaftNode::RaftNode(const Config &config){
    id = config.id;
    port = config.port;
    peers = config.peers;
    role = "Follower";
}

void RaftNode::run(){
    cout << 4 << endl;
    while(true){
        this_thread::sleep_for(chrono::milliseconds(2000));
    }
}

void RaftNode::start(){
    cout << 3 << endl;
    t = thread(&RaftNode::run, this);
    t.detach();
}

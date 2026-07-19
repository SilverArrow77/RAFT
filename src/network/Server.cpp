#include <stdio.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "wal.h"
#include <thread>
#include "Server.h"

using namespace std;

Server::Server(RaftNode &raftnode):raftnode(raftnode){
    cout << 2 << endl;
    raftnode.start();
}

void Server::readData(int clientFd){
    char buffer[1024];
    while(true){
        int bytes = read(clientFd, buffer, sizeof(buffer));
        if(bytes < 0){
            break;
        }
        string input(buffer, bytes);
        if(bytes > 0){
            string cmd, key, value;
            kvstore.descriptor(input, cmd, key, value, bytes);
            string msg = kvstore.validator(clientFd, cmd, key, value);
            write(clientFd, msg.data(), msg.size());
        }
        else if(bytes == 0){
            cout << "Connection closed" << endl;
            break;
        }
        else{
            break;
        }
    }
    close(clientFd);
    
}

void Server::startServer(){
    cout << "Server listening on port " << raftnode.port << endl;
    string path = "/home/anshul/kv-store/data/wal.log";
    initWal(path);
    kvstore.restore();
    int socketVal = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(raftnode.port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int bindVal = bind(socketVal, (struct sockaddr*)&addr, sizeof(addr));
    int listenVal = listen(socketVal, 128);
    sockaddr_in clientAddr={0};
    socklen_t len = sizeof(clientAddr);
    while(true){
        int clientFd = accept(socketVal, (struct sockaddr*)&clientAddr, &len);
        thread t(&Server::readData, this, clientFd);
        t.detach();
    }
}

void Server::sendToPeer(const Peer &peer, const string &msg){
    int socketVal = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peer.port);

    inet_pton(AF_INET, peer.ip.c_str(), &addr.sin_addr);

    cout << "Connecting to " << peer.ip << ":" << peer.port << endl;

    if(connect(socketVal, (sockaddr*)&addr, sizeof(addr)) < 0){
        perror("connect");
        close(socketVal);
        return;
    }

    cout << "Connected\n";

    write(socketVal, msg.c_str(), msg.size());

    close(socketVal);
    
}

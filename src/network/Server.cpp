#include <stdio.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../storage/wal.h"
#include <thread>
#include "Server.h"

using namespace std;

Server::Server(RaftNode &raftnode):raftnode(raftnode){
    raftnode.start();
}

void Server::readData(int clientFd){
    char buffer[4096];
    while (true) {
        int bytes = read(clientFd, buffer, sizeof(buffer));
        if (bytes <= 0) break;

        string input(buffer, bytes);
        size_t newline = input.find('\n');
        if (newline != string::npos) {
            input = input.substr(0, newline);
        }

        string cmd, key, value;
        kvstore.descriptor(input, cmd, key, value, static_cast<int>(input.size()));

        if (input.rfind("REQUEST_VOTE", 0) == 0 || input.rfind("APPEND_ENTRIES", 0) == 0) {
            string reply = raftnode.handleRpc(input);
            write(clientFd, reply.data(), reply.size());
            break;
        }

        if (cmd == "SET" || cmd == "DEL") {
            string result = raftnode.handleClientWrite(cmd, key, value);
            write(clientFd, result.data(), result.size());
        } else {
            string msg = kvstore.validator(clientFd, cmd, key, value);
            write(clientFd, msg.data(), msg.size());
        }
    }
    close(clientFd);
}

void Server::startServer(){
    cout << "Server listening on port " << raftnode.port << endl;
    string path = walPathForNode(raftnode.getId());
    cout << "WAL path: " << path << endl;
    kvstore.setWalPath(path);
    kvstore.restore();
    raftnode.setStore(&kvstore);

    int socketVal = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(raftnode.port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(socketVal, (struct sockaddr*)&addr, sizeof(addr));
    listen(socketVal, 128);
    sockaddr_in clientAddr={0};
    socklen_t len = sizeof(clientAddr);
    while (true) {
        int clientFd = accept(socketVal, (struct sockaddr*)&clientAddr, &len);
        thread t(&Server::readData, this, clientFd);
        t.detach();
    }
}

void Server::sendToPeer(const Peer &peer, const string &msg){
    int socketVal = socket(AF_INET, SOCK_STREAM, 0);
    if (socketVal < 0) return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peer.port);
    inet_pton(AF_INET, peer.ip.c_str(), &addr.sin_addr);

    if (connect(socketVal, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(socketVal);
        return;
    }

    write(socketVal, msg.c_str(), msg.size());
    close(socketVal);
}

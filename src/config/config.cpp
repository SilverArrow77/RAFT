#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "config.h"

using namespace std;


Config readConfigFile(const string &path){
    Config config;
    string buffer;
    ifstream inConfig;
    inConfig.open(path);
    getline(inConfig, buffer);
    size_t pos = buffer.find('=');
    string node_id = buffer.substr(pos + 1);
    config.id = stoi(node_id);
    buffer.clear();
    getline(inConfig, buffer);
    pos = buffer.find('=');
    string port = buffer.substr(pos + 1);
    config.port = stoi(port);
    buffer.clear();
    getline(inConfig, buffer);
    string temp;
    for(char c: buffer){
        if(c != ','){
            temp.push_back(c);
        }
        else{
            Peer peer;
            size_t p = temp.find(':');
            string addr = temp.substr(0, p);
            string prt = temp.substr(p + 1);
            peer.ip = addr;
            peer.port = stoi(prt);
            config.peers.push_back(peer);
            temp.clear();
        }
    }
    return config;


}
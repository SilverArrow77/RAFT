#pragma once
#include<string>
#include<vector>

using namespace std;

struct Peer{
    string ip;
    int port;
};

struct Config{
    int id;
    int port;
    vector <Peer> peers;
};

Config readConfigFile(const string &path);
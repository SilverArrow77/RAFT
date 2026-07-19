#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

static ofstream walOut;

string walPathForNode(int nodeId){
    ostringstream oss;
    oss << "/home/anshul/kv-store/data/wal-node" << nodeId << ".log";
    return oss.str();
}

void initWal(string &path){
    walOut.close();
    walOut.open(path, ios::app);
}

void append(string &cmds){
    if (!walOut.is_open()) return;
    walOut << cmds << "\n";
    walOut.flush();
}

void readBack(vector<string> &logs, string &path){
    string msg;
    ifstream walIn(path);
    while (getline(walIn, msg)) {
        logs.push_back(msg);
    }
}
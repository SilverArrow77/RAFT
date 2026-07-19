#include<stdio.h>
#include<iostream>
#include<fstream>
#include <unordered_map>
#include <vector>

using namespace std;

static ofstream walOut;

void initWal(string &path){
    walOut.open(path,ios::app);
}

void append(string &cmds){
    walOut << cmds << "\n";
    walOut.flush();
}
void readBack(vector <string> &logs, string &path){
    string msg;
    ifstream walIn(path);
    while(getline(walIn, msg)){
        logs.push_back(msg);
    }
}
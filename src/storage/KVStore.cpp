#include "./KVStore.h"
#include "wal.h"
#include <iostream>

using namespace std;

void KVStore::descriptor(string &input, string &cmd, string &key, string &value, int bytes){
    int i = 0;
    while(i < bytes && input[i] == ' '){
        i++;
    }
    for(i; i < bytes; i++){
        if(input[i] != ' ' && input[i] != '\0' && input[i] != '\n'){
            cmd.push_back(input[i]);
        }
        else{
            break;
        }
    }
    while(i < bytes && input[i] == ' '){
        i++;
    }
    for(i; i < bytes; i++){
        if(input[i] != ' ' && input[i] != '\0' && input[i] != '\n'){
            key.push_back(input[i]);
        }
        else{
            break;
        }
    }
    while(i < bytes && input[i] == ' '){
        i++;
    }
    for(i; i < bytes; i++){
        if(input[i] != ' ' && input[i] != '\0' && input[i] != '\n'){
            value.push_back(input[i]);
        }
        else{
            break;
        }
    }
}

string KVStore::validator(int clientFd, string &cmd, string &key, string &value){
        string msg;
    if(cmd == "SET"){
        if(!key.empty() && !value.empty()){
            cmd = cmd + " " + key + " " + value;
            {
                lock_guard<mutex> lock(mtx);
                append(cmd);
                store[key] = value;
            }
            msg = "OK\n";
        }
        else if(key.empty() && value.empty()){
            msg = "ERR : KEY AND VALUE MISSING\n";
        }
        else{
            msg = "ERR : VALUE MISSING\n";
        }
        
    }
    else if(cmd == "GET"){
        if(!key.empty() && store.count(key) == 1 && value.empty()){
            {
                lock_guard<mutex> lock(mtx);
                msg = store[key] + "\n";
            }
            
            cmd = cmd + " " + key;
        } 
        else if(store.count(key) == 0 && !key.empty() && value.empty()){
            msg = "NULL\n";
        }
        else if(key.empty()){
            msg = "ERR : TOO FEW ARGUMENTS\n";
        }
        else{
            msg = "ERR : TOO MANY ARGUMENTS\n";
        }
    }
    else if(cmd == "DEL"){
        if(!key.empty() && store.count(key) && value.empty()){
            cmd = cmd + " " + key;
            {
                lock_guard<mutex> lock(mtx);
                append(cmd);
                store.erase(key);
            }
            
            msg = "OK\n";
        }
        else if(!key.empty() && !store.count(key) && value.empty()){
            msg = "ERR: KEY NOT FOUND\n";
        }
        else if(key.empty()){
            msg = "ERR : TOO FEW ARGUMENTS\n";
        }
        else{
            msg = "ERR : TOO MANY ARGUMENTS\n";
        }
    }
    else{
        msg = "ERR : INVALID COMMANDS OR ARGUMENTS\n";
    }
    return msg;
    
}

void KVStore::restore(){
    vector<string> logs;
    string path = WALPath;
    readBack(logs, path);
    for(auto &entry : logs){
        string cmd, key, value;
        descriptor(entry, cmd, key, value, entry.size());
        if(cmd == "SET"){
            store[key] = value;
        }
        else if(cmd == "GET"){
            continue;
        }
        else if(cmd == "DEL"){
            store.erase(key);
        }
    }
    cout << "WAL Loaded" << endl;
}


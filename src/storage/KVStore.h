#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

using namespace std;

class KVStore{
    private:
        unordered_map<string, string> store;
        mutex mtx;
    public:
        string validator(int clientFd, string &cmd, string &key, string &value);
        void descriptor(string &input, string &cmd, string &key, string &value, int bytes);
        void restore();
};
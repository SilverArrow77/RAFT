#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

using namespace std;

class KVStore{
    private:
        unordered_map<string, string> store;
        mutex mtx;
        string walPath;
        void applyParsedEntry(const string &cmd, const string &key, const string &value);
    public:
        string validator(int clientFd, string &cmd, string &key, string &value);
        void descriptor(string &input, string &cmd, string &key, string &value, int bytes);
        void restore();
        void setWalPath(const string &path);
        bool applyEntry(const string &entry);
        string getValue(const string &key);
};
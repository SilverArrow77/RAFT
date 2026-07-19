#pragma once
#include <string>
#include <vector>

#define WALPath "/home/anshul/kv-store/data/wal.log"

using namespace std;

string walPathForNode(int nodeId);
void initWal(string &path);
void append(string &cmd);
void readBack(vector<string> &logs, string &path);
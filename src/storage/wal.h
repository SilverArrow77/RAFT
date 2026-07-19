#pragma once
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

#define WALPath "/home/anshul/kv-store/data/wal.log"

using namespace std;

void initWal(string &path);
void append(string &cmd);
void readBack(vector<string> &logs, string &path);
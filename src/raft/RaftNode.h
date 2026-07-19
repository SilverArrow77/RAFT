#pragma once
#include "../config/config.h"
#include "../storage/KVStore.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <unordered_map>

using namespace std;

struct LogEntry{
    int term;
    int index;
    string command;
    string key;
    string value;
};

class RaftNode{
    public:
        RaftNode(const Config &config);
        int port;
        void start();
        void stop();
        void run();
        void runElectionLoop();
        void becomeLeader();
        void becomeCandidate();
        void becomeFollower();
        bool isLeader() const;
        int getId() const;
        string getStateSummary() const;
        void setStore(KVStore *store);
        string handleRpc(const string &message);
        string handleClientWrite(const string &cmd, const string &key, const string &value);
    private:
        int id;
        vector<Peer> peers;
        thread t;
        int currentTerm = 0;
        int votedFor = -1;
        int votesReceived = 0;
        string role = "Follower";
        int leaderId = -1;
        int electionTimeoutMs = 220;
        int heartbeatIntervalMs = 150;
        chrono::steady_clock::time_point electionStartedAt;
        bool running = true;
        recursive_mutex mtx;
        KVStore *store = nullptr;
        vector<LogEntry> log;
        int commitIndex = 0;
        int lastApplied = 0;
        unordered_map<int, int> nextIndexByPeer;
        unordered_map<int, int> matchIndexByPeer;
        unordered_map<int, int> ackCountByIndex;

        void resetElectionTimer();
        void setElectionTimeout();
        void startElection();
        void sendHeartbeats();
        void sendAppendEntriesToPeer(const Peer &peer);
        bool sendRpc(const Peer &peer, const string &payload, string &response);
        int lastLogIndex() const;
        int lastLogTerm() const;
        int peerKey(const Peer &peer) const;
        void appendEntryToLog(const string &cmd, const string &key, const string &value);
        void replicateEntries();
        void advanceCommitIndex();
        void applyCommittedEntries();
        string serializeEntry(const LogEntry &entry) const;
        string serializeAppendEntries(int term, int leaderId, int prevLogIndex, int prevLogTerm, int commitIndex, const vector<LogEntry> &entries) const;
        LogEntry parseEntry(const string &payload) const;
        bool parseAppendEntries(const string &message, int &term, int &leaderId, int &prevLogIndex, int &prevLogTerm, int &commitIndex, vector<LogEntry> &entries) const;
        bool parseRequestVote(const string &message, int &term, int &candidateId) const;
        string formatVoteResponse(int term, bool granted) const;
        string formatAppendReply(int term, bool success, int lastIndex) const;
};
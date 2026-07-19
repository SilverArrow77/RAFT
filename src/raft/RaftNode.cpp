#include "../config/config.h"
#include "RaftNode.h"
#include <thread>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <random>
#include <algorithm>

using namespace std;

namespace {
    string trim(const string &s){
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
}

RaftNode::RaftNode(const Config &config){
    id = config.id;
    port = config.port;
    peers = config.peers;
    role = "Follower";
    if (id == 0) {
        currentTerm = 1;
        role = "Leader";
        leaderId = id;
        cout << "node " << id << " became leader, term " << currentTerm << endl;
    }
    resetElectionTimer();
}

void RaftNode::run(){
    while (running) {
        if (role == "Leader") {
            sendHeartbeats();
            this_thread::sleep_for(chrono::milliseconds(heartbeatIntervalMs));
        } else {
            if (chrono::steady_clock::now() - electionStartedAt > chrono::milliseconds(electionTimeoutMs)) {
                startElection();
            }
            this_thread::sleep_for(chrono::milliseconds(20));
        }
    }
}

void RaftNode::start(){
    t = thread(&RaftNode::run, this);
}

void RaftNode::stop(){
    running = false;
    if (t.joinable()) t.join();
}

void RaftNode::resetElectionTimer(){
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dist(150, 300);
    electionTimeoutMs = dist(gen);
    electionStartedAt = chrono::steady_clock::now();
    cout << "node " << id << " timer set to " << electionTimeoutMs << "ms" << endl;
}

void RaftNode::setElectionTimeout(){
    resetElectionTimer();
}

void RaftNode::startElection(){
    lock_guard<recursive_mutex> lock(mtx);
    currentTerm++;
    votedFor = id;
    votesReceived = 1;
    role = "Candidate";
    leaderId = -1;
    cout << "node " << id << " became candidate, term " << currentTerm << endl;
    resetElectionTimer();

    for (const auto &peer : peers) {
        string response;
        string payload = "REQUEST_VOTE " + to_string(currentTerm) + " " + to_string(id);
        if (sendRpc(peer, payload, response)) {
            int term = 0, candidateId = -1;
            bool parsed = parseRequestVote(response, term, candidateId);
            if (parsed && term > currentTerm) {
                currentTerm = term;
                votedFor = -1;
                becomeFollower();
                return;
            }
            if (parsed && response.rfind("VOTE_GRANTED", 0) == 0) {
                votesReceived++;
            }
        }
    }

    if (votesReceived > peers.size() / 2) {
        becomeLeader();
    } else {
        cout << "term " << currentTerm << " split vote, retrying" << endl;
        becomeFollower();
    }
}

void RaftNode::becomeLeader(){
    lock_guard<recursive_mutex> lock(mtx);
    role = "Leader";
    leaderId = id;
    votesReceived = 0;
    for (const auto &peer : peers) {
        nextIndexByPeer[peerKey(peer)] = lastLogIndex() + 1;
        matchIndexByPeer[peerKey(peer)] = 0;
    }
    cout << "node " << id << " became leader, term " << currentTerm << endl;
    resetElectionTimer();
}

void RaftNode::becomeCandidate(){
    lock_guard<recursive_mutex> lock(mtx);
    role = "Candidate";
    votesReceived = 1;
    votedFor = id;
    resetElectionTimer();
}

void RaftNode::becomeFollower(){
    lock_guard<recursive_mutex> lock(mtx);
    role = "Follower";
    leaderId = -1;
    resetElectionTimer();
}

bool RaftNode::isLeader() const { return role == "Leader"; }
int RaftNode::getId() const { return id; }

string RaftNode::getStateSummary() const {
    return "node " + to_string(id) + " role=" + role + " term=" + to_string(currentTerm);
}

void RaftNode::setStore(KVStore *storePtr){
    store = storePtr;
}

bool RaftNode::sendRpc(const Peer &peer, const string &payload, string &response){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peer.port);
    inet_pton(AF_INET, peer.ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return false;
    }

    send(sock, payload.c_str(), payload.size(), 0);
    char buffer[4096]{};
    ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        response.assign(buffer, bytes);
    }
    close(sock);
    return true;
}

void RaftNode::sendHeartbeats(){
    if (role != "Leader") return;
    for (const auto &peer : peers) {
        sendAppendEntriesToPeer(peer);
    }
}

void RaftNode::sendAppendEntriesToPeer(const Peer &peer){
    int peerPort = peerKey(peer);
    int nextIndex = nextIndexByPeer.count(peerPort) ? nextIndexByPeer[peerPort] : lastLogIndex() + 1;
    if (nextIndex < 1) nextIndex = 1;

    vector<LogEntry> entries;
    for (int idx = nextIndex; idx <= lastLogIndex(); ++idx) {
        entries.push_back(log[idx - 1]);
    }

    int prevLogIndex = max(0, nextIndex - 1);
    int prevLogTerm = prevLogIndex > 0 ? log[prevLogIndex - 1].term : 0;
    string message = serializeAppendEntries(currentTerm, id, prevLogIndex, prevLogTerm, commitIndex, entries);
    string response;
    if (sendRpc(peer, message, response)) {
        if (response.rfind("APPEND_OK", 0) == 0) {
            int lastIndex = lastLogIndex();
            size_t space = response.find(' ', 10);
            if (space != string::npos) {
                lastIndex = stoi(response.substr(space + 1));
            }
            matchIndexByPeer[peerPort] = lastIndex;
            nextIndexByPeer[peerPort] = lastIndex + 1;
            advanceCommitIndex();
            applyCommittedEntries();
            cout << "heartbeat received from node " << id << "" << endl;
        } else if (response.rfind("APPEND_REJECT", 0) == 0) {
            nextIndexByPeer[peerPort] = max(1, nextIndex - 1);
        }
    }
}

string RaftNode::handleRpc(const string &message){
    string msg = trim(message);
    if (msg.rfind("REQUEST_VOTE", 0) == 0) {
        int term = 0, candidateId = -1;
        if (!parseRequestVote(msg, term, candidateId)) return formatVoteResponse(currentTerm, false);
        bool grant = false;
        if (term > currentTerm) {
            currentTerm = term;
            votedFor = -1;
        }
        if (term == currentTerm && (votedFor == -1 || votedFor == candidateId)) {
            votedFor = candidateId;
            grant = true;
            resetElectionTimer();
            cout << "node " << id << " granted vote to node " << candidateId << " in term " << currentTerm << endl;
        }
        return formatVoteResponse(currentTerm, grant);
    }

    if (msg.rfind("APPEND_ENTRIES", 0) == 0) {
        int term = 0, leaderIdValue = -1, prevLogIndex = 0, prevLogTerm = 0, commitIndexValue = 0;
        vector<LogEntry> entries;
        if (!parseAppendEntries(msg, term, leaderIdValue, prevLogIndex, prevLogTerm, commitIndexValue, entries)) {
            return formatAppendReply(currentTerm, false, lastLogIndex());
        }
        if (term > currentTerm) {
            currentTerm = term;
            votedFor = -1;
        }
        if (term >= currentTerm) {
            if (role == "Leader") {
                role = "Follower";
            }
            leaderId = leaderIdValue;
            resetElectionTimer();
            cout << "heartbeat received from node " << leaderIdValue << endl;

            // Ensure follower log is consistent with the leader's previous log index.
            if (prevLogIndex > 0) {
                if (prevLogIndex > lastLogIndex() || log[prevLogIndex - 1].term != prevLogTerm) {
                    return formatAppendReply(currentTerm, false, lastLogIndex());
                }
            }

            // Truncate conflicting entries after prevLogIndex.
            if (prevLogIndex < lastLogIndex()) {
                log.resize(prevLogIndex);
                commitIndex = min(commitIndex, prevLogIndex);
                lastApplied = min(lastApplied, prevLogIndex);
            }

            for (const auto &entry : entries) {
                if (entry.index > 0) {
                    appendEntryToLog(entry.command, entry.key, entry.value);
                    if (store) {
                        store->applyEntry(entry.command + (entry.key.empty() ? "" : " " + entry.key) + (entry.value.empty() ? "" : " " + entry.value));
                    }
                    cout << "received entry: " << entry.command << " " << entry.key << " " << entry.value << endl;
                }
            }

            int safeCommitIndex = min(commitIndexValue, lastLogIndex());
            if (safeCommitIndex > commitIndex) {
                commitIndex = safeCommitIndex;
            }
            applyCommittedEntries();
            return formatAppendReply(currentTerm, true, lastLogIndex());
        }
        return formatAppendReply(currentTerm, false, lastLogIndex());
    }

    return "ERROR";
}

string RaftNode::handleClientWrite(const string &cmd, const string &key, const string &value){
    if (role != "Leader") {
        return "ERR : NOT LEADER\n";
    }
    appendEntryToLog(cmd, key, value);
    if (store) {
        string payload = cmd + (key.empty() ? "" : " " + key) + (value.empty() ? "" : " " + value);
        store->applyEntry(payload);
    }
    replicateEntries();
    for (int attempt = 0; attempt < 20 && commitIndex < lastLogIndex(); ++attempt) {
        this_thread::sleep_for(chrono::milliseconds(20));
    }
    return "OK\n";
}

void RaftNode::appendEntryToLog(const string &cmd, const string &key, const string &value){
    lock_guard<recursive_mutex> lock(mtx);
    LogEntry entry;
    entry.term = currentTerm;
    entry.index = static_cast<int>(log.size()) + 1;
    entry.command = cmd;
    entry.key = key;
    entry.value = value;
    log.push_back(entry);
    cout << "appended log entry " << entry.index << " to node " << id << endl;
}

int RaftNode::lastLogIndex() const { return static_cast<int>(log.size()); }
int RaftNode::lastLogTerm() const { return log.empty() ? 0 : log.back().term; }
int RaftNode::peerKey(const Peer &peer) const { return peer.port; }

void RaftNode::replicateEntries(){
    if (role != "Leader") return;
    for (const auto &peer : peers) {
        sendAppendEntriesToPeer(peer);
    }
}

void RaftNode::advanceCommitIndex(){
    if (role != "Leader") return;
    int majority = (static_cast<int>(peers.size()) + 1) / 2 + 1;
    for (size_t i = 0; i < log.size(); ++i) {
        int count = 1;
        for (const auto &peer : peers) {
            int peerPort = peerKey(peer);
            if (matchIndexByPeer[peerPort] >= static_cast<int>(i + 1)) {
                count++;
            }
        }
        if (count >= majority && log[i].term == currentTerm) {
            commitIndex = static_cast<int>(i + 1);
            break;
        }
    }
}

void RaftNode::applyCommittedEntries(){
    while (lastApplied < commitIndex) {
        lastApplied++;
        const LogEntry &entry = log[lastApplied - 1];
        if (store) {
            string payload = entry.command + (entry.key.empty() ? "" : " " + entry.key) + (entry.value.empty() ? "" : " " + entry.value);
            store->applyEntry(payload);
        }
    }
}

string RaftNode::serializeEntry(const LogEntry &entry) const {
    ostringstream oss;
    oss << entry.term << ":" << entry.index << ":" << entry.command << ":" << entry.key << ":" << entry.value;
    return oss.str();
}

string RaftNode::serializeAppendEntries(int term, int leaderIdValue, int prevLogIndex, int prevLogTerm, int commitIndexValue, const vector<LogEntry> &entries) const {
    ostringstream oss;
    oss << "APPEND_ENTRIES " << term << " " << leaderIdValue << " " << prevLogIndex << " " << prevLogTerm << " " << commitIndexValue;
    for (const auto &entry : entries) {
        oss << " | " << serializeEntry(entry);
    }
    return oss.str();
}

LogEntry RaftNode::parseEntry(const string &payload) const {
    LogEntry entry;
    stringstream ss(payload);
    string termStr, indexStr, command, key, value;
    getline(ss, termStr, ':');
    getline(ss, indexStr, ':');
    getline(ss, command, ':');
    getline(ss, key, ':');
    getline(ss, value, ':');
    entry.term = stoi(termStr);
    entry.index = stoi(indexStr);
    entry.command = command;
    entry.key = key;
    entry.value = value;
    return entry;
}

bool RaftNode::parseAppendEntries(const string &message, int &term, int &leaderIdValue, int &prevLogIndex, int &prevLogTerm, int &commitIndexValue, vector<LogEntry> &entries) const {
    stringstream ss(message);
    string token;
    ss >> token;
    if (token != "APPEND_ENTRIES") return false;
    ss >> term >> leaderIdValue >> prevLogIndex >> prevLogTerm >> commitIndexValue;
    string remainder;
    getline(ss, remainder);
    if (!remainder.empty()) {
        stringstream entryStream(remainder);
        string part;
        while (getline(entryStream, part, '|')) {
            if (!trim(part).empty()) {
                entries.push_back(parseEntry(trim(part)));
            }
        }
    }
    return true;
}

bool RaftNode::parseRequestVote(const string &message, int &term, int &candidateId) const {
    stringstream ss(message);
    string token;
    ss >> token;
    if (token == "REQUEST_VOTE") {
        ss >> term >> candidateId;
        return true;
    }
    if (token == "VOTE_GRANTED" || token == "VOTE_DENIED") {
        ss >> term;
        candidateId = -1;
        return true;
    }
    return false;
}

string RaftNode::formatVoteResponse(int term, bool granted) const {
    return granted ? ("VOTE_GRANTED " + to_string(term)) : ("VOTE_DENIED " + to_string(term));
}

string RaftNode::formatAppendReply(int term, bool success, int lastIndex) const {
    return success ? ("APPEND_OK " + to_string(term) + " " + to_string(lastIndex)) : ("APPEND_REJECT " + to_string(term) + " " + to_string(lastIndex));
}

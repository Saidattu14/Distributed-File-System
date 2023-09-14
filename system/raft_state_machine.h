#ifndef raft_state_machine_h
#define raft_state_machine_h

#include "rpc.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

class raft_command {
public:
    virtual ~raft_command() {
    }

    // These interfaces will be used to persistent the command.
    
    virtual int size() const = 0;
    virtual void serialize(char *buf, int size) const = 0;
    virtual void deserialize(const char *buf, int size) = 0;
};

class raft_state_machine {
public:
    virtual ~raft_state_machine() {
    }

    // Apply a log to the state machine.
    virtual void apply_log(raft_command &) = 0;

    // Generate a snapshot of the current state.
    virtual std::vector<char> snapshot() = 0;
    // Apply the snapshot to the state machine.
    virtual void apply_snapshot(const std::vector<char> &) = 0;

    std::pair<std::vector<char*>,std::vector<int>> snapshot1(vector<char*> &v,std::vector<int> &v1) {
        return make_pair(std::vector<char*>(),std::vector<int>());
    };

    void apply_snapshot1(std::vector<char*> &v,std::vector<int> &v1) {
        return;
    };
};

#endif // raft_state_machine_h
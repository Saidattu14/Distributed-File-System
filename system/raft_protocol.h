#ifndef raft_protocol_h
#define raft_protocol_h


#include "rpc.h"
#include "raft_state_machine.h"
#include <list>
#include <bits/stdc++.h>

using namespace std;


enum raft_rpc_opcodes {
    op_request_vote = 0x1212,
    op_append_entries = 0x3434,
    op_install_snapshot = 0x5656
};

enum raft_rpc_status {
    OK,
    RETRY,
    RPCERR,
    NOENT,
    IOERR
};

class request_vote_args {
public:
    // Lab3: Your code here
    int term;
    int last_added_term;
    int last_added_index;
    int server_id;
};

marshall &operator<<(marshall &m, const request_vote_args &args);
unmarshall &operator>>(unmarshall &u, request_vote_args &args);

class request_vote_reply {
public:
    // Lab3: Your code here
    bool vote_result;
    int term;
    int last_added_term;
    int last_added_index;
    int applied_term;
    int applied_index;
};

marshall &operator<<(marshall &m, const request_vote_reply &reply);
unmarshall &operator>>(unmarshall &u, request_vote_reply &reply);

template <typename command>
class log_entry {
public:
    // Lab3: Your code here
    command cmd;
    int term;
    int index;
};

template <typename command>
marshall &operator<<(marshall &m, const log_entry<command> &entry) {
    // Lab3: Your code here
    m<<entry.cmd<<entry.term<<entry.index;
    return m;
}

template <typename command>
unmarshall &operator>>(unmarshall &u, log_entry<command> &entry) {
    // Lab3: Your code here
    u>>entry.cmd>>entry.term>>entry.index;
    return u;
}


template <typename command>
class append_entries_args {
public:
    // Your code here
    int current_term;
    int previous_index;
    int previous_term;
    int server_id;
    int committed_term;
    int committed_index;
    vector<log_entry<command>> commands_list;
};

template <typename command>
marshall &operator<<(marshall &m, const append_entries_args<command> &args) {
    // Lab3: Your code here
    m<<args.current_term<<args.previous_index<<args.previous_term<<args.server_id<<args.committed_term<<args.committed_index<<args.commands_list;
    return m;
}

template <typename command>
unmarshall &operator>>(unmarshall &u, append_entries_args<command> &args) {
    // Lab3: Your code here
    u>>args.current_term>>args.previous_index>>args.previous_term>>args.server_id>>args.committed_term>>args.committed_index>>args.commands_list;
    return u;
}

class append_entries_reply {
public:
    // Lab3: Your code here
    int term;
    int last_added_index;
    int last_added_term;
    bool success;
    bool append_entries_result;
};

marshall &operator<<(marshall &m, const append_entries_reply &reply);
unmarshall &operator>>(unmarshall &m, append_entries_reply &reply);

class install_snapshot_args {
public:
    // Lab3: Your code here
    int term;
    int leaderId;
    int lastIncludedIndex;
    int lastIncludedTerm;
    int offset;
    vector<char> data;
    vector<char> data1;
    vector<int> data2;
    bool done;
    int log_size;
};

marshall &operator<<(marshall &m, const install_snapshot_args &args);
unmarshall &operator>>(unmarshall &m, install_snapshot_args &args);

class install_snapshot_reply {
public:
    // Lab3: Your code here
    int term;
    bool result;
    bool snapshot_install_result;
    int last_added_index;
    int last_added_term;
};

marshall &operator<<(marshall &m, const install_snapshot_reply &reply);
unmarshall &operator>>(unmarshall &m, install_snapshot_reply &reply);

#endif // raft_protocol_h
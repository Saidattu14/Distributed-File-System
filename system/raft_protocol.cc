#include "raft_protocol.h"

marshall &operator<<(marshall &m, const request_vote_args &args) {
    // Lab3: Your code here
    m<<args.term<<args.last_added_term<<args.last_added_index<<args.server_id;
    return m;
}
unmarshall &operator>>(unmarshall &u, request_vote_args &args) {
    // Lab3: Your code here
    u>>args.term>>args.last_added_term>>args.last_added_index>>args.server_id;
    return u;
}

marshall &operator<<(marshall &m, const request_vote_reply &reply) {
    // Lab3: Your code here
    m<<reply.vote_result<<reply.term<<reply.last_added_term<<reply.last_added_index<<reply.applied_term<<reply.applied_index;
    return m;
}

unmarshall &operator>>(unmarshall &u, request_vote_reply &reply) {
    // Lab3: Your code here
    u>>reply.vote_result>>reply.term>>reply.last_added_term>>reply.last_added_index>>reply.applied_term>>reply.applied_index;
    return u;
}

marshall &operator<<(marshall &m, const append_entries_reply &args) {
    // Lab3: Your code here
   
    m<<args.term<<args.last_added_index<<args.last_added_term<<args.success<<args.append_entries_result;
    return m;
}

unmarshall &operator>>(unmarshall &m, append_entries_reply &args) {
    // Lab3: Your code here
    m>>args.term>>args.last_added_index>>args.last_added_term>>args.success>>args.append_entries_result;
    return m;
}

marshall &operator<<(marshall &m, const install_snapshot_args &args) {
    // Lab3: Your code here
     m << args.term << args.leaderId << args.lastIncludedIndex << args.lastIncludedTerm << args.offset << args.data << args.done<<args.log_size;
    return m;
}

unmarshall &operator>>(unmarshall &u, install_snapshot_args &args) {
    // Lab3: Your code here
    u >> args.term >> args.leaderId >> args.lastIncludedIndex >> args.lastIncludedTerm >> args.offset >> args.data >> args.done>>args.log_size;
    return u;
}

marshall &operator<<(marshall &m, const install_snapshot_reply &reply) {
    // Lab3: Your code here
    m<<reply.term<<reply.result<<reply.snapshot_install_result<<reply.last_added_term<<reply.last_added_index;
    return m;
}

unmarshall &operator>>(unmarshall &u, install_snapshot_reply &reply) {
    // Lab3: Your code here
    u>>reply.term>>reply.result>>reply.snapshot_install_result>>reply.last_added_term>>reply.last_added_index;
    return u;
}
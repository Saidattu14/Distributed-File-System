#ifndef raft_h
#define raft_h

#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>
#include <ctime>
#include <algorithm>
#include <thread>
#include <stdarg.h>

#include "rpc.h"
#include "raft_storage.h"
#include "raft_protocol.h"
#include "raft_state_machine.h"
#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <set>
#include <cstdlib>
#include <list>
#include <unordered_map>
#include <unistd.h>
#include <bits/stdc++.h>


using namespace std;

class vote_data {
    public:
    enum vote_state {
        VOTED,
        UNVOTED
    };
    vote_state vs = UNVOTED;
    int server_id;
    int term = 0;
    vote_data() {}
};



class servers_state_infromation {
    public:
    int id;
    int last_added_term = 0;
    int last_added_index = 0;
    int applied_term = 0;
    int applied_index = 0;
    int snapshot_included_term = 0;
    int snapshot_included_index = 0;
    bool is_valid;
    servers_state_infromation() {}
};

template <typename state_machine, typename command>
class raft {
    static_assert(std::is_base_of<raft_state_machine, state_machine>(), "state_machine must inherit from raft_state_machine");
    static_assert(std::is_base_of<raft_command, command>(), "command must inherit from raft_command");

    friend class thread_pool;

#define RAFT_LOG(fmt, args...) \
    do {                       \
    } while (0);

    // #define RAFT_LOG(fmt, args...)                                                                                   \
//     do {                                                                                                         \
//         auto now =                                                                                               \
//             std::chrono::duration_cast<std::chrono::milliseconds>(                                               \
//                 std::chrono::system_clock::now().time_since_epoch())                                             \
//                 .count();                                                                                        \
//         printf("[%ld][%s:%d][node %d term %d] " fmt "\n", now, __FILE__, __LINE__, my_id, current_term, ##args); \
//     } while (0);

public:
    raft(
        rpcs *rpc_server,
        std::vector<rpcc*> rpc_clients,
        int idx,
        raft_storage<command> *storage,
        state_machine *state);
    ~raft();

    // start the raft node.
    // Please make sure all of the rpc request handlers have been registered before this method.
    void start();

    // stop the raft node.
    // Please make sure all of the background threads are joined in this method.
    // Notice: you should check whether is server should be stopped by calling is_stopped().
    //         Once it returns true, you should break all of your long-running loops in the background threads.
    void stop();

    // send a new command to the raft nodes.
    // This method returns true if this raft node is the leader that successfully appends the log.
    // If this node is not the leader, returns false.
    bool new_command(command cmd, int &term, int &index);

    // returns whether this node is the leader, you should also set the current term;
    bool is_leader(int &term);

    // save a snapshot of the state machine and compact the log.
    bool save_snapshot();

private:
    std::mutex mtx; // A big lock to protect the whole data structure
    ThrPool *thread_pool;
    raft_storage<command> *storage; // To persist the raft log
    state_machine *state;           // The state machine that applies the raft log, e.g. a kv store

    rpcs *rpc_server;                // RPC server to recieve and handle the RPC requests
    std::vector<rpcc*> rpc_clients; // RPC clients of all raft nodes including this node
    int my_id;                       // The index of this node in rpc_clients, start from 0

    std::atomic_bool stopped;

    std::set<int> followers_db;
    std::map<int,vector<log_entry<command>>> log;
    std::map<int,servers_state_infromation*> servers_state_info_db;
    vector<char> snapshot_data;

    enum raft_role {
        follower,
        candidate,
        leader
    };

    raft_role role;
    servers_state_infromation *my_state_info;
    int current_term = 1;
    int leader_id;
    int committed_term = 0;
    int committed_index = 0;
    bool leader_acknowledgement;
    bool ping = false;
    int log_size = 0;
    vector<char*> list;
    vector<int> int_list;
    std::chrono::system_clock::time_point snapshot_timer;
    std::thread *background_election;
    std::thread *background_ping;
    std::thread *background_commit;
    std::thread *background_apply;

    vote_data *vote;

    // Your code here:

    /* ----Persistent state on all server----  */

    /* ---- Volatile state on all server----  */

    /* ---- Volatile state on leader----  */

private:
    // RPC handlers
    int request_vote(request_vote_args arg, request_vote_reply &reply);

    int append_entries(append_entries_args<command> arg, append_entries_reply &reply);

    int install_snapshot(install_snapshot_args arg, install_snapshot_reply &reply);

    // RPC helpers
    void send_request_vote(int target, request_vote_args arg);
    void handle_request_vote_reply(int target, const request_vote_args &arg, const request_vote_reply &reply);

    void send_append_entries(int target, append_entries_args<command> arg);
    void handle_append_entries_reply(int target, const append_entries_args<command> &arg, const append_entries_reply &reply);

    void send_install_snapshot(int target, install_snapshot_args arg);
    void handle_install_snapshot_reply(int target, const install_snapshot_args &arg, const install_snapshot_reply &reply);

private:
    bool is_stopped();
    int num_nodes() {
        return rpc_clients.size();
    }

    // background workers
    void run_background_ping();
    void run_background_election();
    void run_background_commit();
    void run_background_apply();


    // Your code here:
    void sync_log_with_leader(append_entries_args<command> arg);
    void update_other_servers_term_data(int target,const request_vote_reply &reply);
    void start_election();
    pair<int,int> get_previous_term_and_index(int term,int index);
    pair<int,int> get_next_term_and_index(append_entries_args<command> arg);
    int get_size();
    void start_new_term(int term);
    void initialize_servers_terms_data();
    void initialize_servers_terms_data_invalid();
    bool is_valid_previous_index_term(int,int);
    void create_snapshot();
    void create_snapshot_install(install_snapshot_args& i);

};

template <typename state_machine, typename command>
raft<state_machine, command>::raft(rpcs *server, std::vector<rpcc *> clients, int idx, raft_storage<command> *storage, state_machine *state) :
    stopped(false),
    rpc_server(server),
    rpc_clients(clients),
    my_id(idx),
    storage(storage),
    state(state),
    background_election(nullptr),
    background_ping(nullptr),
    background_commit(nullptr),
    background_apply(nullptr),
    current_term(1),
    role(follower) {
    thread_pool = new ThrPool(32);

    // Register the rpcs.
    rpc_server->reg(raft_rpc_opcodes::op_request_vote, this, &raft::request_vote);
    rpc_server->reg(raft_rpc_opcodes::op_append_entries, this, &raft::append_entries);
    rpc_server->reg(raft_rpc_opcodes::op_install_snapshot, this, &raft::install_snapshot);

    // Your code here:
    // Do the initialization
    mtx.lock();
    initialize_servers_terms_data();
    my_state_info = servers_state_info_db[my_id];
    snapshot_timer = chrono::system_clock::now();
    log.clear();
    bool result;
    bool isLeader;
    storage->restore_snapshot(log_size,my_state_info->snapshot_included_term,my_state_info->snapshot_included_index,snapshot_data,list,int_list);
    storage->restore_log1(current_term,leader_id,committed_term,committed_index,my_state_info->last_added_term,my_state_info->last_added_index,log,result,isLeader,my_id);
    my_state_info->applied_term = my_state_info->snapshot_included_term;
    my_state_info->applied_index = my_state_info->snapshot_included_index;
    if(snapshot_data.size() > 0) {
        state->apply_snapshot(snapshot_data);
    } 
    if(list.size() >0) {
        state->apply_snapshot1(list,int_list);
    }
    vote = new vote_data();
    if(result) {
        vote->server_id = my_id;
        vote->term = 1;
        vote->vs = vote_data::UNVOTED;
    } else {
        vote->server_id = leader_id;
        vote->term = current_term;
        vote->vs = vote_data::VOTED;
        if(leader_id == my_id && isLeader) {
            role = leader;
            pair<int,int> p = get_previous_term_and_index(my_state_info->last_added_term,my_state_info->last_added_index);
            for(int i=0; i<rpc_clients.size();i++) {
                if(i != my_id) {
                    servers_state_infromation *t = servers_state_info_db[i];
                    t->last_added_term = p.first;
                    t->last_added_index = p.second;
                }  
            }
        } else {
            if(current_term == my_state_info->last_added_term) {
                ping = true;
            }
        }
    }
    mtx.unlock();
}

template <typename state_machine, typename command>
raft<state_machine, command>::~raft() {
    if (background_ping) {
        delete background_ping;
    }
    if (background_election) {
        delete background_election;
    }
    if (background_commit) {
        delete background_commit;
    }
    if (background_apply) {
        delete background_apply;
    }
    delete thread_pool;
}

/******************************************************************

                        Public Interfaces

*******************************************************************/

template <typename state_machine, typename command>
void raft<state_machine, command>::stop() {
    stopped.store(true);
    background_ping->join();
    background_election->join();
    background_commit->join();
    background_apply->join();
    thread_pool->destroy();
}

template <typename state_machine, typename command>
bool raft<state_machine, command>::is_stopped() {
    return stopped.load();
}

template <typename state_machine, typename command>
bool raft<state_machine, command>::is_leader(int &term) {
    term = current_term;
    return role == leader;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::start() {
    // Lab3: Your code here
    RAFT_LOG("start");
    this->background_election = new std::thread(&raft::run_background_election, this);
    this->background_ping = new std::thread(&raft::run_background_ping, this);
    this->background_commit = new std::thread(&raft::run_background_commit, this);
    this->background_apply = new std::thread(&raft::run_background_apply, this);
}

template <typename state_machine, typename command>
bool raft<state_machine, command>::new_command(command cmd, int &term, int &index) {
    // Lab3: Your code here
    mtx.lock();
    if(role == leader) {        
      if(log[current_term].size() == 0 && my_state_info->last_added_term != current_term) {
        my_state_info->last_added_term = current_term;
        my_state_info->last_added_index = 0;
      }
      log_entry<command> cd;
      cd.cmd = cmd;
      cd.index = my_state_info->last_added_index + 1;
      cd.term = current_term;
      term = current_term;
      index = log_size + get_size()+ 1;
      log[current_term].push_back(cd);
      my_state_info->last_added_index++;
      storage->append_log_entry1(current_term,my_id,committed_term,committed_index,my_state_info->last_added_term,my_state_info->last_added_index,log,get_size(),false,my_id);
      mtx.unlock();
      return true;
    }
    mtx.unlock();
    return false;
}

template <typename state_machine, typename command>
bool raft<state_machine, command>::save_snapshot() {
    // Lab3: Your code here
    return true;
}

/******************************************************************

                         RPC Related

*******************************************************************/
template <typename state_machine, typename command>
int raft<state_machine, command>::request_vote(request_vote_args args, request_vote_reply &reply) {
    // Lab3: Your code here
    mtx.lock();
    if((my_state_info->last_added_term > args.last_added_term) || ((my_state_info->last_added_term == args.last_added_term) && (my_state_info->last_added_index > args.last_added_index))) {
        reply.vote_result = false;
        if(args.term > current_term) {
            role = follower;
            start_new_term(args.term);
        }
    } else {
        if((vote->term == args.term && vote->vs == vote_data::UNVOTED) || args.term > vote->term) {
        storage->append_log_entry1(args.term,args.server_id,committed_term,committed_index,my_state_info->last_added_term,my_state_info->last_added_index,log,get_size(),false,my_id);
        role = follower;
        vote->server_id = args.server_id;
        leader_id = args.server_id;
        vote->vs = vote_data::VOTED;
        leader_acknowledgement = true;
        vote->term = args.term;
        current_term = args.term;
        reply.vote_result = true;
        ping = false;
        } else  {
            reply.vote_result = false;
        }
    }
    reply.term = current_term;
    reply.last_added_index = my_state_info->last_added_index;
    reply.last_added_term = my_state_info->last_added_term;
    reply.applied_term = my_state_info->applied_term;
    reply.applied_index = my_state_info->applied_index;
    mtx.unlock();
    return 0;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::handle_request_vote_reply(int target, const request_vote_args &arg, const request_vote_reply &reply) {
    // Lab3: Your code here
    mtx.lock();
    if(reply.vote_result) {
        if(current_term == arg.term && (role == candidate || role == leader)) {
            update_other_servers_term_data(target,reply);
            followers_db.insert(target);
            if(followers_db.size() >= num_nodes()/2+1 && role == candidate) {
                role = leader;
                leader_id = my_id;
                log[current_term] = std::vector<log_entry<command>>(0);
                snapshot_timer = chrono::system_clock::now();
                if(reply.applied_term > committed_term || (reply.applied_term == committed_term && reply.applied_index > committed_index)) {
                    committed_term = reply.applied_term;
                    committed_index = reply.applied_index;
                
                }
                storage->append_log_entry1(current_term,leader_id,committed_term,committed_index,my_state_info->last_added_term,my_state_info->last_added_index,log,get_size(),true,my_id);
                initialize_servers_terms_data_invalid();
            } else {
                if(reply.applied_term > committed_term || (reply.applied_term == committed_term && reply.applied_index > committed_index)) {
                    committed_term = reply.applied_term;
                    committed_index = reply.applied_index;
                    storage->append_log_entry1(current_term,leader_id,committed_term,committed_index,my_state_info->last_added_term,my_state_info->last_added_index,log,get_size(),true,my_id);
                }
            }
        }
    } else{
        if(reply.term > current_term) {
            role = follower;
            start_new_term(reply.term);
        }
    }
    mtx.unlock();
    return;
}

template <typename state_machine, typename command>
int raft<state_machine, command>::append_entries(append_entries_args<command> arg, append_entries_reply &reply) {
    // Lab3: Your code here
    mtx.lock();
    if(vote->server_id == arg.server_id && vote->term == arg.current_term) {
        reply.success = true;
        leader_acknowledgement = true;
        if(arg.commands_list.size() !=  0) {
            if(is_valid_previous_index_term(arg.previous_term,arg.previous_index)) {
                if(arg.previous_term > my_state_info->applied_term || (my_state_info->applied_term == arg.previous_term &&  arg.previous_index >= my_state_info->applied_index)) {
                    sync_log_with_leader(arg);
                    ping = true;
                }
            }
        }
        if(ping == true) {
            if(arg.committed_term > committed_term || (arg.committed_term == committed_term && arg.committed_index > committed_index)) {
                committed_index = arg.committed_index;
                committed_term = arg.committed_term;
            }
        }
    } else {
        reply.success = false;
    }
    reply.append_entries_result = ping;
    reply.term = current_term;
    reply.last_added_index = my_state_info->last_added_index;
    reply.last_added_term = my_state_info->last_added_term;
    mtx.unlock();
    return 0;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::handle_append_entries_reply(int node, const append_entries_args<command> &arg, const append_entries_reply &reply) {
    // Lab3: Your code here
    mtx.lock();
    if(reply.success && role == leader) {
        followers_db.insert(node);
        servers_state_infromation *t = servers_state_info_db[node];
        if(!reply.append_entries_result) {
            if(arg.commands_list.size() > 0) {
                pair<int,int> p = get_previous_term_and_index(arg.previous_term,arg.previous_index);
                t->last_added_term = p.first;
                t->last_added_index = p.second;
            }
        } else {
            if(!t->is_valid) {
                t->is_valid = true;
            }
            t->last_added_index = reply.last_added_index;
            t->last_added_term = reply.last_added_term;
        }
        vector<int> v;
        vector<int> v1;
        int count = 0;
        for(int i=0; i<rpc_clients.size();i++) { 
            servers_state_infromation *t1 = servers_state_info_db[i];
            if(my_id != i  &&  t1->is_valid) {
                v.push_back(t1->last_added_term);
            } else if(my_id == i) {
                v.push_back(my_state_info->last_added_term);
            } else {
                v.push_back(t1->applied_term);
            }
        }
        sort(v.begin(),v.end());
        int majority_term  = v[v.size()/2];
        if(majority_term == current_term) {
            for(int i=0; i<rpc_clients.size();i++) {
                servers_state_infromation *t1 = servers_state_info_db[i];
                if(t1->last_added_term >= majority_term) {
                    count++;
                    v1.push_back(t1->last_added_index);
                } else {
                    v1.push_back(0);
                }
            }
            if(count >= num_nodes()/2) {
                sort(v1.begin(),v1.end());
                committed_term = majority_term;
                committed_index = v1[v1.size()/2];
                //cout<<committed_term<<"    "<<committed_index<<"nmm"<<my_id<<"\n";
            }   
        }   
      // }
    } else {
        if(reply.term > current_term) {
            role = follower;
            start_new_term(reply.term);
        }
    }
    mtx.unlock();
    return;
}


template <typename state_machine, typename command>
int raft<state_machine, command>::install_snapshot(install_snapshot_args args, install_snapshot_reply &reply) {
    // Lab3: Your code here
    mtx.lock();
    if(vote->server_id == args.leaderId && vote->term == args.term) {
        if(args.lastIncludedTerm > my_state_info->last_added_term || (args.lastIncludedTerm == my_state_info->last_added_term && args.lastIncludedIndex > my_state_info->last_added_index)) {
            snapshot_data = args.data;
            log.clear();
            std::vector<char*> v2;
            for(int i=0;i<args.data1.size();i++) {
                char c1 = args.data1[i];
                v2.push_back(&c1);
            }
            list = v2;
            int_list = args.data2;
            log_size = args.log_size;
            my_state_info->last_added_term = args.lastIncludedTerm;
            my_state_info->last_added_index = args.lastIncludedIndex;
            my_state_info->applied_term = args.lastIncludedTerm;
            my_state_info->applied_index = args.lastIncludedIndex;
            committed_term = my_state_info->applied_term;
            committed_index = my_state_info->applied_index;
            my_state_info->snapshot_included_term = args.lastIncludedTerm;
            my_state_info->snapshot_included_index = args.lastIncludedIndex;
            storage->make_snapshot(args.log_size,my_state_info->applied_term,my_state_info->applied_index,snapshot_data,v2,args.data2);
           // cout<<"snaaaaaaaaaaaaaaaaaaaaakdkd"<<my_state_info->last_added_index<<" \n";
            storage->append_log_entry1(current_term,leader_id,committed_term,committed_index,my_state_info->last_added_term,my_state_info->last_added_index,log,0,false,my_id);
            state->apply_snapshot(snapshot_data);  
           state->apply_snapshot1(list,int_list);
        } else if(args.lastIncludedTerm > my_state_info->applied_term || (args.lastIncludedTerm == my_state_info->applied_term && args.lastIncludedIndex > my_state_info->applied_index)) {
            
            snapshot_data = args.data;
            log_size = args.log_size;
            std::vector<char*> v2;
            for(int i=0;i<args.data1.size();i++) {
                char c1 = args.data1[i];
                v2.push_back(&c1);
            }
            for(int i=1;i<=args.lastIncludedTerm;i++) {
                if(log.find(i) != log.end()) {
                    if(i == args.lastIncludedTerm) {
                        vector<log_entry<command>> v;
                        vector<log_entry<command>> v1 = log[i];
                        for(int i1=0;i1<v1.size();i1++) {
                            log_entry<command> lg = v1[i1];
                            if(lg.index > args.lastIncludedIndex) {
                                v.push_back(lg);
                            }
                        }
                        log[i].clear();
                        log[i] = v;
                    } else {
                        log[i].clear();
                    }
                }
            }
            list = v2;
            int_list = args.data2;
            my_state_info->applied_term = args.lastIncludedTerm;
            my_state_info->applied_index = args.lastIncludedIndex;
            committed_term = my_state_info->applied_term;
            committed_index = my_state_info->applied_index;
            my_state_info->snapshot_included_term = my_state_info->applied_term;
            my_state_info->snapshot_included_index = my_state_info->applied_index;
            storage->make_snapshot(log_size,my_state_info->applied_term,my_state_info->applied_index,snapshot_data,v2,args.data2);
            storage->append_log_entry1(current_term,leader_id,my_state_info->applied_term,my_state_info->applied_index,my_state_info->last_added_term,my_state_info->last_added_index,log,get_size(),false,my_id);
            state->apply_snapshot(snapshot_data);
            state->apply_snapshot1(list,int_list);
        }
        reply.result = true;
    } else {
        reply.result = false;
    }
    reply.term = current_term;
    reply.last_added_term = my_state_info->last_added_term;
    reply.last_added_index = my_state_info->last_added_index;
    mtx.unlock();
    return 0;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::handle_install_snapshot_reply(int node, const install_snapshot_args &arg, const install_snapshot_reply &reply) {
    // Lab3: Your code here
    mtx.lock();
    if(reply.result) {
        servers_state_infromation *t = servers_state_info_db[node];
        t->last_added_index = reply.last_added_index;
        t->last_added_term = reply.last_added_term;
        t->is_valid = true;
    } else {
        if(reply.term > current_term) {
            role = follower;
            start_new_term(reply.term);
        }
    }
    mtx.unlock();
    return;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::send_request_vote(int target, request_vote_args arg) {
    request_vote_reply reply;
    if (rpc_clients[target]->call(raft_rpc_opcodes::op_request_vote, arg, reply) == 0) {
        handle_request_vote_reply(target, arg, reply);
    } else {
        // RPC fails
    }
}
template <typename state_machine, typename command>
void raft<state_machine, command>::send_append_entries(int target, append_entries_args<command> arg) {
    append_entries_reply reply;
    if (rpc_clients[target]->call(raft_rpc_opcodes::op_append_entries, arg, reply) == 0) {
        handle_append_entries_reply(target, arg, reply);
    } else {
        // RPC fails
    }
}

template <typename state_machine, typename command>
void raft<state_machine, command>::send_install_snapshot(int target, install_snapshot_args arg) {
    install_snapshot_reply reply;
    if (rpc_clients[target]->call(raft_rpc_opcodes::op_install_snapshot, arg, reply) == 0) {
        handle_install_snapshot_reply(target, arg, reply);
    } else {
        // RPC fails
    }
}

/******************************************************************

                        Background Workers

*******************************************************************/

template <typename state_machine, typename command>
void raft<state_machine, command>::run_background_election() {
    // Periodly check the liveness of the leader.
    // Work for followers and candidates.
   
    while (true) {
        if (is_stopped()) return;
    // Lab3: Your code here
        mtx.lock();
        if(role == follower || role == candidate) {
            if(leader_acknowledgement) {
                leader_acknowledgement = false;
                mtx.unlock();
                this_thread::sleep_for(chrono::milliseconds(300));
            } else {
                int lb = 10, ub = 200;
                int timeout = (rand() % (ub - lb + 1)) + lb;
                mtx.unlock();
                this_thread::sleep_for(chrono::milliseconds((timeout)));
                mtx.lock();
                if(vote->vs == vote_data::UNVOTED) {
                    role = candidate;
                    followers_db.clear();
                    vote->server_id = my_id;
                    vote->vs = vote_data::VOTED;
                    followers_db.insert(my_id);
                    start_election();
                    mtx.unlock();
                    this_thread::sleep_for(chrono::milliseconds(300));
                    mtx.lock();
                }
                if((role == follower && !leader_acknowledgement) || role == candidate) {
                    current_term++;
                    vote->vs = vote_data::UNVOTED;
                    vote->term = current_term;
                }
                mtx.unlock();
            }
        } else{
            mtx.unlock();
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    }
    return;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::run_background_commit() {
    // Periodly send logs to the follower.
    // Only work for the leader.
    while (true) {
        if (is_stopped()) return;
        mtx.lock();
        if(role == leader) {
            for(int i1=0; i1<rpc_clients.size();i1++) {
                if(my_id != i1 && followers_db.find(i1) != followers_db.end()) {
                   append_entries_args<command> a;
                   a.current_term = current_term;
                   a.server_id = my_id;
                   a.committed_term = my_state_info->applied_term;
                   a.committed_index = my_state_info->applied_index;
                   servers_state_infromation *t = servers_state_info_db[i1];
                   if(my_state_info->snapshot_included_term > t->last_added_term || (my_state_info->snapshot_included_term == t->last_added_term && my_state_info->snapshot_included_index > t->last_added_index)) {
                        install_snapshot_args is;
                        create_snapshot_install(is);
                        thread_pool->addObjJob(this, &raft::send_install_snapshot, i1, is);
                   }
                   
                   else {
                       if(is_valid_previous_index_term(t->last_added_term,t->last_added_index)) {
                      a.previous_index = t->last_added_index;
                      a.previous_term = t->last_added_term;
                   } else {
                      pair<int,int> p = get_previous_term_and_index(t->last_added_term,t->last_added_index);
                      a.previous_term = p.first;
                      a.previous_index = p.second;
                   }
                    pair<int,int> p = get_next_term_and_index(a);
                    int next_term = p.first;
                    int next_index = p.second;
                    for(int i=next_term;i<=current_term;i++) {
                        if(log.find(i) != log.end()) {
                            vector<log_entry<command>> v = log[i];
                            int k = 0;
                            if (i == next_term) {
                                if(v.size() > 0) {
                                    log_entry<command> lg = v[0];
                                    k = next_index - lg.index + 1;
                                }
                            }
                            for(int j=k;j<v.size();j++) {
                                a.commands_list.push_back(v[j]);
                            }
                        }
                    }
                    if(a.commands_list.size() != 0) {
                        thread_pool->addObjJob(this, &raft::send_append_entries, i1, a);
                    } 
                   }
                }
            }
        }
        mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(30));
        //Lab3: Your code here
    }
    return;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::run_background_apply() {
    // Periodly apply committed logs the state machine
    // Work for all the nodes.
    while (true) {
        if (is_stopped()) return;
        // Lab3: Your code here:
        mtx.lock();
        for(int i=my_state_info->applied_term;i<=committed_term;i++) {
            bool y = true;
            if(log.find(i) != log.end()) {
                int size = log[i].size();
                int k1=0,k=size;
                if(i== my_state_info->applied_term) {
                    if(size > 0) {
                        log_entry<command> lg = log[i][0];
                        k1 = my_state_info->applied_index - lg.index + 1; 
                    }
                }
                if(size > 0) {
                    log_entry<command> lg = log[i][size-1];
                    k = lg.index;
                }
                if(i == committed_term) {
                    k = committed_index;
                }
                for(int j=k1;j<log[i].size();j++) {  
                log_entry<command> c = log[i][j];
                if(c.index <= k) {
                state->apply_log(c.cmd);
                my_state_info->applied_term = c.term;
                my_state_info->applied_index = c.index;
                 } else {
                    y = false;
                    break;
                }
                }
            }
            if(!y) {
                break;
            }
        }
        if(chrono::system_clock::now() - snapshot_timer > chrono::milliseconds(500)) {
            snapshot_timer = chrono::system_clock::now();
            create_snapshot();
        }
        mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(30));
    }
    return;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::run_background_ping() {
    // Periodly send empty append_entries RPC to the followers.
    // Only work for the leader.
    while (true) {
        if (is_stopped()) return;
        // Lab3: Your code here:
        mtx.lock();
        if(role == raft::leader) {
            for(int i=0; i<rpc_clients.size();i++) {
                if(my_id != i ) {
                   append_entries_args<command> a;
                   vector<log_entry<command>> v;
                   a.current_term = current_term;
                   a.server_id = my_id;
                   a.committed_index = committed_index;
                   a.committed_term = committed_term;
                   a.commands_list = v;
                   thread_pool->addObjJob(this, &raft::send_append_entries, i, a);
                }
            }
        }
        mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(150));
    }
    return;
}

/******************************************************************

                        Other functions

*******************************************************************/


template <typename state_machine, typename command>
void raft<state_machine, command>::start_election() {
    for(int i=0; i<num_nodes();i++) {
        if(my_id != i) {
            request_vote_args r;
            r.term = current_term;
            r.server_id = my_id;
            r.last_added_index = my_state_info->last_added_index;
            r.last_added_term = my_state_info->last_added_term;
            thread_pool->addObjJob(this, &raft::send_request_vote, i, r);
       }
    }
}

template <typename state_machine, typename command>
void raft<state_machine, command>::sync_log_with_leader(append_entries_args<command> arg) {
        for(int i = arg.previous_term; i<=current_term;i++) {
            if(log.find(i) != log.end()) {
                if(i == arg.previous_term) {
                    while (!log[i].empty()) {
                      log_entry<command>it  = log[i].back();
                      if(it.index > arg.previous_index) {
                        log[i].pop_back();
                      } else {
                            break;
                       }
                    }
                } else {
                    log[i].clear();
                }
            }
        }
       
        for(int i=0; i<arg.commands_list.size();i++) {
            log_entry<command> c = arg.commands_list[i];
            if(log.find(i) == log.end()) {
              log[i] = std::vector<log_entry<command>>(0);;
            }
            log[c.term].push_back(c);
            my_state_info->last_added_index = c.index;
            my_state_info->last_added_term = c.term;
        }
        storage->append_log_entry1(current_term,leader_id,committed_term,committed_index,my_state_info->last_added_term,my_state_info->last_added_index,log,get_size(),false,my_id);
}


template <typename state_machine, typename command>
void raft<state_machine, command>::update_other_servers_term_data(int target,const request_vote_reply &reply) {
    servers_state_infromation *t = servers_state_info_db[target];
    t->last_added_index = reply.last_added_index;
    t->last_added_term = reply.last_added_term;
    t->applied_term = reply.applied_term;
    t->applied_index = reply.applied_index;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::initialize_servers_terms_data() {
    for(int i=0; i<rpc_clients.size();i++) {
        servers_state_infromation *t = new servers_state_infromation();
        servers_state_info_db[i] = t;
        t->id = i;
        t->last_added_index = 0;
        t->last_added_term = 0;
        t->applied_index = 0;
        t->applied_term = 0;
        t->is_valid = false;
        t->snapshot_included_index = 0;
        t->snapshot_included_term = 0;
    }
}

template <typename state_machine, typename command>
void raft<state_machine, command>::initialize_servers_terms_data_invalid() {
    for(int i=0; i<rpc_clients.size();i++) {
        if(i != my_id) {
            servers_state_infromation *t = servers_state_info_db[i];
            t->is_valid = false;
        }  
    }
}

template <typename state_machine, typename command>
pair<int,int> raft<state_machine, command>::get_previous_term_and_index(int term,int index) {
    if(log.find(term) != log.end()) {
        int size =  log[term].size();
        if(size > 0) {
            log_entry<command> lg = log[term][0];
            if(size + lg.index - 1 >= index - 1 && index-1 > 0) {
                return make_pair(term,index - 1);
            } else if(index -1 > 0) {
                return make_pair(term, log[term][size-1].index);
            }
        }
    }
    int trm = term - 1;
    while (trm > 0) {
        if(log.find(trm) != log.end()) {
            if(log[trm].size() != 0) {
                int size =  log[trm].size();
                log_entry<command> lg1 = log[trm][size-1];
                return make_pair(trm,lg1.index);
            }
        }
        trm--;
    }
    return make_pair(my_state_info->snapshot_included_term,my_state_info->snapshot_included_index);
}


template <typename state_machine, typename command>
pair<int,int> raft<state_machine, command>::get_next_term_and_index(append_entries_args<command> arg) {
    if(log.find(arg.previous_term) != log.end()) {
        int size = log[arg.previous_term].size();
        if(size > 0) {
           log_entry<command> lg = log[arg.previous_term][0];
           if(size + lg.index - 1 >= arg.previous_index + 1) {
              return make_pair(arg.previous_term,arg.previous_index);
           }
        }
    }
    int trm = arg.previous_term+1;
    while(true) {
        if(trm > current_term) {
            return make_pair(trm,0);
        }
        if(log.find(trm) != log.end()) {
            int size = log[trm].size();
            if(size != 0) {
                return make_pair(trm,0);
            }
        }
        trm++;  
    }
}

template <typename state_machine, typename command>
void raft<state_machine, command>::start_new_term(int term) {
   current_term = term;
   vote->vs = vote_data::UNVOTED;
   followers_db.clear();
}

template <typename state_machine, typename command>
int raft<state_machine, command>::get_size() {
  int sz = 0;
  for(int i=1;i<=current_term;i++) {
      if(log.find(i) != log.end()) {
           sz = sz + log[i].size();
      }
  }
  return sz;
}

template <typename state_machine, typename command>
bool raft<state_machine, command>::is_valid_previous_index_term(int term,int index) {
    if(term == 0 && index == 0) {
        return true;
    } 
    if(my_state_info->snapshot_included_term == term && my_state_info->snapshot_included_index == index) {
        return true;
    }
    if(log.find(term) != log.end()) {
        if(log[term].size() > 0) {
            log_entry<command> lg = log[term][0];
            if(log[term].size() + lg.index - 1 >= index) {
                return true;
            }
        }
    }
    return false;
}

template <typename state_machine, typename command>
void raft<state_machine, command>::create_snapshot() {
   if(my_state_info->applied_term > my_state_info->snapshot_included_term || (my_state_info->snapshot_included_term == my_state_info->applied_term && my_state_info->applied_index > my_state_info->snapshot_included_index)) {
   snapshot_data = state->snapshot();
   int s1 = 0;
   for(int i=1;i<=my_state_info->applied_term;i++) {
        if(log.find(i) != log.end()) {
            if(i == my_state_info->applied_term) {
                vector<log_entry<command>> v;
                vector<log_entry<command>> v1 = log[i];
                for(int i1=0;i1<v1.size();i1++) {
                    log_entry<command> lg = v1[i1];
                    if(lg.index > my_state_info->applied_index) {
                        v.push_back(lg);
                    } else {
                        int se = lg.cmd.size();
                        char* buf = new char[se];
                        lg.cmd.serialize(buf, se);
                        list.push_back(buf);
                        int_list.push_back(se);
                    }
                }
                s1 = s1 + log[i].size() - v.size();
                log[i].clear();
                log[i] = v;
            } else {
                s1 = s1 + log[i].size();
                for(int i1 = 0; i1<log[i].size();i1++) {
                    log_entry<command> lg = log[i][i1];
                    int se = lg.cmd.size();
                    char* buf = new char[se];
                    lg.cmd.serialize(buf, se);
                    list.push_back(buf);
                    int_list.push_back(se);
                }
                log[i].clear();
            }
        }
    }
    log_size = log_size + s1;
    pair<vector<char*>,vector<int>> p = state->snapshot1(list,int_list);
    list = p.first;
    int_list = p.second;
    int temp;
    bool isLeader = is_leader(temp);
    storage->make_snapshot(log_size,my_state_info->applied_term,my_state_info->applied_index,snapshot_data,p.first,p.second);
    storage->append_log_entry1(current_term,leader_id,my_state_info->applied_term,my_state_info->applied_index,my_state_info->last_added_term,my_state_info->last_added_index,log,get_size(),isLeader,my_id);
    my_state_info->snapshot_included_term = my_state_info->applied_term;
    my_state_info->snapshot_included_index = my_state_info->applied_index;
    }
}

template <typename state_machine, typename command>
void raft<state_machine, command>::create_snapshot_install(install_snapshot_args& i) {
   i.term = current_term;
   i.lastIncludedTerm = my_state_info->snapshot_included_term;
   i.lastIncludedIndex = my_state_info->snapshot_included_index;
   i.leaderId = my_id;
   i.data = snapshot_data;
   i.offset = 0;
   i.log_size = log_size;
   vector<char> c8;
   for(int i=0; i<list.size();i++) {
      char * c = list[i];
      char *c1 = new char[int_list[i]];
      memcpy(c1,c,int_list[i]);
      c8.push_back(*c1);
   }
   i.data1 = c8;
   i.data2 = int_list;
}
#endif // raft_h
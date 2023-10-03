#ifndef persister_h
#define persister_h

#include <fcntl.h>
#include <mutex>
#include <iostream>
#include <fstream>
#include "rpc.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>

using namespace std;

#define MAX_LOG_SZ 131072

/*
 * Your code here for Lab2A:
 * Implement class chfs_command, you may need to add command types such as
 * 'create', 'put' here to represent different commands a transaction requires. 
 * 
 * Here are some tips:
 * 1. each transaction in ChFS consists of several chfs_commands.
 * 2. each transaction in ChFS MUST contain a BEGIN command and a COMMIT command.
 * 3. each chfs_commands contains transaction ID, command type, and other information.
 * 4. you can treat a chfs_command as a log entry.
 */
class chfs_command {
public:
    typedef unsigned long long txid_t;
    enum cmd_type {
        CMD_BEGIN = 0,
        CMD_COMMIT,
        CMD_OPERATION
    };

    enum operation_type {
        NONE = 0,
        CREATE_FILE,
        MAKE_DIR,
        WRITE,
        UNLINK,
        SYMLINK,
        PUT,
        REMOVE
    };
    cmd_type type = CMD_BEGIN;
    operation_type op = NONE;
    txid_t id = 0;
    extent_protocol::extentid_t eid = 0;
    string data;

    // constructor
    chfs_command() {}


    // chfs_command(txid_t n, cmd_type t, extent_protocol::extentid_t i, string &p,operation_type op1) {
    //     id = n;
    //     type = t;
    //     eid = i;
    //     data = p;
    //     op = op1;
    // }

    // uint64_t size() const {
    //     uint64_t s = sizeof(cmd_type) + sizeof(txid_t) + sizeof(operation_type) + sizeof(eid) + data.size();
    //     return s;
    // }
};


struct Transaction {
    vector<chfs_command> transaction_commands;  
};


/*
 * Your code here for Lab2A:
 * Implement class persister. A persister directly interacts with log files.
 * Remember it should not contain any transaction logic, its only job is to 
 * persist and recover data.
 * 
 * P.S. When and how to do checkpoint is up to you. Just keep your logfile size
 *      under MAX_LOG_SZ and checkpoint file size under DISK_SIZE.
 */
template<typename command>
class persister {

public:
    persister(const std::string& file_dir);
    ~persister();
    map<chfs_command::txid_t,long long> transaction_map1;
    // persist data into solid binary file
    // You may modify parameters in these functions
    void append_log(const command& log);
    void checkpoint(long long);
    void clear_log_entries();
    void clear_check_point_entries();
    // restore data from solid binary file
    // You may modify parameters in these functions
    void restore_logdata();
    void restore_checkpoint();
    std::vector<command> get_log_entries();
    std::vector<command> get_check_point_entries();
    std::string to_string(chfs_command::txid_t, chfs_command::cmd_type ,chfs_command::operation_type , extent_protocol::extentid_t,string);
    long long log_size = 0;
    std::map<chfs_command::txid_t,bool> t_map;
    std::map<extent_protocol::extentid_t,string> db;

private:
    std::mutex mtx;
    std::string file_dir;
    std::string file_path_checkpoint;
    std::string file_path_logfile;

    // restored log data
    std::vector<command> log_entries;
    std::vector<command> check_point_entries;
};

template<typename command>
persister<command>::persister(const std::string& dir){
    // DO NOT change the file names here
    file_dir = dir;
    file_path_checkpoint = file_dir + "/checkpoint.bin";
    file_path_logfile = file_dir + "/logdata.bin";
}

template<typename command>
persister<command>::~persister() {
    // Your code here for lab2A
}


template<typename command>
std::string persister<command>::to_string(chfs_command::txid_t id, chfs_command::cmd_type type,chfs_command::operation_type op, extent_protocol::extentid_t eid,string data) {
    string buf = "^",buf1 = "/";
    std::ostringstream os1,os2,os3,os4;
    os1 << id;
    os2 << type;
    os3 << op;
    os4 << eid;
    string os = buf + os1.str()  + "!" + os2.str() + "!" + os3.str() + "!"+ os4.str() + "!" + data + "!" + buf;
    return os;
}

template<typename command>
void persister<command>::append_log(const command& log) {
    // Your code here for lab2A
    std::ofstream file;
    file.open(file_path_logfile, std::ios::app|std::ios::binary);
    if (file.is_open()){
        string os = to_string(log.id,log.type,log.op,log.eid,log.data);
        file.write(os.c_str(),os.length());
        log_entries.push_back(log);
        log_size = log_size + os.length();
        if(log.type == chfs_command::CMD_BEGIN) {
            t_map[log.id] = log_entries.size();
        } else if (log.type == chfs_command::CMD_COMMIT) {
            t_map.erase(log.id);
        }
        if(log_size > MAX_LOG_SZ) {
            if(t_map.size() != 0) {
                std::map<chfs_command::txid_t,bool> t_map1;
                for (auto& i  : t_map) {
                    checkpoint(i.second-1);
                    vector<chfs_command>::iterator it1, it2;
                    it1 = log_entries.begin();
                    it2 = log_entries.end();
                    log_entries.erase(it1,it1+i.second-1);
                    for(int i1=0; i1<log_entries.size();i1++) {
                        chfs_command ch2 = log_entries[i1];
                       if(ch2.type == chfs_command::CMD_BEGIN) {
                          t_map1[ch2.id] = log_entries.size();
                        } else if (ch2.type == chfs_command::CMD_COMMIT) {
                          t_map1.erase(ch2.id);
                        }
                    }
                    break;
                }
                t_map.clear();
                for (auto &entry: t_map1) {
                  t_map[entry.first] = entry.second;
                }
            } else {
               // cout<<"helllooooooooooooooo"<<"\n";
                checkpoint(log_entries.size()-1);
                vector<chfs_command>::iterator it1, it2;
                it1 = log_entries.begin();
                it2 = log_entries.end();
                log_entries.erase(it1,it2);
            }
        }
    }
    file.close();
}

template<typename command>
void persister<command>::checkpoint(long long end) {
    // Your code here for lab2A
    if(log_size < MAX_LOG_SZ) {
        return;
    }
    ifstream in_file(file_path_logfile, ios::binary);
    in_file.seekg(0, ios::end);

    std::vector<command> committed_entries;
    for(int i=0;i<=end;i++) {
        chfs_command ch = log_entries[i];
        if(ch.op == chfs_command::CREATE_FILE || ch.op == chfs_command::MAKE_DIR
            || ch.op == chfs_command::SYMLINK) {
            committed_entries.push_back(ch);
         }
        else if(ch.op == chfs_command::PUT) {  
          db[ch.eid]  = ch.data;
        } else if (ch.op == chfs_command::REMOVE){
          committed_entries.push_back(ch);
          db.erase(ch.eid);
        }
    }
    std::ofstream file;
    file.open(file_path_checkpoint, std::ios::app|std::ios::binary);
    if (file.is_open()){
        string st1;
        for(chfs_command& ch1 : committed_entries) {
            st1  = st1 + to_string(ch1.id,ch1.type,ch1.op,ch1.eid,ch1.data);
        }
        for (auto& i1  : db) {
           st1  = st1 + to_string(0,chfs_command::CMD_OPERATION,chfs_command::PUT,i1.first,i1.second);
        }
        file.write(st1.c_str(),st1.length());
    }
    file.close();
    std::ofstream ofs;
    ofs.open(file_path_logfile, std::ofstream::out | std::ofstream::trunc);
     if (ofs.is_open()){
        string st;
        for(int i=end+1;i<log_entries.size();i++) {
            chfs_command ch2 = log_entries[i];
            st = st + to_string(ch2.id,ch2.type,ch2.op,ch2.eid,ch2.data);
        }
        ofs.write(st.c_str(),st.length());
        log_size = st.length();
    }
    ofs.close();
}

template<typename command>
void persister<command>::restore_logdata() {
    // Your code here for lab2A
    std::ifstream file;
    file.open(file_path_logfile, std::ios::app|std::ios::binary);
    map<chfs_command::txid_t,int> transaction_map1;
    if (file.is_open()){
        std::string str((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        string st = "^",st1 = "!";
        chfs_command ch;
        for(int i=0;i<str.length();i++) {
           if(str[i] == st[0]) {
              int j = i+1;
              for(int k = 0;k<5;k++) {
                string buf;
                for( int j1 = j;j1<str.length();j1++) {
                    if(str[j1] == st1[0]) {
                     j = j1+1;   
                     break;
                    } else {
                       buf.push_back(str[j1]);
                    }
                }
                if(k == 0) {
                    ch.id =  static_cast<chfs_command::txid_t>(stoi(buf));
                } else if(k == 1) {
                    ch.type = static_cast<chfs_command::cmd_type>(stoi(buf));
                } else if(k == 2) {
                    ch.op = static_cast<chfs_command::operation_type>(stoi(buf));
                } else if(k== 3){
                    ch.eid = static_cast<extent_protocol::extentid_t>(stoi(buf));
                } else if(k== 4){
                    ch.data = buf;
                }
                buf.clear();
              }
              log_entries.push_back(ch);
              if(ch.type == chfs_command::CMD_BEGIN) {
                transaction_map1[ch.id] = log_entries.size();
              } else if (ch.type == chfs_command::CMD_COMMIT) {
                transaction_map1.erase(ch.id);
              }
              i = j;
           }
        }
    }
    file.close();


    for (auto& i  : transaction_map1) {
        vector<chfs_command>::iterator it1, it2;
        it1 = log_entries.begin();
        it2 = log_entries.end();
        log_entries.erase(it1+i.second-1,it2);
        break;
    }
};


template<typename command>
void persister<command>::restore_checkpoint() {
    // Your code here for lab2A
    std::ifstream file;
    file.open(file_path_checkpoint, std::ios::app|std::ios::binary);
    if (file.is_open()){
        std::string str((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        string st = "^",st1 = "!";
        chfs_command ch;
        //cout<<str.length();
        for(int i=0;i<str.length();i++) {
           if(str[i] == st[0]) {
              int j = i+1;
              for(int k = 0;k<5;k++) {
                string buf;
                for( int j1 = j;j1<str.length();j1++) {
                    if(str[j1] == st1[0]) {
                     j = j1+1;   
                     break;
                    } else {
                       buf.push_back(str[j1]);
                    }
                }
                if(k == 0) {
                    ch.id =  static_cast<chfs_command::txid_t>(stoi(buf));
                } else if(k == 1) {
                    ch.type = static_cast<chfs_command::cmd_type>(stoi(buf));
                } else if(k == 2) {
                    ch.op = static_cast<chfs_command::operation_type>(stoi(buf));
                } else if(k== 3){
                    ch.eid = static_cast<extent_protocol::extentid_t>(stoi(buf));
                } else if(k== 4){
                    ch.data = buf;
                }
               // cout<<buf<<" ";
                buf.clear();
              }
              check_point_entries.push_back(ch);
              if(ch.op == chfs_command::PUT) {  
                db[ch.eid]  = ch.data;
              } else if(ch.op == chfs_command::REMOVE) {
                db.erase(ch.eid);
              }
              i = j;
           }
        }
    }
    file.close();
};

template<typename command>
std::vector<command> persister<command>::get_log_entries(){
    return log_entries;
}

template<typename command>
std::vector<command> persister<command>::get_check_point_entries(){
    return check_point_entries;
}


template<typename command>
void persister<command>::clear_check_point_entries(){
    check_point_entries.clear();
}

template<typename command>
void persister<command>::clear_log_entries(){
    log_entries.clear();
}


using chfs_persister = persister<chfs_command>;
#endif // persister_h

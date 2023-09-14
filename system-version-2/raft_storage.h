#ifndef raft_storage_h
#define raft_storage_h

#include "raft_protocol.h"
#include <fcntl.h>
#include <mutex>
#include <fstream>
#include <unordered_map>

template <typename command>
class raft_storage {
public:
    raft_storage(const std::string &file_dir);
    ~raft_storage();
    // Lab3: Your code here
    void append_log_entry1(int current_term,int leader_id,int committed_term,int committed_index,int last_added_term,int last_added_index,std::map<int,vector<log_entry<command>>> &log,int,bool,int);
    void restore_log1(int& ,int&,int&,int&,int &,int &,std::map<int,vector<log_entry<command>>>& log,bool &,bool&,int);
    void make_snapshot(int size,int last_included_term,int last_included_index,vector<char> data,vector<char*> char_list,vector<int>);
    void restore_snapshot(int& log_size,int& snapshot_included_term,int& snapshot_included_index,vector<char>& data,vector<char*> &list,vector<int> & list1);


private:
    std::string file_path_logfile;
    std::string file_path_snapshotfile;
    std::string file_path_logfile1;
    std::string file_path_logfile2;
    std::mutex mtx;
  //  vector<log_entry<command>> log_entry_list;
     std::map<int,vector<log_entry<command>>> log_entry_map;
    // Lab3: Your code here
};

template <typename command>
raft_storage<command>::raft_storage(const std::string &dir) {
    // Lab3: Your code here
    file_path_logfile = dir + "/logdata.bin";
    file_path_snapshotfile = dir + "/snapshot.bin";
    file_path_logfile1 = "./logdata.bin";
    file_path_logfile2 = "./logdata1.bin";
}

template <typename command>
raft_storage<command>::~raft_storage() {
    // Lab3: Your code here
}


template <typename command>
void raft_storage<command>::append_log_entry1(int current_term,int leader_id,int committed_term,int committed_index,
            int last_added_term,int last_added_index,
             std::map<int,vector<log_entry<command>>> &log,int size1,bool isLeader,int myid) {
    mtx.lock();
    log_entry_map.clear();
    log_entry_map = log;
    std::ofstream file(file_path_logfile, std::ios::out | std::ios::binary);
    file.write((char *)& current_term,sizeof(int));
    file.write((char *)& leader_id,sizeof(int));
    file.write((char *)& committed_term, sizeof(int ));
    file.write((char *)& committed_index,sizeof(int ));
    file.write((char *)& last_added_term, sizeof(int ));
    file.write((char *)& last_added_index,sizeof(int ));
    file.write((char *)& isLeader,sizeof(bool));
    file.write((char *)& size1, sizeof(int));
  
    for(auto i = log_entry_map.begin();i!= log_entry_map.end();i++) {
        vector<log_entry<command>> v = log_entry_map[i->first];
        for (log_entry<command>& lg : v) {
            file.write((char *)&lg.term, sizeof(int));
            file.write((char *)&lg.index, sizeof(int));
            int se = lg.cmd.size();
            file.write((char *)&se,sizeof(int));
            char* buf = new char[se];
            lg.cmd.serialize(buf, se);
           file.write(buf, se);
        }
    }
    file.close();
    mtx.unlock();
}


template <typename command>
void raft_storage<command>::restore_log1(int& current_term,int& leader_id,int& committed_term,int& committed_index,
                                         int& last_added_term,int& last_added_index,
                                          std::map<int,vector<log_entry<command>>>&log,bool& result, bool& isLeader,int myid) {

    mtx.lock();  
   
    std::ifstream file(file_path_logfile, std::ios::in | std::ios::binary);
    if (file.is_open()){
        
        file.read((char *)& current_term,sizeof(int));
        file.read((char *)& leader_id,sizeof(int));
        file.read((char *)& committed_term,sizeof(int ));
        file.read((char *)& committed_index,sizeof(int ));
        file.read((char *)& last_added_term,sizeof(int ));
        file.read((char *)& last_added_index,sizeof(int ));
        file.read((char *)& isLeader,sizeof(bool));
        unsigned int s1;
        file.read((char *)& s1, sizeof(int));
        for(int i=0; i<s1;i++) {
            log_entry<command> lg;
            file.read((char *)&lg.term, sizeof(int));
            file.read((char *)&lg.index, sizeof(int));
            int sz;
            file.read((char *)&sz, sizeof(int));
            char* buf = new char[sz];
            file.read(buf, sz);
            lg.cmd.deserialize(buf, sz);
            log[lg.term].push_back(lg);
        }
        result = false;
    } else {
      result = true;
    }
    file.close();
    mtx.unlock();
}


template <typename command>
void raft_storage<command>::make_snapshot(int size,int last_included_term,int last_included_index,vector<char> data,vector<char*> char_list,vector<int> sizes_list) {
    mtx.lock();
    std::ofstream file(file_path_snapshotfile, std::ios::out | std::ios::binary);
    file.write((char *)& last_included_term,sizeof(int));
    file.write((char *)& last_included_index,sizeof(int));
    file.write((char *)& size,sizeof(int));
    int size1 = data.size();
    file.write((char *)& size1, sizeof(int));
    for(int i=0; i<data.size();i++) {
        file.write((char*)&data[i], sizeof(char));
    }
    int su = char_list.size();
    file.write((char*)&su, sizeof(int));
    for(int i=0; i<char_list.size();i++) {
        char *s1 = char_list[i];
        file.write((char*)&sizes_list[i],sizeof(int));
        file.write(s1, sizes_list[i]);
    }
    file.close();
    mtx.unlock();
}

template <typename command>
void raft_storage<command>::restore_snapshot(int& log_size,int& snapshot_included_term,int& snapshot_included_index,vector<char>& data,vector<char*> &list,vector<int> & list1) {
    mtx.lock();
    std::ifstream file(file_path_snapshotfile, std::ios::in | std::ios::binary);
    if (file.is_open()){
        file.read((char *)& snapshot_included_term,sizeof(int));
        file.read((char *)& snapshot_included_index,sizeof(int ));
        file.read((char *)& log_size,sizeof(int));
        int s1;
        file.read((char *)& s1, sizeof(int));
        for(int i=0; i<s1;i++) {
            char sz;
            file.read((char *)&sz, sizeof(char));
            data.push_back(sz);
        }
        int s2;
        file.read((char *)& s2, sizeof(int));
        for(int i=0; i<s2;i++) {
            int se;
            file.read((char *)&se, sizeof(int));
            char *s3 = new char[se];
            file.read(s3, se);
            list.push_back(s3);
            list1.push_back(se);
        }
    }
    file.close();
    mtx.unlock();
}


#endif // raft_storage_h
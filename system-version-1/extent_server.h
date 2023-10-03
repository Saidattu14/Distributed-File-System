
// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "inode_manager.h"
#include "persister.h"

class extent_server {
 protected:
#if 0
  typedef struct extent {
    std::string data;
    struct extent_protocol::attr attr;
  } extent_t;
  std::map <extent_protocol::extentid_t, extent_t> extents;
#endif
  inode_manager *im;
  chfs_persister *_persister;
  

 public:
  bool restore;
  extent_server();
  long long current_txid_t;
  int create(uint32_t type, extent_protocol::extentid_t &id);
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
  
  // Your code here for lab2A: add logging APIs
  void begin_transaction(chfs_command::txid_t,chfs_command::operation_type);
  void write_put_data_transaction(chfs_command::txid_t,string,extent_protocol::extentid_t);
  void write_remove_data_transaction(chfs_command::txid_t,extent_protocol::extentid_t);
  void commit_transaction(chfs_command::txid_t);
  void redo(std::vector<chfs_command>); 
};
#endif 

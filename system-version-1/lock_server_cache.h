#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
#include "extent_protocol.h"
#include <queue>


class lock_client_data {
    public:
    enum lock_status{
            NONE = 0,
            ACQUIRING,
            ACQUIRED,
            RELEASING,
            GRANTING,
    };
    string id;
    lock_status ls = NONE;
    // constructor
    lock_client_data() {}
};


class lock_server_cache {
 private:
  int nacquire;
 public:
  pthread_mutex_t myMutex;
  pthread_cond_t cond;
  std::map<lock_protocol::lockid_t,lock_client_data> lock_holders_db;
  std::map<lock_protocol::lockid_t,queue<string>> waiting_clients_db;
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
  void retry_call(lock_protocol::lockid_t,string id);
  void revoke_call(lock_protocol::lockid_t,string id);
};

#endif
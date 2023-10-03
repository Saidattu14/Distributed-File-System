// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

using namespace std;

class lock_data {
    public:
    enum lock_status{
            FREE = 0,
            LOCKED,
    };
    lock_status ls = FREE;
    // constructor
    lock_data() {}
};

class lock_server {

 protected:
  int nacquire;
  pthread_mutex_t myMutex;
  pthread_cond_t cond;
  std::map<lock_protocol::lockid_t,lock_data> locks_db;
 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif
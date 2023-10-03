// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include "extent_client.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 6.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
  class locks_data {
public:
    enum state {
        NONE,
        IDLE,
        WORKING,
    };
    state ls = NONE;
    string client_id;
    locks_data() {
      ls = NONE;
    };
};

  class thread_data {
  public:
      queue<pthread_cond_t*> threads_queue;
      thread_data() {
        threads_queue = std::queue<pthread_cond_t*>();
      };
  };
  class lock_revoke_info {
    public:
      bool revoke_request_recieved;
      int count;
      bool blocked;
      lock_revoke_info() {
        revoke_request_recieved = false;
        count = 0;
        blocked = false;
      };
  };
  pthread_mutex_t myMutex;
  std::map<lock_protocol::lockid_t,locks_data*> locks_db;
  std::map<lock_protocol::lockid_t,thread_data*> waiting_db;
  std::map<lock_protocol::lockid_t,lock_client_cache::lock_revoke_info*> revoke_db;
  std::map<std::string,rpcc*> socket_data;
 public:
  static int last_port;
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache() {};
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t,
                                       int &);
  lock_protocol::status rpc_acquire_lock(lock_protocol::lockid_t);
  void rpc_release_lock(lock_protocol::lockid_t);
  void checking_lock_state(lock_protocol::lockid_t);
  void make_client_cache_dirty(lock_protocol::lockid_t,string,rpcc*);
  lock_protocol::status acquire_main(lock_protocol::lockid_t,std::string);
  void update_release(lock_protocol::lockid_t);
  rpcc* get_release_client_connection(string);
};


#endif
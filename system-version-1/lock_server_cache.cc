// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"

lock_server_cache::lock_server_cache()
{
  nacquire = 0;
  pthread_mutex_init(&myMutex,NULL);
  pthread_cond_init(&cond, NULL);
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &)
{
  cout<<"server-acquire"<<lid<<"ksk"<<id<<"\n";
  pthread_mutex_lock(&myMutex);
  lock_protocol::status ret = lock_protocol::OK;
  if(lock_holders_db[lid].ls == lock_client_data::NONE) {
    lock_client_data lc;
    lc.id = id;
    lock_holders_db[lid] = lc;
    lock_holders_db[lid].ls = lock_client_data::ACQUIRED;
  } else {
    if(lock_holders_db[lid].ls == lock_client_data::ACQUIRED) {
       waiting_clients_db[lid].push(id);
       lock_holders_db[lid].ls = lock_client_data::GRANTING;
       pthread_mutex_unlock(&myMutex);
       revoke_call(lid,lock_holders_db[lid].id);
    } else if(lock_holders_db[lid].ls == lock_client_data::GRANTING || lock_holders_db[lid].ls == lock_client_data::ACQUIRING) {
       waiting_clients_db[lid].push(id);
       pthread_mutex_unlock(&myMutex);
    }
    return lock_protocol::RETRY;
  }
  pthread_mutex_unlock(&myMutex);
  return ret;
}

int
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &r) {
  cout<<"release"<<lid<<"ddd"<<id<<"\n";
  pthread_mutex_lock(&myMutex);
  lock_protocol::status ret = lock_protocol::OK;
  if((id.compare(lock_holders_db[lid].id)) == 0) {
    if(waiting_clients_db[lid].size() != 0) { 
      lock_client_data lc;
      lc.id = waiting_clients_db[lid].front();
      lock_holders_db[lid] = lc;
      lock_holders_db[lid].ls = lock_client_data::ACQUIRING;
      pthread_mutex_unlock(&myMutex);
      retry_call(lid,waiting_clients_db[lid].front());
      pthread_mutex_lock(&myMutex);
      waiting_clients_db[lid].pop();
    } else {
      waiting_clients_db.erase(lid);
      lock_holders_db.erase(lid);
    }
  } else {
    pthread_mutex_unlock(&myMutex);
    return lock_protocol::RPCERR;
  }
  pthread_mutex_unlock(&myMutex);
  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

void lock_server_cache::retry_call(lock_protocol::lockid_t lid,string id) {
   rlock_protocol::status ret = rlock_protocol::OK;
  int r;
  handle h(id);
  rpcc *cl = h.safebind();
  //do{
    ret = cl->call(rlock_protocol::retry, lid,r);
    if(ret == rlock_protocol::OK) {
       lock_holders_db[lid].ls = lock_client_data::ACQUIRED;
    } else {
      //retry_call(lid,id);
      cout<<"ssssssssssssssssssssssssssssssss"<<"\n";
    }
   
   // printf("retry from server %d\n", ret);
// } while (ret != rlock_protocol::OK);
 // return ret;
  //return lock_protocol::OK;
}

void lock_server_cache::revoke_call(lock_protocol::lockid_t lid,string id) {
 rlock_protocol::status ret;
  int r;
  handle h(id);
  rpcc *cl = h.safebind();
// do{
    cout<<"revokeeee"<<lid<<"ddddddd"<<id<<"\n";
    ret = cl->call(rlock_protocol::revoke, lid, r);
    if(ret == rlock_protocol::OK) {

    } else {
      cout<<"iuuuuuuuuuuuuuuuuuuuuu"<<lid<<"\n";
    }
    printf("revoke from server ret: %d\n", ret);
    int r1 = 0;
 // } while (ret != rlock_protocol::OK);
  //return ret;
}
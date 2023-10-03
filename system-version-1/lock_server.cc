// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>

lock_server::lock_server():
  nacquire (0) {
  pthread_mutex_init(&myMutex,NULL);
  pthread_cond_init(&cond, NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r) {
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab2B part2 code goes here
  r = 0;
  pthread_mutex_lock(&myMutex);
  for (auto& i  : locks_db) {
    if(i.first == lid) {
        while(locks_db[lid].ls == lock_data::LOCKED) {
          pthread_cond_wait(&cond,&myMutex);
        }
        break;
      }
  }
  lock_data ld ;
  ld.ls = lock_data::LOCKED;
  locks_db[lid] = ld;
  pthread_mutex_unlock(&myMutex);
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab2B part2 code goes here
   r = 0;
   pthread_mutex_lock(&myMutex);
   for (auto& i  : locks_db) {
    if(i.first == lid) {
      lock_data ld = i.second;
      if(ld.ls == lock_data::LOCKED) {
        ld.ls = lock_data::FREE;
        locks_db[lid] = ld;
        pthread_cond_broadcast(&cond);
      } 
      break;
    }
  }
  pthread_mutex_unlock(&myMutex);
  return ret;
}
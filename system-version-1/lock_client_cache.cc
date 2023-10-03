// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"
#include <bits/stdc++.h>


int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
  pthread_mutex_init(&myMutex,NULL);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid) {
 // cout<<lid<<"acquire-main-lockid"<<lid<<"\n";
  pthread_mutex_lock(&myMutex);
  lock_revoke_info *lc = revoke_db[lid];
  if(lc == NULL) {
    lc = new lock_revoke_info();
    revoke_db[lid] = lc;
    pthread_mutex_unlock(&myMutex);
    checking_lock_state(lid);
    pthread_mutex_lock(&myMutex);
  } else {
    if((!lc->revoke_request_recieved || lc->count <= 3) && !lc->blocked) {
      pthread_mutex_unlock(&myMutex);
      checking_lock_state(lid);
      pthread_mutex_lock(&myMutex);
    } else {
    // cout<<"budddddddddddddddddddddddddddd"<<"\n";
      thread_data *t = waiting_db[lid];
      pthread_cond_t cond2;
      pthread_cond_init(&cond2, NULL);
      t->threads_queue.push(&cond2);
      while(lc->revoke_request_recieved) {
        pthread_cond_wait(&cond2,&myMutex);
      }
      pthread_mutex_unlock(&myMutex);
      acquire(lid);
      pthread_mutex_lock(&myMutex);
    }
  }
  pthread_mutex_unlock(&myMutex);
 // cout<<lid<<"done"<<"\n";
  return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  //cout<<lid<<"releaseeeeeeeeeeeeeeeeeeeeeeeee-lockid"<<"\n";
  pthread_mutex_lock(&myMutex);
  lock_revoke_info *lc = revoke_db[lid];
  thread_data *t = waiting_db[lid];
  t->threads_queue.pop();
  if(lc != NULL) {
    if(lc->revoke_request_recieved) {
      if(t->threads_queue.size() == 0) {
        lc->blocked = true;
        locks_data *ld = locks_db[lid];
        ld->ls = locks_data::IDLE;
        pthread_mutex_unlock(&myMutex);
        rpc_release_lock(lid);
        pthread_mutex_lock(&myMutex);
        rpcc *c = get_release_client_connection(ld->client_id);
        pthread_mutex_unlock(&myMutex);
        make_client_cache_dirty(lid,ld->client_id,c);
        pthread_mutex_lock(&myMutex);
        update_release(lid);
      } else {
        pthread_cond_signal(t->threads_queue.front());
        lc->count++;
      }
    } else {
      locks_data *ld = locks_db[lid];
      ld->ls = locks_data::IDLE;
      if(t->threads_queue.size() != 0) {
        pthread_cond_signal(t->threads_queue.front());
      }
    }
  } else {
      locks_data *ld = locks_db[lid];
      ld->ls = locks_data::IDLE;
      if(t->threads_queue.size() != 0) {
        pthread_cond_signal(t->threads_queue.front());
      }
  }
  pthread_mutex_unlock(&myMutex);
  return lock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  //cout<<lid<<"revoke-yyyyyyyyyyyyyyyylockid"<<"\n";
  pthread_mutex_lock(&myMutex);
  bool revoke = false;
  locks_data *ld = locks_db[lid];
  if(ld != NULL) {
    if(ld->ls == locks_data::IDLE) {
    lock_revoke_info *lc = revoke_db[lid];
    if(lc == NULL) {
      lc = new lock_revoke_info();
      revoke_db[lid] = lc;
    }
    thread_data *t = waiting_db[lid];
    if(t->threads_queue.size() == 0) {
      lc->blocked = true;
      lc->revoke_request_recieved = true;
      pthread_mutex_unlock(&myMutex);
      rpc_release_lock(lid);
      pthread_mutex_lock(&myMutex);
      rpcc *c = get_release_client_connection(ld->client_id);
      pthread_mutex_unlock(&myMutex);
      make_client_cache_dirty(lid,ld->client_id,c);
      pthread_mutex_lock(&myMutex);
      update_release(lid);
    } else {
      lc->count = 0;
      lc->blocked = false;
      lc->revoke_request_recieved = true;
    }
  } else if(ld->ls == locks_data::WORKING) {
      lock_revoke_info *lc = revoke_db[lid];
      if(lc == NULL) {
        lc = new lock_revoke_info();
        revoke_db[lid] = lc;
      }
      lc->count = 0;
      lc->blocked = false;
      lc->revoke_request_recieved = true;
    }
  }
  pthread_mutex_unlock(&myMutex);
 // cout<<"qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"<<"\n";
  return rlock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &) {
  //cout<<lid<<"retryyyyyyyyyyyyyyyyyyyyy"<<"\n";
  pthread_mutex_lock(&myMutex);
  locks_data *ld = locks_db[lid];
  if(ld->ls == locks_data::NONE) {
    ld->ls = locks_data::WORKING;
    thread_data *t1 = waiting_db[lid];
    if(t1->threads_queue.size() != 0) {
      pthread_cond_signal(t1->threads_queue.front());
    }
  }
  pthread_mutex_unlock(&myMutex);
  return rlock_protocol::OK;
}

lock_protocol::status lock_client_cache::rpc_acquire_lock(lock_protocol::lockid_t lid) {
  int r;
  lock_protocol::status ret = lock_protocol::OK;
  do{
    ret = cl->call(lock_protocol::acquire, lid, id, r);
  } while (ret != lock_protocol::OK && ret != lock_protocol::RETRY);
  return ret;
}

rpcc* lock_client_cache::get_release_client_connection(string client_id) {
  rpcc *c = socket_data[client_id];
  if(c == NULL) {
    sockaddr_in ds;
    make_sockaddr(client_id.c_str(), &ds);
    c = new rpcc(ds);
    if (c->bind() < 0) {
      printf("lock_client: call bind\n");
    }
    socket_data[client_id] = c;
  }
  return c;
}


void lock_client_cache::rpc_release_lock(lock_protocol::lockid_t lid) {
  //cout<<"releaseeeeeeeeeeeeeeeeee"<<lid<<"idd"<<id<<"\n";

  int r;
  lock_protocol::status ret = lock_protocol::OK;
  do{
    ret = cl->call(lock_protocol::release, lid, id, r);
  } while (ret != lock_protocol::OK);
 // cout<<"release done"<<"\n";
}


void lock_client_cache::checking_lock_state(lock_protocol::lockid_t lid) {
 // cout<<"mmmmmmmmmmmmm"<<lid<<"\n";
  pthread_mutex_lock(&myMutex);
  thread_data *t = waiting_db[lid];
  if(t == NULL) {
    t = new thread_data();
    waiting_db[lid] = t;
  }
  locks_data *ld = locks_db[lid];
  if(ld == NULL) {
    ld = new locks_data();
    locks_db[lid] = ld;
  }
  pthread_cond_t cond2;
  pthread_cond_init(&cond2, NULL);
  t->threads_queue.push(&cond2);
  if(t->threads_queue.size() == 1) {
      if(ld->ls == locks_data::NONE) {
        pthread_mutex_unlock(&myMutex);
        lock_protocol::status rt = rpc_acquire_lock(lid);
        pthread_mutex_lock(&myMutex);
        if(rt == lock_protocol::OK) {
          locks_data *ld = locks_db[lid];
          ld->ls = locks_data::WORKING;
          //cout<<"lock Goitiiii"<<"\n";
        } else {
          while(ld->ls == locks_data::NONE) {
            pthread_cond_wait(t->threads_queue.front(),&myMutex);
          }
          //cout<<"lockGoityyy"<<"\n";
        }
      } else if(ld->ls == locks_data::IDLE) {
        ld->ls = locks_data::WORKING;
      }
    //  cout<<"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"<<"\n";
  } else {
   // cout<<"ggggggggggggggggggggggggggggggggggggggggggggggggggg"<<"\n";
    while(ld->ls == locks_data::WORKING || ld->ls == locks_data::NONE) {
      pthread_cond_wait(&cond2,&myMutex);
    }
    ld->ls = locks_data::WORKING;
   //cout<<"tttttttttttttttttttiiiiiidjjdjdjdj"<<"\n";
  }
  pthread_mutex_unlock(&myMutex);
}

lock_protocol::status
lock_client_cache::acquire_main(lock_protocol::lockid_t lid,std::string id) {
 // cout<<"acquire"<<lid<<"lockid"<<"yyyyyyyyyy"<<id<<"\n";
  acquire(lid);
  pthread_mutex_lock(&myMutex);
  locks_data *ld = locks_db[lid];
  if(ld == NULL) {
    ld = new locks_data();
    locks_db[lid] = ld;
  }
  ld->client_id = id;
  pthread_mutex_unlock(&myMutex);
  return lock_protocol::OK;
}


void lock_client_cache::make_client_cache_dirty(lock_protocol::lockid_t lid,string id1,rpcc *c) {
  rextent_protocol::status ret;
  int r;
  do{
    ret = c->call(rextent_protocol::dirty_data, lid, r);
  } while (ret != rextent_protocol::OK);
}


void lock_client_cache::update_release(lock_protocol::lockid_t lid) {
   locks_data *ld = locks_db[lid];
    ld->ls = locks_data::NONE;
    lock_revoke_info *lc = revoke_db[lid];
    if(lc != NULL) {
      lc->count = 0;
      lc->revoke_request_recieved = false;
      lc->blocked = false;
    }
    thread_data *t = waiting_db[lid];
    if(t != NULL) {
      if(t->threads_queue.size() != 0) {
        pthread_cond_broadcast(t->threads_queue.front());
        t->threads_queue.pop();
      }
    }
}



// the extent server implementation

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bits/stdc++.h>
#include "extent_server.h"
#include "persister.h"

extent_server::extent_server() {
  im = new inode_manager();
  _persister = new chfs_persister("log"); // DO NOT change the dir name here
  // Your code here for Lab2A: recover data on startup
  _persister->restore_checkpoint();
  _persister->restore_logdata();
  restore = false;
  vector<chfs_command> check_point_commands = _persister->get_check_point_entries();
  //cout<<"check_pont"<<check_point_commands.size()<<"\n";
  vector<chfs_command> vector_chfs_commands = _persister->get_log_entries();
 // cout<<"vector_pont"<<vector_chfs_commands.size()<<"\n";
  redo(check_point_commands);
  redo(vector_chfs_commands);
  _persister->clear_log_entries();
  _persister->clear_check_point_entries();
  restore = true;
}

void extent_server::redo(std::vector<chfs_command> v) {
  // alloc a new inode and return inum
  for (chfs_command& ch  : v) {
     // cout<<"id"<<ch.id<<"hello"<<ch.type<<"tt"<<ch.op<<"ss"<<ch.eid<<"\n";
      if(ch.op == chfs_command::CREATE_FILE) {
          extent_protocol::extentid_t id;
          create(extent_protocol::T_FILE,id);
      } else if(ch.op == chfs_command::MAKE_DIR) {
          extent_protocol::extentid_t id;
          create(extent_protocol::T_DIR,id);
      } else if (ch.op == chfs_command::WRITE){
        
      } else if (ch.op == chfs_command::UNLINK) {
            
      } else if (ch.op == chfs_command::SYMLINK) {
          extent_protocol::extentid_t id;
          create(extent_protocol::T_SYMLINK,id);
      } else if(ch.op == chfs_command::PUT) {
          int r;
          put(ch.eid,ch.data,r);
      } else if (ch.op == chfs_command::REMOVE){
          int r;
          remove(ch.eid,r);
      } else {
        //cout<<ch.op<<"sss"<<ch.type<<"ss"<<ch.eid<<"ss"<<ch.data.length()<<"\n";
      }
  }
}


int extent_server::create(uint32_t type, extent_protocol::extentid_t &id) {
  // alloc a new inode and return inum
  int trans = current_txid_t++;
  chfs_command ch;
  ch.op = static_cast<chfs_command::operation_type>(type);
  begin_transaction(trans,ch.op);
  printf("extent_server: create inode\n");
  id = im->alloc_inode(type);
  commit_transaction(trans);
  return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  int trans = current_txid_t++;
  begin_transaction(trans,chfs_command::PUT);
  id &= 0x7fffffff;
  write_put_data_transaction(trans,buf,id);
  const char * cbuf = buf.c_str();
  int size = buf.size();
  im->write_file(id, cbuf, size);
  commit_transaction(trans);
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  printf("extent_server: get %lld\n", id);

  id &= 0x7fffffff;

  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    free(cbuf);
  }
  
  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  printf("extent_server: getattr %lld\n", id);

  id &= 0x7fffffff;
  
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->get_attr(id, attr);
  a = attr;
  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  int trans = current_txid_t++;
  begin_transaction(trans,chfs_command::REMOVE);  
  printf("extent_server: write %lld\n", id);
  write_remove_data_transaction(trans,id);
  id &= 0x7fffffff;
  im->remove_file(id);
  commit_transaction(trans);
  return extent_protocol::OK;
}

void extent_server::begin_transaction(chfs_command::txid_t id,chfs_command::operation_type op1) {
  chfs_command chfs;
  chfs.id = id;
  chfs.type = chfs_command::CMD_BEGIN;
  chfs.op = op1;
  _persister->append_log(chfs);
}

void extent_server::write_put_data_transaction(chfs_command::txid_t id,string data,extent_protocol::extentid_t eid) {
  if(restore) {
  chfs_command chfs;
  chfs.id = id;
  chfs.type = chfs_command::CMD_OPERATION;
  chfs.op = chfs_command::PUT;
  chfs.data = data;
  chfs.eid = eid;
  _persister->append_log(chfs);
  }
}

void extent_server::write_remove_data_transaction(chfs_command::txid_t id,extent_protocol::extentid_t eid) {
  if(restore) {
  chfs_command chfs;
  chfs.id = id;
  chfs.type = chfs_command::CMD_OPERATION;
  chfs.op = chfs_command::REMOVE;
  chfs.eid = eid;
  _persister->append_log(chfs);
  }
}

void extent_server::commit_transaction(chfs_command::txid_t id) {
  if(restore) {
  chfs_command chfs;
  chfs.id = id;
  chfs.type = chfs_command::CMD_COMMIT;
  _persister->append_log(chfs);
  }
}
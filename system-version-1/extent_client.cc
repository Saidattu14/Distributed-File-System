// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

int extent_client::last_port = 12345;
extent_client::extent_client(std::string dst,std::string &extent_sock)
{
 // es = new extent_server();
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
  srand(time(NULL)^last_port);
  client_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << client_port;
  id = host.str();
  last_port = client_port;
  rpcs *rlsrpc = new rpcs(client_port);
  extent_sock = id;
  rlsrpc->reg(rextent_protocol::dirty_data, this, &extent_client::dirty_data_handler);
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
{
  extent_protocol::status ret;
  // Your lab2B part1 code goes here
  ret = cl->call(extent_protocol::create, type, id);
  VERIFY (ret == extent_protocol::OK);
  return ret;
}



extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret;
  cache_class::cache_data *c1 = c.cache_db[eid];
  if(c1 != NULL) {
    if(c1->st == cache_class::cache_data::CLEAN_CACHE) {
      buf = c1->data;
      //cout<<"pppppppppppppppppp"<<buf<<"hee"<<eid<<"\n";
      return extent_protocol::OK;
    }
  } else {
    c1 = new cache_class::cache_data();
    c.cache_db[eid] = c1;
  }
  ret = rpc_get(eid,buf);
  if(ret == extent_protocol::OK) {
    c1->st = cache_class::cache_data::CLEAN_CACHE;
    c1->data = buf;
    //cout<<"mmmmmmmmmmmmmmmmmm"<<eid<<"sssssssss"<<c1->data<<"\n";
  }
  return ret;
}

extent_protocol::status extent_client::rpc_get(extent_protocol::extentid_t eid, std::string &buf) {
  extent_protocol::status ret;
  ret = cl->call(extent_protocol::get,eid,buf);
  return ret;
}

extent_protocol::status extent_client::rpc_put(extent_protocol::extentid_t eid, std::string buf) {
  extent_protocol::status ret;
  int r = 0;
  ret = cl->call(extent_protocol::put, eid,buf,r);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret;
  // Your lab2B part1 code goes here
  ret = cl->call(extent_protocol::getattr,eid,attr);
  VERIFY (ret == extent_protocol::OK);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret;
  ret = rpc_put(eid,buf);
  if(ret == extent_protocol::OK) {
    cache_class::cache_data *c1 = c.cache_db[eid];
    if(c1 == NULL) {
      c1 = new cache_class::cache_data();
      c.cache_db[eid] = c1;
    }
    c1->st = cache_class::cache_data::CLEAN_CACHE;
    c1->data = buf;
    //cout<<c1->data<<"dddd"<<eid<<"\n";
  }
  return ret;
}


extent_protocol::status
extent_client::put1(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret;
  // Your lab2B part1 code goes here
  int r = 0;
  ret = cl->call(extent_protocol::put, eid,buf,r);
  VERIFY (ret == extent_protocol::OK);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret;
  // Your lab2B part1 code goes here
  int r = 0;
  ret = cl->call(extent_protocol::remove, eid,r); 
  VERIFY (ret == extent_protocol::OK);
  return ret;
}

rextent_protocol::status
extent_client::dirty_data_handler(extent_protocol::extentid_t eid,int &) {
  cache_class::cache_data *c1 = c.cache_db[eid];
  if(c1 != NULL) {
    c1->st = cache_class::cache_data::DIRTY_CACHE;
    //cout<<"heeeeeeeeeeeeeeeeeeeeeeee"<<c.cache_db[eid]->data<<"\n";
  }
  return rextent_protocol::OK;
}

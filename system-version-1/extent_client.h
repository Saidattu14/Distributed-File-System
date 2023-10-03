// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "extent_server.h"

class cache_class {
  public:
  class cache_data {
    public:
    enum state {
        NONE,
        CLEAN_CACHE,
        DIRTY_CACHE
    };
    state st = NONE;
    string data;
    cache_data() {};
  };
  std::map<extent_protocol::extentid_t,cache_class::cache_data*> cache_db;
  cache_class() {};
};

class extent_client {
 private:
  rpcc *cl,*cl1;
  int client_port;
  std::string hostname;
  std::string id;
  cache_class c;
  //extent_server *es;
 public:
  static int last_port;
  extent_client(std::string dst,std::string &extent_sock);
  extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
  extent_protocol::status get(extent_protocol::extentid_t eid, std::string &buf);
  extent_protocol::status rpc_get(extent_protocol::extentid_t eid, std::string &buf);
  extent_protocol::status rpc_put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status put1(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  rextent_protocol::status dirty_data_handler(extent_protocol::extentid_t eid,int &);
};

#endif 


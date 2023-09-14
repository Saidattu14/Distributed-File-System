#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <mutex>
#include "extent_protocol.h"
#include "extent_server_dist.h"
#include "mr_protocol.h"
#include "rpc.h"
#include "extent_client.h"
#include "chfs_client.h"

#define NUM_NODES 3

using namespace std;

chfs_client *chfs_c;
extent_client *ec;
extent_server_dist *es_rg;
struct Task {
	int taskType;     // should be either Mapper or Reducer
	bool isAssigned;  // has been assigned to a worker
	bool isCompleted; // has been finised by a worker
	int index;        // index to the file
	std::chrono::system_clock::time_point task_assign_timer;
	long long unsigned int inum = 0;
	long long unsigned int file_length = 0;
};

class Coordinator {
public:
	Coordinator(const vector<string> &files, int nReduce);
	mr_protocol::status askTask(int, mr_protocol::AskTaskResponse &reply);
	mr_protocol::status submitTask(int taskType, int index,vector<KeyVal> v, bool &success);
	bool isFinishedMap();
	bool isFinishedReduce();
	bool Done();
	int get_partion_index(string s);

private:
	vector<string> files;
	vector<Task> mapTasks;
	vector<Task> reduceTasks;

	mutex mtx;

	long completedMapCount;
	long completedReduceCount;
	bool isFinished;
	
	string getFile(int index);
	int last_finished_map_task_index = -1;
	int last_finished_reduc_task_index = -1;
	int mapTasksLength;
	int reduceTasksLength;
};



// Your code here -- RPC handlers for the worker to call.

mr_protocol::status Coordinator::askTask(int, mr_protocol::AskTaskResponse &reply) {
	// Lab4 : Your code goes here.
	  mtx.lock();
	  if(this->completedMapCount != mapTasks.size()) {
		reply.taskType = mr_tasktype::NONE;
		for(int i = last_finished_map_task_index+1; i< mapTasks.size();i++) {
            Task t = mapTasks[i];
			if(!t.isCompleted) {
				if(t.isAssigned) {
                    if(chrono::system_clock::now() - t.task_assign_timer > chrono::milliseconds(50)) {
						reply.index = t.index;
						reply.taskType =  mr_tasktype::MAP;
						reply.file_length = t.file_length;
						reply.inum = t.inum;
						mapTasks[i].task_assign_timer = chrono::system_clock::now();
						break;
					}
				} else {
                   reply.index = t.index;
					reply.taskType = mr_tasktype::MAP;
					reply.file_length = t.file_length;
					reply.inum = t.inum;
					mapTasks[i].task_assign_timer = chrono::system_clock::now();
					mapTasks[i].isAssigned = true;
					break;
				}
			}
		}
		mtx.unlock();
		return mr_protocol::OK;
	} else {
		reply.taskType = mr_tasktype::NONE;
		if(this->completedReduceCount != reduceTasks.size()) {
			for(int i = last_finished_reduc_task_index+1; i< reduceTasks.size();i++) {
				Task t = reduceTasks[i];
				if(!t.isCompleted) {
					if(t.isAssigned) {
						if(chrono::system_clock::now() - t.task_assign_timer > chrono::milliseconds(50)) {
							reply.index = t.index;
							reply.taskType = mr_tasktype::REDUCE;
							reply.inum = t.inum;
							reply.file_length = t.file_length;
							reduceTasks[i].task_assign_timer = chrono::system_clock::now();
							break;
						}
					} else {
                        reply.index = t.index;
						reply.inum = t.inum;
						reply.file_length = t.file_length;
						reply.taskType = mr_tasktype::REDUCE;
						reduceTasks[i].isAssigned = true;
						reduceTasks[i].task_assign_timer = chrono::system_clock::now();
						break;
					}
				}
			}
			mtx.unlock();
			return mr_protocol::OK;
		} else {
		  mtx.unlock();
          return mr_protocol::OK;
		}
	} 
	return mr_protocol::OK;
}

mr_protocol::status Coordinator::submitTask(int taskType, int index,vector<KeyVal> v, bool &success) {
	// Lab4 : Your code goes here.
	mtx.lock();
	if(taskType == 1) {
		Task t = mapTasks[index];
		if(!t.isCompleted) {
			mapTasks[index].isCompleted = true;
			vector<string> k;
			for(int i =0; i<REDUCER_COUNT;i++) {
				k.push_back("");
			}
			for (auto const &keyVal : v){
              int partion_key = get_partion_index(keyVal.key);
			  string s2 = keyVal.key + ' ' + keyVal.val + '\n';
			  k[partion_key] = k[partion_key] + s2;
    		}
			for(int i=0; i<REDUCER_COUNT;i++) {
				Task t1 = reduceTasks[i];
				string buf = k[i];
				size_t bytes_written;
				//this->mtx.unlock();
				chfs_c->write(t1.inum,buf.length(),t1.file_length,buf.c_str(),bytes_written);
				//this->mtx.lock();
				reduceTasks[i].file_length = reduceTasks[i].file_length + buf.length();
				//chfs_c->print(to_string(buf.length()) + "mappramp " + to_string(t1.inum));
			}
			if(index == last_finished_map_task_index + 1) {
				for(int i=last_finished_map_task_index;i<mapTasks.size();i++) {
					Task t1 = mapTasks[i];
					if(t1.isCompleted) {
						last_finished_map_task_index = i;
					} else {
						break;
					}
				}
			}
			//chfs_c->print(to_string(this->completedMapCount) + " completed");
			this->completedMapCount++;
		}
	} else if(taskType == 2) {
		Task t = reduceTasks[index];
		if(!t.isCompleted) {
           reduceTasks[index].isCompleted = true;
		   this->completedReduceCount++;
		}
	}
	mtx.unlock();
	return mr_protocol::OK;
}

int Coordinator::get_partion_index(string s) {
	string s1;
	s1[0] = toupper(s[0]);
	int val = int(s1[0]) - 65;
	return val%REDUCER_COUNT;
}

string Coordinator::getFile(int index) {
	this->mtx.lock();
	string file = this->files[index];
	this->mtx.unlock();
	return file;
}

bool Coordinator::isFinishedMap() {
	bool isFinished = false;
	this->mtx.lock();
	if (this->completedMapCount >= long(this->mapTasks.size())) {
		isFinished = true;
	}
	this->mtx.unlock();
	return isFinished;
}

bool Coordinator::isFinishedReduce() {
	bool isFinished = false;
	this->mtx.lock();
	if (this->completedReduceCount >= long(this->reduceTasks.size())) {
		isFinished = true;
	}
	this->mtx.unlock();
	return isFinished;
}

//
// mr_coordinator calls Done() periodically to find out
// if the entire job has finished.
//
bool Coordinator::Done() {
	bool r = false;
	this->mtx.lock();
	r = this->isFinished;
	this->mtx.unlock();
	return r;
}

//
// create a Coordinator.
// nReduce is the number of reduce tasks to use.
//
Coordinator::Coordinator(const vector<string> &files, int nReduce)
{
	
	this->files = files;
	this->isFinished = false;
	this->completedMapCount = 0;
	this->completedReduceCount = 0;
	this->mapTasksLength = mapTasks.size();
	this->reduceTasksLength = reduceTasks.size();
	int filesize = files.size();
	for (int i = 0; i < filesize; i++) {
		this->mapTasks.push_back(Task{mr_tasktype::MAP, false, false, i});
		int parent = 1;
		mode_t mode;
		long long unsigned int inum;
		chfs_c->create(parent,getFile(i).c_str(),mode,inum);
		string filepath = "./" + getFile(i);
		ifstream file(filepath);
		ostringstream s;
		s << file.rdbuf();
		file.close();
		string content = s.str();
		const char *data;
        size_t bytes_written;
		chfs_c->write(inum,content.length(),0,content.c_str(),bytes_written);
		this->mapTasks[i].inum = inum;
		this->mapTasks[i].file_length = content.length();
	}
	for (int i = 0; i < nReduce; i++) {
		this->reduceTasks.push_back(Task{mr_tasktype::REDUCE, false, false, i});
		int parent = 1;
		mode_t mode;
		long long unsigned int inum;
		string fname = "reducer" + to_string(i);
		chfs_c->create(parent,fname.c_str(),mode,inum);
		this->reduceTasks[i].inum = inum;
		this->reduceTasks[i].file_length = 0;
	}
}

int main(int argc, char *argv[])
{
		int count = 0;

	if(argc < 3){
		fprintf(stderr, "Usage: %s <port-listen> <inputfiles>...\n", argv[0]);
		exit(1);
	}
	char *port_listen = argv[1];
	
	setvbuf(stdout, NULL, _IONBF, 0);

	char *count_env = getenv("RPC_COUNT");
	if(count_env != NULL){
		count = atoi(count_env);
	}

	vector<string> files;
	char **p = &argv[3];
	while (*p) {
		files.push_back(string(*p));
		++p;
	}

    std::string extent_port = argv[2];
	
   
	rpcs server1(stoi(extent_port), count);
    es_rg = new extent_server_dist(NUM_NODES);
    server1.reg(extent_protocol::get, es_rg, &extent_server_dist::get);
    server1.reg(extent_protocol::getattr, es_rg, &extent_server_dist::getattr);
    server1.reg(extent_protocol::put, es_rg, &extent_server_dist::put);
    server1.reg(extent_protocol::remove, es_rg, &extent_server_dist::remove);
    server1.reg(extent_protocol::create, es_rg, &extent_server_dist::create);

	chfs_c = new chfs_client(extent_port);


	rpcs server(atoi(port_listen), count);

	
	Coordinator c(files, REDUCER_COUNT);
	
	//
	// Lab4: Your code here.
	// Hints: Register "askTask" and "submitTask" as RPC handlers here
	// 
	server.reg(mr_protocol::asktask, &c, &Coordinator::askTask);
	server.reg(mr_protocol::submittask, &c, &Coordinator::submitTask);
    
	while(!c.Done()) {
		sleep(20);
	}
	return 0;
}
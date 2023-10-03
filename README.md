

## Project Version2

* This is a simple scalable distributed file system bulit on Raft.
* MapReduce framework for words count was build on top of the file system.

## Raft Algorithm Supports

* Leader Election
* Log Replication
* Log Persistency.
* Log Snapshotting.


## Commands to run the project
* `make clean && make`, At the root `system-version-2` for build
* `run_Raft.py`, At the root `system-version-2` for raft algorithm test
* `run_partA.py`, At the root `system-version-2` for sequential mapreduce test
* `run_partB.py`, At the root `system-version-2` for distributed mapreduce test


## Project Version1

* This is a simple scalable distributed file system supports cache consistency for file locking and file data.
* write-back caching is implemented.


## Commands to run the project
* `make clean && make`, At the root `system-version-1` for build
* `sudo ./start`, At the root `system-version-2`
* `sudo ./test-lab2-part2-a ./chfs1 ./chfs2`, At the root `system-version-1` for file system test
* `sudo ./test-lab2-part2-b ./chfs1 ./chfs2`, At the root `system-version-1` for file system test
* `sudo ./stop.sh`, At the root `system-version-1` for processess clean up.
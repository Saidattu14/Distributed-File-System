#include "inode_manager.h"
#include <ctime>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/stdc++.h>
using namespace std;

// disk layer -----------------------------------------

disk::disk() {
  bzero(blocks, sizeof(blocks));
}

void disk::read_block(blockid_t id, char *buf) {
  if (id < 0 || id >= BLOCK_NUM || !buf)
      return;
  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void disk::write_block(blockid_t id, const char *buf) {
  if (id < 0 || id >= BLOCK_NUM || !buf)
      return;
  memcpy(blocks[id], buf, BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t block_manager::alloc_block() {
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
   for (blockid_t i = (IBLOCK(INODE_NUM+1, BLOCK_NUM) + 1); i < BLOCK_NUM; i++){
      if (using_blocks[i] == 0){
          using_blocks[i] = 1;
          //cout<<"BLOCk"<<i<<"\n";
          return i;
      }
  }
  return 0;
}

void block_manager::free_block(uint32_t id)
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
     if (id < 0 || id >=BLOCK_NUM) {
        return;
     } else {
        using_blocks[id] = 0;
     }
     return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager() {
  d = new disk();
  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;
}

void block_manager::read_block(uint32_t id, char *buf) {
  d->read_block(id, buf);
}

void block_manager::write_block(uint32_t id, const char *buf) {
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager() {
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
    for(uint32_t i=1; i<=INODE_NUM; i++) {
      inode_t * inode = get_inode(i);
      if(!inode) {
          struct inode i1;
          i1.type = type;
          i1.size = 0;
          i1.atime = (unsigned int)time(NULL);
          i1.ctime = (unsigned int)time(NULL);
          i1.mtime = (unsigned int)time(NULL);
          put_inode(i,&i1);
          return i;
      }
    }
    return 1;
}

void inode_manager::free_inode(uint32_t inum)
{

  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
   struct inode *ino = get_inode(inum);
    if (ino){
      ino->type = -1;
      bm->free_block(IBLOCK(inum,bm->sb.nblocks));
      put_inode(inum, ino);
      free(ino);
    }
  return;
}

/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* inode_manager::get_inode(uint32_t inum) {
   /* 
    * your code goes here.
    */
  char buf[BLOCK_SIZE];
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  struct inode *ino;
  ino = (struct inode*)buf + inum%IPB;
  if(ino->type == 0) {
    return NULL;
  }
  struct inode *ino1 = (inode*) malloc(sizeof(inode));
  *ino1 = *ino;
  return ino1;
}

void inode_manager::put_inode(uint32_t inum, struct inode *ino) {
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;
  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void inode_manager::read_file(uint32_t inum, char **buf_out, int *size) {
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
   struct inode *ino = get_inode(inum);
    if(ino->type == 0) {
      return;
    } else {
        int temp = (ino->size/BLOCK_SIZE) + 1;
        char *buf4 = (char*)malloc(temp*BLOCK_SIZE);
        int index = 0;
        for(int i=0;i<NDIRECT && i<temp;i++) {
            char *buf2 = (char*)malloc(BLOCK_SIZE); 
            bm->read_block(ino->blocks[i], buf2);
            memcpy(buf4+index,buf2, BLOCK_SIZE);
            index = index + BLOCK_SIZE;
        }
        if(ino->size > (NDIRECT) *BLOCK_SIZE) {
          blockid_t inDirectBlocks[NINDIRECT];
          bm->read_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
          int s1 = ino->size - ((NDIRECT) * BLOCK_SIZE);
          temp =  (s1/BLOCK_SIZE)+1;
          for(int i=0;i<temp && i<NINDIRECT;i++) {
            char *buf3 = (char*)malloc(BLOCK_SIZE);
            bm->read_block(inDirectBlocks[i], buf3);
            memcpy(buf4+index,buf3, BLOCK_SIZE);
            index = index + BLOCK_SIZE;
          }
        }
        *size = ino->size;
        *buf_out = buf4;
        ino->atime = (uint32_t)(time(NULL));
        put_inode(inum, ino);
        free(ino);
    }
  return;
}

/* alloc/free blocks if needed */
void inode_manager::write_file(uint32_t inum, const char *buf, int size) {
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
    //cout<<inum<<"sks"<<buf<<"sksks\n"<<size;
    struct inode *ino = get_inode(inum);
    if(ino->type == 0 || size > (MAXFILE*BLOCK_SIZE)) {
      return;
    } else {
      alloc_free_blocks(ino,size);
      if(size <=(NDIRECT)*BLOCK_SIZE) {
        pair<int, int> p = filling_directBlocks(ino,buf,size);
      }
      else {
          pair<int, int> p;
          if(ino->size <=(NDIRECT)*BLOCK_SIZE) {
            p = filling_directBlocks(ino,buf,size);
          }
          filling_indirectBlocks(ino,buf,p.second,p.first);
      }
      ino->atime = (uint32_t)(time(NULL));
      ino->ctime = (uint32_t)(time(NULL));
      ino->mtime = (uint32_t)(time(NULL));
      put_inode(inum, ino);
      free(ino);
    }
    return;
}

void inode_manager::get_attr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
    struct inode *ino = get_inode(inum);
    if (ino){
          a.type = ino->type;
          a.mtime = ino->mtime;
          a.ctime = ino->ctime;
          a.atime = ino->atime;
          a.size = ino->size;
    }
    return;
}

void inode_manager::remove_file(uint32_t inum){
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
    struct inode *ino = get_inode(inum);
    if(ino){
      int temp;
      if(ino->size%BLOCK_SIZE != 0) {
       temp = (ino->size/BLOCK_SIZE)+1;
      }else {
         temp = (ino->size/BLOCK_SIZE);
      }
      for(int i=0;i<NDIRECT && temp;i++) {
        bm->free_block(ino->blocks[i]);
      }
      if(temp > NDIRECT) {
        blockid_t inDirectBlocks[NINDIRECT];
        bm->read_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
        for(int i=0;i<temp-NDIRECT;i++) {
          bm->free_block(inDirectBlocks[i]);
        }
      }
      free_inode(inum);
    }
  return;
}

std::pair<int,int> inode_manager::filling_directBlocks(struct inode *ino,const char *buf, int size){
    int temp;
    int index = 0;
    int size1 = size;
    if(size%BLOCK_SIZE != 0) {
      temp =  (size1/BLOCK_SIZE)+1;
    } else {
      temp =  (size1/BLOCK_SIZE);
    }
    for(int i1=0;i1<NDIRECT && i1<temp;i1++) {
      char *buf1 = (char *)malloc(BLOCK_SIZE);
      memcpy(buf1,buf+index, BLOCK_SIZE);
      bm->write_block(ino->blocks[i1],buf1);
      if(size1 < BLOCK_SIZE) {
        ino->size = ino->size + size1;
        index = index + size1;
      } else {
         ino->size = ino->size + BLOCK_SIZE;
         index = index + BLOCK_SIZE;
      }
      size1 = size1 - BLOCK_SIZE;
    }
    return std::make_pair(index, size1);
}

void inode_manager::filling_indirectBlocks(struct inode *ino,const char *buf, int size1,int index){
    blockid_t inDirectBlocks[NINDIRECT];
    bm->read_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
    int temp;
    if(size1%BLOCK_SIZE != 0) {
       temp = (size1/BLOCK_SIZE)+1;
    } else {
      temp = (size1/BLOCK_SIZE);
    }
    for(int i1=0;i1<temp;i1++) {
      char *buf1 = (char *)malloc(BLOCK_SIZE);
      memcpy(buf1,buf+index, BLOCK_SIZE);
      bm->write_block(inDirectBlocks[i1],buf1);
      if(size1 < BLOCK_SIZE) {
        ino->size = ino->size + size1;
        index = index + size1;
      } else {
        ino->size = ino->size + BLOCK_SIZE;
        index = index + BLOCK_SIZE;
      }
      size1 = size1 - BLOCK_SIZE;
    }
    bm->write_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
    return;
}

void inode_manager::alloc_free_blocks(struct inode *ino,int size) {
    int temp1,temp2;
    if(size%BLOCK_SIZE != 0) {
      temp1 = (size/BLOCK_SIZE)+1;
    } else {
      temp1 = (size/BLOCK_SIZE);
    }
     if(ino->size%BLOCK_SIZE != 0) {
      temp2 = (ino->size/BLOCK_SIZE)+1;
    } else {
      temp2 = (ino->size/BLOCK_SIZE);
    }
    if(temp1 >= NDIRECT && temp2 >= NDIRECT) {
      temp1 = temp1 - NDIRECT;
      temp2 = temp2 - NDIRECT;
      blockid_t inDirectBlocks[NINDIRECT];
      bm->read_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
      if(temp1 > temp2) {
        for(int i=temp2; i<temp1;i++) {
          inDirectBlocks[i] = bm->alloc_block();
        }
      } else {
        for(int i=temp1; i<temp2;i++) {
          bm->free_block(inDirectBlocks[i]);
        }
      }
      bm->write_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
    }
    else if(temp1 >= NDIRECT && temp2 <= NDIRECT) {
      for(int i=temp2; i<NDIRECT;i++) {
        ino->blocks[i] = bm->alloc_block();
      }
      blockid_t inDirectBlocks[NINDIRECT];
      ino->blocks[NDIRECT] = bm->alloc_block();
      temp1 = temp1 - NDIRECT;
      for(int i=0; i<temp1;i++) {
        inDirectBlocks[i] = bm->alloc_block();
      }
      bm->write_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
    }
    else if(temp2 >= NDIRECT && temp1 <= NDIRECT) {
      for(int i=temp1; i<NDIRECT;i++) {
        bm->free_block(ino->blocks[i]);
      }
      blockid_t inDirectBlocks[NINDIRECT];
      bm->read_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
      temp2 = temp2 - NDIRECT;
      for(int i=0; i<temp2;i++) {
        bm->free_block(inDirectBlocks[i]);
      }
      bm->write_block(ino->blocks[NDIRECT], (char*)inDirectBlocks);
      bm->free_block(ino->blocks[NDIRECT]);
    }
    else if(temp1 <= NDIRECT && temp2 <= NDIRECT) {
      if(temp1 > temp2) {
        for(int i=temp2; i<temp1;i++) {
          ino->blocks[i] = bm->alloc_block();
        }
      } else {
        for(int i=temp1; i<temp2;i++) {
          bm->free_block(ino->blocks[i]);
        }
      }
  }
  ino->size = 0;
}
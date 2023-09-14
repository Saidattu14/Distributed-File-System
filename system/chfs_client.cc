// chfs client.  implements FS operations using extent and lock server
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "chfs_client.h"
#include "extent_client.h"

/* 
 * Your code here for Lab2A:
 * Here we treat each ChFS operation(especially write operation such as 'create', 
 * 'write' and 'symlink') as a transaction, your job is to use write ahead log 
 * to achive all-or-nothing for these transactions.
 */

chfs_client::chfs_client(std::string extent_dst) {
    ec = new extent_client(extent_dst);
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

chfs_client::inum
chfs_client::n2i(std::string n) {
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
chfs_client::filename(inum inum) {
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
chfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
chfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    extent_protocol::attr a;
      if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }
    printf("isdir: %lld is not a dir\n", inum);
    return false;
}

bool
chfs_client::issymlink(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    //return (! isfile(inum) && !isSymlink(inum));
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_SYMLINK) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }
    return false;
}


int
chfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
chfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;
    
    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}

int
chfs_client::getsymlink(inum inum, symlinkinfo &sin)
{
    int r = OK;
    printf("getsymlink %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    sin.atime = a.atime;
    sin.mtime = a.mtime;
    sin.ctime = a.ctime;
    sin.size = a.size;
release:
    return r;
}

#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
// Your code here for Lab2A: add logging to ensure atomicity
int
chfs_client::setattr(inum ino, size_t size)
{
    int r = OK;
    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    std::string buf;
    if (ec->get(ino, buf) != OK) {
        return r;
    }
    buf.resize(size);
    if(ec->put(ino,buf) != OK) {
        return r;
    }
    return r;
}

// Your code here for Lab2A: add logging to ensure atomicity
int
chfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    lookup(parent,name,found,ino_out);
    if(found) {
        return chfs_client::EXIST;
    }
    if (ec->create(extent_protocol::T_FILE,ino_out) != OK) {
        return r;
    }
    string buf,buf1;
    if (ec->get(parent, buf) != OK) {
        return r;
    }
    buf1 = buf;
    string ss = "*";
    buf.push_back(ss[0]);
    buf = buf + name;
    buf = buf + "&";
    buf = buf + filename(ino_out);
    buf = buf + "&";
    buf.push_back(ss[0]);
    //ec->operation_put_transaction_data(transaction_id,buf,parent);
    if(ec->put(parent,buf) != OK) {
        ec->remove(ino_out);
        return r;
    }
    return r;
}

// Your code here for Lab2A: add logging to ensure atomicity
int
chfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    lookup(parent,name,found,ino_out);
    if(found) {
        return chfs_client::EXIST;
    }
    if (ec->create(extent_protocol::T_DIR,ino_out) != OK) {
        return r;
    }
    string buf;
    if (ec->get(parent, buf) != OK) {
        return r;
    }
    string ss = "*";
    buf.push_back(ss[0]);
    buf = buf + name;
    buf = buf + "&";
    buf = buf + filename(ino_out);
    buf = buf + "&";
    buf.push_back(ss[0]);
    if(ec->put(parent,buf) != OK) {
        ec->remove(ino_out);
        return r;
    }
    return r;
}

int
chfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;
    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    std::list<dirent> list;
    readdir(parent,list);
    for(auto i : list) {
        if(i.name == name) {
            found = true;
            ino_out = i.inum;
        }
    }
    found = false;
    return r;
}

int
chfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    string buf;
     if (ec->get(dir, buf) != OK) {
        return r;
    }
    string ss = "*";
    for(int i=0;i<buf.length();i++) {
        if(buf[i] == ss[0]) {
            string s,s1,s2="&";
            int c =0;
            bool st = false;
            for(int j=i+1;j<buf.length();j++) {
                c++;
                if(buf[j] == ss[0]) {
                    break;
                }
                if(buf[j] == s2[0]) {
                    st = true;
                }
                if(!st && buf[j] != s2[0]) {
                  s.push_back(buf[j]);
                } else if(buf[j] != s2[0]) {
                  s1.push_back(buf[j]); 
                }
            }
            dirent d;
            d.inum = stoi(s1);
            d.name = s;
            list.push_back(d);
            i = i + c;
        }
    }
    return r;
}

int
chfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;
    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string buf;
    if (ec->get(ino, buf) != OK) {
        return r;
    }
    for(int i=off;i<off+size && i<buf.length();i++) {
        data.push_back(buf[i]);
    }
    return r;
}

// Your code here for Lab2A: add logging to ensure atomicity
int
chfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;
    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    
    string buf;
    if (ec->get(ino, buf) != OK) {
        return r;
    }
    if(off + size < buf.length()) {
        int index = 0;
        for(int i=off;i<off+size;i++) {
            buf[i] = data[index];
            index++;
        }
    } else {
        if(off > buf.length()) {
            for(int i=buf.length();i<off;i++) {
               buf.push_back('\0');
            }
        }
        int index = 0;
        for(int i=off;i<off+size;i++) {
            if (i >= buf.length()) {
                buf.push_back(data[index]);
            } else {
                buf[i] = data[index];
            }
            index++;
        }
    }
    bytes_written = size;
    if(ec->put(ino,buf) != OK) {
        return r;
    }
    return r;
}

// Your code here for Lab2A: add logging to ensure atomicity
int chfs_client::unlink(inum parent,const char *name)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    std::string buf,buf2;
    if (ec->get(parent, buf) != OK) {
        return r;
    }
    inum ino;
    bool eql1 = false;
    string ss = "*";
    for(int i=0;i<buf.length();i++) {
        if(buf[i] == ss[0]) {
            string s,s1,s2="&";
            int c =0;
            bool st = false;
            for(int j=i+1;j<buf.length();j++) {
                c++;
                if(buf[j] == ss[0]) {
                    break;
                }
                if(buf[j] == s2[0]) {
                    st = true;
                }
                if(!st && buf[j] != s2[0]) {
                  s.push_back(buf[j]);
                } else if(buf[j] != s2[0]) {
                  s1.push_back(buf[j]); 
                }
            }
            if(s == name) {
              eql1 = true;  
              ino = stoi(s1);
              buf2 = buf.substr(0, i) + buf.substr(i+c+1, buf.length());
              break;
            }
            i = i + c;
        }
    }
    if(!eql1) {
       return NOENT;
    }
    
    if ((r = ec->remove(ino)) != OK) {
        return r;
    }
    
    if ((r = ec->put(parent, buf2)) != OK) {
        return r;
    }
    
    return r;
}

int
chfs_client::symlink(const char *link, inum parent,
                    const char *name, inum &ino_out) {

    int r = OK;
    bool found = false;
    lookup(parent,name,found,ino_out);
    if(found) {
        return chfs_client::EXIST;
    }
    if (ec->create(extent_protocol::T_SYMLINK,ino_out) != OK) {
        return r;
    }
    std::string buf;
    if (ec->get(parent, buf) != OK) {
        return r;
    }
    //ec->operation_put_transaction_data(transaction_id,string(link),ino_out);
    if (ec->put(ino_out, string(link)) != OK) {
        return r;
    }
    std::string ss = "*";
    buf.push_back(ss[0]);
    buf = buf + name;
    buf = buf + "&";
    buf = buf + filename(ino_out);
    buf = buf + "&";
    buf.push_back(ss[0]);
    //ec->operation_put_transaction_data(transaction_id,buf,parent);
    if(ec->put(parent,buf) != OK) {
        ec->remove(ino_out);
        return r;
    }
    return r;
}

int chfs_client::readlink(inum ino, std::string &link) {
    int r = OK;
    if ((r = ec->get(ino, link)) != OK){
        return r;
    }
    return r;
}
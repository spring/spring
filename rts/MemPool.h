#ifndef CMEMPOOL_H
#define CMEMPOOL_H

#include <new>

const int MAX_MEM_SIZE=200;

class CMemPool
{
 public:
  CMemPool();
  void *Alloc(size_t n);
  void Free(void *p,size_t n);
  ~CMemPool();
 private:
  void* nextFree[MAX_MEM_SIZE+1];
  int poolSize[MAX_MEM_SIZE+1];
};
extern CMemPool mempool;
#endif

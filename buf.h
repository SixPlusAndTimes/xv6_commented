// buf这个结构是对磁盘一个block的描述，
struct buf {
  int flags; // 看本文件下面的两个宏
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[BSIZE]; // 位于内存的对磁盘一个扇区的内容的拷贝。BSIZE 与磁盘的一个扇区的大小相同即512字节
};
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk


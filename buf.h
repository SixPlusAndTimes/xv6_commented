// buf这个结构是对磁盘一个block的描述，
struct buf {
  int flags; // 看本文件下面的两个宏
  uint dev;
  uint blockno;     // 对应磁盘上的块号
  struct sleeplock lock;
  uint refcnt;      // 进程读写完缓冲区后，refcnt--。当refcnt为0时，表示没有进程与缓冲块关联。但这个缓冲块依然和磁盘的对应块关联
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue, ide.c
  uchar data[BSIZE]; // 位于内存的对磁盘一个扇区的内容的拷贝。BSIZE 与磁盘的一个扇区的大小相同即512字节
};
#define B_VALID 0x2  // buffer has been read from disk, 如果这个标志被设置为1，就表示缓冲块与硬盘内容一致，进程可以放心地使用这个缓冲块
                      // 每次从刚磁盘读如缓冲块，或者刚把缓冲块写入磁盘，都把这个标志为写为1。
                      // 相反，如果这个标志为0，则表示磁盘数据块与内存不一致，其他进程不能直接它，必须先从磁盘读如对应的数据。
#define B_DIRTY 0x4  // buffer needs to be written to disk


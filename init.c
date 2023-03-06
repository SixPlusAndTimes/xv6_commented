// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
// 这是 initcode.S 中使用exec调用的程序
char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){ // 打开控制台文件，在xv6中控制台也对应一个iniode，inode->type = T_DEV
    // 创建控制台文件
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf(1, "init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      // 子进程执行exec运行shell程序
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    // 父进程，也就是执行本文件的进程等待 shell进程退出
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}

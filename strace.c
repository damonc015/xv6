#include "types.h"
#include "user.h"
#include "trace.h"
#include "syscall.h"

static char *syscall_name[] = {
  [SYS_fork]    "fork",
  [SYS_exit]    "exit",
  [SYS_wait]    "wait",
  [SYS_pipe]    "pipe",
  [SYS_read]    "read",
  [SYS_kill]    "kill",
  [SYS_exec]    "exec",
  [SYS_fstat]   "fstat",
  [SYS_chdir]   "chdir",
  [SYS_dup]     "dup",
  [SYS_getpid]  "getpid",
  [SYS_sbrk]    "sbrk",
  [SYS_sleep]   "sleep",
  [SYS_uptime]  "uptime",
  [SYS_open]    "open",
  [SYS_write]   "write",
  [SYS_mknod]   "mknod",
  [SYS_unlink]  "unlink",
  [SYS_link]    "link",
  [SYS_mkdir]   "mkdir",
  [SYS_close]   "close",
  [SYS_trace]  "trace",
  [SYS_t_toggle] "t_toggle",
};

int
main(int argc, char *argv[])
{
  if(argc < 2){
    printf(2, "Usage: strace [on|off|run <command>]\n");
    exit();
  }

  //adding option checking
  int i, j;
  const char* j_str;

  //exclusive_flag = 0;

  for(i = 1; i < argc; i++){
    if(strcmp(argv[i], "-e") == 0){
      if(i + 1 >= argc){
        printf(2, "Usage: strace -e <syscall>\n");
        exit();
      }
      
      // Find and set exclusive flag for specified syscall
      for(j = 1; j < 24; j++){
        j_str = syscall_name[j];
        if(strcmp(argv[i + 1], j_str) == 0){
          excid(j);  // Set exclusive flag for this syscall
          t_toggle(TRACE_ON);  // Enable tracing
          exit();  // Exit and let user enter their command in shell
        }
      }
      printf(2, "Unknown syscall: %s\n", argv[i + 1]);
      exit();
    } else if(strcmp(argv[i], "-f") == 0){
      t_toggle(TRACE_ON);
      set_fail_flag();  
      exit();  
    } else if(strcmp(argv[i], "-s") == 0){
      t_toggle(TRACE_ON);
      set_success_flag();   
      exit();  
    }
  }

  if(strcmp(argv[1], "run") == 0){
    if(argc < 3){
        printf(2, "Usage: strace run <command>\n");
        exit();
    }
    
    // Check trace_flag
    int was_tracing = get_trace_flag();
    t_toggle(TRACE_ON);
    
    int pid = fork();
    if(pid == 0){
        if(exec(argv[2], &argv[2]) < 0){
            printf(2, "exec failed: %s\n", argv[2]);
            exit();
        }
    } else if (pid > 0) {
        wait();
        t_toggle(was_tracing);
    } else {
        printf(2, "fork failed\n");
        exit();
    }
  } else if(strcmp(argv[1], "on") == 0){
    t_toggle(TRACE_ON);
  } else if(strcmp(argv[1], "off") == 0){
    t_toggle(TRACE_OFF);
  } else {
    printf(2, "Invalid argument. Use 'on' or 'off'.\n");
    exit();
  }
  exit();
  return 0;
}
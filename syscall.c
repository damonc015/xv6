#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "trace.h"

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  if(addr >= proc->sz || addr+4 > proc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;

  if(addr >= proc->sz)
    return -1;
  *pp = (char*)addr;
  ep = (char*)proc->sz;
  for(s = *pp; s < ep; s++)
    if(*s == 0)
      return s - *pp;
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint(proc->tf->esp + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;

  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= proc->sz || (uint)i+size > proc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}




extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_trace(void);
extern int sys_t_toggle(void);
extern int sys_excid(void);
extern int sys_get_trace_flag(void);
extern int sys_set_success_flag(void);
extern int sys_set_fail_flag(void);
extern int trace_flag;

int shell_reading_command = 0;
int exclusive_flag = 0;
int success_flag = 0;
int fail_flag = 0;

static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_t_toggle] sys_t_toggle,
[SYS_excid]   sys_excid,
[SYS_get_trace_flag] sys_get_trace_flag,
[SYS_set_success_flag] sys_set_success_flag,
[SYS_set_fail_flag] sys_set_fail_flag,
};

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
  [SYS_excid]   "excid",
  [SYS_get_trace_flag] "get_trace_flag",
  [SYS_set_success_flag] "set_success_flag",
  [SYS_set_fail_flag] "set_fail_flag",
};

void
syscall(void)
{
  int num;
  int return_value;

  num = proc->tf->eax;
  if (proc->tracer && strncmp(proc->name, "strace", 6) != 0) {
    const char *syscall_name_str = syscall_name[num];
    if(syscall_name_str && exclusive_flag && success_flag){
        // cprintf("DEBUG: Handling syscall with exclusive=%d success=%d flags\n", exclusive_flag, success_flag);
        if(num == exclusive_flag){
            return_value = syscalls[num]();
            if(return_value >= 0) {
                cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", 
                        proc->pid, proc->name, syscall_name_str, return_value);
            }
            
            if(num == SYS_exit && strncmp(proc->name, "sh", 2) != 0){
                exclusive_flag = 0;
                success_flag = 0;
                proc->tracer = 0;
            }
        } else {
            if (num == SYS_exit) {
                exclusive_flag = 0;
                success_flag = 0;
            }
            return_value = syscalls[num]();
        }
        proc->tf->eax = return_value;
        return;
    }
    else if(syscall_name_str && exclusive_flag && fail_flag){
        // cprintf("DEBUG: Handling syscall with exclusive=%d fail=%d flags\n", exclusive_flag, fail_flag);
        if(num == exclusive_flag){
            return_value = syscalls[num]();
            if(return_value < 0) {
                cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", 
                        proc->pid, proc->name, syscall_name_str, return_value);
            }
            
            if(num == SYS_exit && strncmp(proc->name, "sh", 2) != 0){
                exclusive_flag = 0;
                fail_flag = 0;
                proc->tracer = 0;
            }
        } else {
            if (num == SYS_exit) {
                exclusive_flag = 0;
                fail_flag = 0;
            }
            return_value = syscalls[num]();
        }
        proc->tf->eax = return_value;
        return;
    }
    else if(syscall_name_str && exclusive_flag){
        // cprintf("DEBUG: Handling syscall with exclusive=%d flag only\n", exclusive_flag);
        if(num == exclusive_flag){
            cprintf("TRACE: pid = %d | command name = %s | syscall = %s", 
                    proc->pid, proc->name, syscall_name_str);
            
            return_value = syscalls[num]();
            
            cprintf(" | return value = %d\n", return_value);
            
            // If this syscall is exit and it's not from the shell,
            // clear the exclusive flag and tracing
            if(num == SYS_exit && strncmp(proc->name, "sh", 2) != 0){
                exclusive_flag = 0;
                proc->tracer = 0;
            }
        } else {
            if (num == SYS_exit) {
                exclusive_flag = 0;
            }
            // Just execute the syscall without tracing if it's not the one we're watching
            return_value = syscalls[num]();
        }
        proc->tf->eax = return_value;
        return;
    }
    else if(syscall_name_str && success_flag){
        // cprintf("DEBUG: Handling syscall with success=%d flag only\n", success_flag);
        // Store the flag for next shellcmd after strace on
        if (!trace_flag) {
            return_value = syscalls[num]();
            proc->tf->eax = return_value;
            return;
        }
        
        // clear flags
        if(num == SYS_exit) {
            cprintf("TRACE: pid = %d | command name = %s | syscall = %s\n", 
                    proc->pid, proc->name, syscall_name_str, success_flag, fail_flag);
            success_flag = 0;
            proc->tracer = 0;
            return_value = syscalls[num]();
        } else {
            return_value = syscalls[num]();
            if(return_value >= 0) {
                cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", 
                        proc->pid, proc->name, syscall_name_str, return_value, success_flag, fail_flag);
            }
        }
        
        proc->tf->eax = return_value;
        return;
    }
    else if(syscall_name_str && fail_flag){
        // cprintf("DEBUG: Handling syscall with fail=%d flag only\n", fail_flag);
        // Store the flag for next shellcmd after strace on
        if (!trace_flag) {
            return_value = syscalls[num]();
            proc->tf->eax = return_value;
            return;
        }
        
        // clear flags
        if(num == SYS_exit) {
            fail_flag = 0;
            proc->tracer = 0;
            return_value = syscalls[num]();
        } else {
            return_value = syscalls[num]();
            if(return_value < 0) {
                cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", 
                        proc->pid, proc->name, syscall_name_str, return_value, success_flag, fail_flag);
            }
        }
        
        proc->tf->eax = return_value;
        return;
    }
    else if (syscall_name_str && exclusive_flag == 0) {
      if (strncmp(proc->name, "sh", 2) == 0) {
        cprintf("TRACE: pid = %d | command name = %s | syscall = %s", 
                  proc->pid, proc->name, syscall_name_str);
                  return_value = syscalls[num]();
          cprintf(" | return value = %d\n", return_value);
          proc->tf->eax = return_value;
        
        // Only trace the shell's read syscall when reading a command
        if (shell_reading_command || num == SYS_exec) {
          cprintf("TRACE: pid = %d | command name = %s | syscall = %s", 
                  proc->pid, proc->name, syscall_name_str);
                  return_value = syscalls[num]();
          cprintf(" | return value = %d\n", return_value);
          proc->tf->eax = return_value;
          return;
        }
      } 
      else {
        // Trace non-shell processes normally
        if (num == SYS_write) {
          return_value = syscalls[num]();
          cprintf("TRACE: pid = %d | command name = %s | syscall = %s | return value = %d\n", 
                  proc->pid, proc->name, syscall_name_str, return_value);
          proc->tf->eax = return_value;
        } else {
          // space after sys call exit
          if (num == SYS_exit) {
            cprintf("TRACE: pid = %d | command name = %s | syscall = %s", 
                    proc->pid, proc->name, syscall_name_str);
            cprintf(" | return value = 0\n");  
            return_value = syscalls[num]();  
            proc->tf->eax = return_value;
          } else {
            cprintf("TRACE: pid = %d | command name = %s | syscall = %s", 
                    proc->pid, proc->name, syscall_name_str);
            return_value = syscalls[num]();
            cprintf(" | return value = %d\n", return_value);
            proc->tf->eax = return_value;
          }
        }
      }
    } /*else {
      cprintf("TRACE: pid = %d | command name = %s | unknown syscall\n", 
              proc->pid, proc->name);
      proc->tf->eax = -1;
    }*/
  } else {
    if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
      proc->tf->eax = syscalls[num]();
    } else {
      cprintf("%d %s: unknown sys call %d\n", proc->pid, proc->name, num);
      proc->tf->eax = -1;
    }
  }
}
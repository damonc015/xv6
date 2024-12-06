#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "trace.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_trace(void)
{
    int on_off;

    if(argint(0, &on_off) < 0)  // Get argument (0 or 1) from user space
        return -1;

    proc->tracer = on_off;  // Set the process's tracer flag directly
    return 0;
}

int sys_t_toggle(void)
{
    int on_off;

    if (argint(0, &on_off) < 0)
        return -1;

    trace_flag = on_off;
    return 0;
}

int sys_excid(void)
{
    int sysid;

    if (argint(0, &sysid) < 0)
        return -1;

    exclusive_flag = sysid;
    return 0;
}

int sys_get_trace_flag(void)
{
    return trace_flag;
}

int sys_set_success_flag(void)
{
    success_flag = 1;
    return 0;
}

int sys_set_fail_flag(void)
{
    fail_flag = 1;
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <chcore/launcher.h>
#include <chcore/syscall.h>

#include "arch.h"
#include "utils.h"
#include "packets.h"
#include "gdb_signals.h"

size_t *entry_stack_ptr;

#define THREAD_NUMBER 64

struct thread_id_t
{
  /* XXX: tids are indexes from 0. */
  pid_t pid;
  pid_t tid;
  int stat;
};

struct thread_list_t
{
  struct thread_id_t t[THREAD_NUMBER];
  struct thread_id_t *curr;
  int len;
  int ptrace_cap;
} threads;

#define BREAKPOINT_NUMBER 64

struct debug_breakpoint_t
{
  size_t addr;
  size_t orig_data;
} breakpoints[BREAKPOINT_NUMBER];

uint8_t tmpbuf[0x20000];
bool attach = false;

void update_breakpoint_pc(pid_t tid)
{
  size_t pc;
  usys_ptrace(CHCORE_PTRACE_PEEKUSER, threads.ptrace_cap,
              tid, (void*)(SZ * PC), &pc);
  pc -= sizeof(break_instr);
  for (int i = 0; i < BREAKPOINT_NUMBER; i++)
    if (breakpoints[i].addr == pc)
    {
      usys_ptrace(CHCORE_PTRACE_POKEUSER, threads.ptrace_cap,
                  tid, (void *)(SZ * PC), (void *)pc);
      break;
    }
}

bool try_wait_breakpoint()
{
  int ret;
  struct timespec ts;
  ret = usys_ptrace(CHCORE_PTRACE_WAITBKPT, threads.ptrace_cap, 0, NULL, NULL);
  if (!ret) {
  /*
   * XXX: breakpoint does not stop all threads immediately.
   *	  So wait a few ticks to guarantee all threads are stopped.
   */
    ts.tv_sec = 0;
    ts.tv_nsec = 50 * 1000;
    nanosleep(&ts, NULL);
    return true;
  }

  return false;
}

void update_thread_status()
{
  void *info_buf;
  int ret;

  ret = usys_ptrace(CHCORE_PTRACE_GETTHREADS, threads.ptrace_cap,
		    0, NULL, &info_buf);
  /* XXX: currently no info is provided */
  for (int i = 0; i < ret; i++) {
    threads.t[i].pid = 1;
    threads.t[i].tid = i + 1;
    threads.t[i].stat = 0;
    if (usys_ptrace(CHCORE_PTRACE_GETBKPTINFO, threads.ptrace_cap, i + 1, NULL, NULL) == 1)
      update_breakpoint_pc(i);
  }
  threads.len = ret;
}

void set_curr_thread(pid_t tid)
{
  for (int i = 0; i < THREAD_NUMBER; i++)
    if (threads.t[i].tid == tid)
    {
      threads.curr = &threads.t[i];
      break;
    }
}

void stop_threads()
{
  struct timespec ts;
  usys_ptrace(CHCORE_PTRACE_SUSPEND, threads.ptrace_cap, 0, NULL, NULL);
  /*
   * XXX: Suspend does not stop all threads immediately.
   *	  So wait a few ticks to guarantee all threads are stopped.
   */
  ts.tv_sec = 0;
  ts.tv_nsec = 50 * 1000;
  nanosleep(&ts, NULL);

  update_thread_status();
}

void init_tids(const pid_t pid)
{
  char dirname[64];
  DIR *dir;
  struct dirent *ent;
  int i = 0;

  snprintf(dirname, sizeof dirname, "/proc/%d/task/", (int)pid);
  dir = opendir(dirname);
  if (!dir)
    perror("opendir()");
  while ((ent = readdir(dir)) != NULL)
  {
    if (ent->d_name[0] == '.')
      continue;
    threads.t[i].pid = pid;
    threads.t[i].tid = atoi(ent->d_name);
    threads.len++;
    i++;
  }
  closedir(dir);
}

void prepare_resume_reply(uint8_t *buf, int gdb_sig)
{
  /* FIXME: handle when current thread exit */
  /* if (WIFEXITED(threads.curr->stat)) */
  /*   sprintf(buf, "W%02x", gdb_signal_from_host(WEXITSTATUS(threads.curr->stat))); */
  sprintf(buf, "T%02xthread:p%02x.%02x;", gdb_sig, threads.curr->pid,
	  threads.curr->tid);
}

void read_auxv(void)
{
  printf("%s not supported\n", __func__);
}

void process_xfer(const char *name, char *args)
{
  const char *mode = args;
  args = strchr(args, ':');
  *args++ = '\0';
  if (!strcmp(name, "features") && !strcmp(mode, "read"))
    write_packet(FEATURE_STR);
  if (!strcmp(name, "auxv") && !strcmp(mode, "read"))
    read_auxv();
  if (!strcmp(name, "exec-file") && !strcmp(mode, "read"))
  {
    uint8_t proc_exe_path[20], file_path[256] = {'l'};
    sprintf(proc_exe_path, "/proc/%d/exe", threads.t[0].pid);
    realpath(proc_exe_path, file_path + 1);
    write_packet(file_path);
  }
}

void process_query(char *payload)
{
  const char *name;
  char *args;

  args = strchr(payload, ':');
  if (args)
    *args++ = '\0';
  name = payload;
  if (!strcmp(name, "C"))
  {
    snprintf(tmpbuf, sizeof(tmpbuf), "QCp%02x.%02x", threads.curr->pid, threads.curr->tid);
    write_packet(tmpbuf);
  }
  if (!strcmp(name, "Attached"))
  {
    if (attach)
      write_packet("1");
    else
      write_packet("0");
  }
  if (!strcmp(name, "Offsets"))
    write_packet("");
  if (!strcmp(name, "Supported"))
    write_packet("PacketSize=8000;qXfer:features:read+;multiprocess+");
  if (!strcmp(name, "Symbol"))
    write_packet("OK");
  if (name == strstr(name, "ThreadExtraInfo"))
  {
    args = payload;
    args = 1 + strchr(args, ',');
    write_packet("41414141");
  }
  if (!strcmp(name, "TStatus"))
    write_packet("");
  if (!strcmp(name, "Xfer"))
  {
    name = args;
    args = strchr(args, ':');
    *args++ = '\0';
    return process_xfer(name, args);
  }
  if (!strcmp(name, "fThreadInfo"))
  {
    struct thread_id_t *ptr = threads.t;
    uint8_t pid_buf[20];
    assert(threads.len > 0);
    strcpy(tmpbuf, "m");
    for (int i = 0; i < threads.len; i++, ptr++)
    {
      snprintf(pid_buf, sizeof(pid_buf), "p%02x.%02x,", ptr->pid, ptr->tid);
      strcat(tmpbuf, pid_buf);
    }
    tmpbuf[strlen(tmpbuf) - 1] = '\0';
    write_packet(tmpbuf);
  }
  if (!strcmp(name, "sThreadInfo"))
    write_packet("l");
}

static int gdb_open_flags_to_system_flags(size_t flags)
{
  int ret;
  switch (flags & 3)
  {
  case 0:
    ret = O_RDONLY;
    break;
  case 1:
    ret = O_WRONLY;
    break;
  case 2:
    ret = O_RDWR;
    break;
  default:
    assert(0);
    return 0;
  }

  assert(!(flags & ~(size_t)(3 | 0x8 | 0x200 | 0x400 | 0x800)));

  if (flags & 0x8)
    ret |= O_APPEND;
  if (flags & 0x200)
    ret |= O_CREAT;
  if (flags & 0x400)
    ret |= O_TRUNC;
  if (flags & 0x800)
    ret |= O_EXCL;
  return ret;
}

void process_vpacket(char *payload)
{
  const char *name;
  char *args;
  int gdb_sig;
  args = strchr(payload, ';');
  if (args)
    *args++ = '\0';
  name = payload;

  if (!strcmp("Cont", name))
  {
    if (args[0] == 'c')
    {
      usys_ptrace(CHCORE_PTRACE_CONT, threads.ptrace_cap, 0, NULL, NULL);

      /*
       * TODO: Currently only interrupt on Ctrl-C from gdb client
       *       and breakpoints. Need to handle traced process's fault
       *       and thread exit.
       */
      while (true) {
        if (try_input_interrupt()) {
          gdb_sig = GDB_SIGNAL_INT;
          break;
        }
        if (try_wait_breakpoint()) {
          gdb_sig = GDB_SIGNAL_TRAP;
          break;
        }
        usys_yield();
      }
      stop_threads();

      prepare_resume_reply(tmpbuf, gdb_sig);
      write_packet(tmpbuf);
    }
    if (args[0] == 's')
    {
      assert(args[1] == ':');
      char *dot = strchr(args, '.');
      assert(dot);
      pid_t tid = strtol(dot + 1, NULL, 16);
      set_curr_thread(tid);
      usys_ptrace(CHCORE_PTRACE_SINGLESTEP, threads.ptrace_cap,
		  threads.curr->tid, NULL, NULL);
      prepare_resume_reply(tmpbuf, GDB_SIGNAL_TRAP);
      write_packet(tmpbuf);
    }
  }
  if (!strcmp("Cont?", name))
    write_packet("vCont;c;C;s;S");
  if (!strcmp("Kill", name))
  {
    printf("kill not supported\n");
    write_packet("OK");
  }
  if (!strcmp("MustReplyEmpty", name))
    write_packet("");
  if (name == strstr(name, "File:"))
  {
    char *operation = strchr(name, ':') + 1;
    if (operation == strstr(operation, "open:"))
    {
      char result[10];
      char *parameter = strchr(operation, ':') + 1;
      char *end = strchr(parameter, ',');
      int len, fd;
      size_t flags, mode;
      assert(end != NULL);
      *end = 0;
      len = strlen(parameter);
      hex2mem(parameter, tmpbuf, len);
      tmpbuf[len / 2] = '\0';
      parameter += len + 1;
      assert(sscanf(parameter, "%zx,%zx", &flags, &mode) == 2);
      flags = gdb_open_flags_to_system_flags(flags);
      assert((mode & ~(int64_t)0777) == 0);
      fd = open(tmpbuf, flags, mode);
      sprintf(result, "F%x", fd);
      write_packet(result);
    }
    else if (operation == strstr(operation, "close:"))
    {
      char *parameter = strchr(operation, ':') + 1;
      size_t fd;
      assert(sscanf(parameter, "%zx", &fd) == 1);
      close(fd);
      write_packet("F0");
    }
    else if (operation == strstr(operation, "pread:"))
    {
      char *parameter = strchr(operation, ':') + 1;
      size_t fd, size, offset;
      assert(sscanf(parameter, "%zx,%zx,%zx", &fd, &size, &offset) == 3);
      assert(size >= 0);
      if (size * 2 > PACKET_BUF_SIZE)
        size = PACKET_BUF_SIZE / 2;
      assert(offset >= 0);
      char *buf = malloc(size);
      FILE *fp = fdopen(fd, "rb");
      fseek(fp, offset, SEEK_SET);
      int ret = fread(buf, 1, size, fp);
      sprintf(tmpbuf, "F%x;", ret);
      write_binary_packet(tmpbuf, buf, ret);
      free(buf);
    }
    else if (operation == strstr(operation, "setfs:"))
    {
      char *endptr;
      int64_t pid = strtol(operation + 6, &endptr, 16);
      assert(*endptr == 0);
      write_packet("F0");
    }
    else
      write_packet("");
  }
}

bool set_breakpoint(pid_t tid, size_t addr, size_t length)
{
  int i;
  for (i = 0; i < BREAKPOINT_NUMBER; i++)
    if (breakpoints[i].addr == 0)
    {
      size_t data;
      usys_ptrace(CHCORE_PTRACE_PEEKDATA, threads.ptrace_cap,
		  tid, (void *)addr, &data);
      breakpoints[i].orig_data = data;
      breakpoints[i].addr = addr;
      assert(sizeof(break_instr) <= length);
      memcpy((void *)&data, break_instr, sizeof(break_instr));
      usys_ptrace(CHCORE_PTRACE_POKEDATA, threads.ptrace_cap,
		  tid, (void *)addr, (void *)data);
      break;
    }
  if (i == BREAKPOINT_NUMBER)
    return false;
  else
    return true;
}

bool remove_breakpoint(pid_t tid, size_t addr, size_t length)
{
  int i;
  for (i = 0; i < BREAKPOINT_NUMBER; i++)
    if (breakpoints[i].addr == addr)
    {
      usys_ptrace(CHCORE_PTRACE_POKEDATA, threads.ptrace_cap,
		  tid, (void *)addr, (void *)breakpoints[i].orig_data);
      breakpoints[i].addr = 0;
      break;
    }
  if (i == BREAKPOINT_NUMBER)
    return false;
  else
    return true;
}

size_t restore_breakpoint(size_t addr, size_t length, size_t data)
{
  for (int i = 0; i < BREAKPOINT_NUMBER; i++)
  {
    size_t bp_addr = breakpoints[i].addr;
    size_t bp_size = sizeof(break_instr);
    if (bp_addr && bp_addr + bp_size > addr && bp_addr < addr + length)
    {
      for (size_t j = 0; j < bp_size; j++)
      {
        if (bp_addr + j >= addr && bp_addr + j < addr + length)
          ((uint8_t *)&data)[bp_addr + j - addr] = ((uint8_t *)&breakpoints[i].orig_data)[j];
      }
    }
  }
  return data;
}

void process_packet()
{
  uint8_t *inbuf = inbuf_get();
  int inbuf_size = inbuf_end();
  uint8_t *packetend_ptr = (uint8_t *)memchr(inbuf, '#', inbuf_size);
  int packetend = packetend_ptr - inbuf;
  assert('$' == inbuf[0]);
  char request = inbuf[1];
  char *payload = (char *)&inbuf[2];
  inbuf[packetend] = '\0';

  uint8_t checksum = 0;
  uint8_t checksum_str[3];
  for (int i = 1; i < packetend; i++)
    checksum += inbuf[i];
  assert(checksum == (hex(inbuf[packetend + 1]) << 4 | hex(inbuf[packetend + 2])));
  switch (request)
  {
  case 'D':
    usys_ptrace(CHCORE_PTRACE_DETACH, threads.ptrace_cap, 0, NULL, NULL);
    exit(0);
  case 'g':
  {
    regs_struct regs;
    uint8_t regbuf[20];
    tmpbuf[0] = '\0';
    usys_ptrace(CHCORE_PTRACE_GETREGS, threads.ptrace_cap,
                threads.curr->tid, NULL, &regs);
    for (int i = 0; i < ARCH_REG_NUM; i++)
    {
      mem2hex((void *)(((size_t *)&regs) + regs_map[i].idx), regbuf, regs_map[i].size);
      regbuf[regs_map[i].size * 2] = '\0';
      strcat(tmpbuf, regbuf);
    }
    write_packet(tmpbuf);
    break;
  }
  case 'H':
    if ('g' == *payload++)
    {
      pid_t tid;
      char *dot = strchr(payload, '.');
      assert(dot);
      tid = strtol(dot + 1, NULL, 16);
      if (tid > 0)
        set_curr_thread(tid);
    }
    write_packet("OK");
    break;
  case 'm':
  {
    size_t maddr, mlen, mdata;
    assert(sscanf(payload, "%zx,%zx", &maddr, &mlen) == 2);
    if (mlen * SZ * 2 > 0x20000)
    {
      puts("Buffer overflow!");
      exit(-1);
    }
    for (int i = 0; i < mlen; i += SZ)
    {
      int ret;
      ret = usys_ptrace(CHCORE_PTRACE_PEEKDATA, threads.ptrace_cap,
			  threads.curr->tid, (void *)(maddr + i), &mdata);
      if (ret)
      {
        sprintf(tmpbuf, "E%02x", errno);
        break;
      }
      mdata = restore_breakpoint(maddr, sizeof(size_t), mdata);
      mem2hex((void *)&mdata, tmpbuf + i * 2, (mlen - i >= SZ ? SZ : mlen - i));
    }
    tmpbuf[mlen * 2] = '\0';
    write_packet(tmpbuf);
    break;
  }
  case 'M':
  {
    size_t maddr, mlen, mdata;
    assert(sscanf(payload, "%zx,%zx", &maddr, &mlen) == 2);
    for (int i = 0; i < mlen; i += SZ)
    {
      if (mlen - i >= SZ)
        hex2mem(payload + i * 2, (void *)&mdata, SZ);
      else
      {
        usys_ptrace(CHCORE_PTRACE_PEEKDATA, threads.ptrace_cap,
                    threads.curr->tid, (void *)(maddr + i), &mdata);
        hex2mem(payload + i * 2, (void *)&mdata, mlen - i);
      }
      usys_ptrace(CHCORE_PTRACE_POKEDATA,threads.ptrace_cap,
		  threads.curr->tid, (void *)(maddr + i), (void *)mdata);
    }
    write_packet("OK");
    break;
  }
  case 'p':
  {
    int i = strtol(payload, NULL, 16);
    if (i >= ARCH_REG_NUM && i != EXTRA_NUM)
    {
      write_packet("E01");
      break;
    }
    size_t regdata;
    if (i == EXTRA_NUM)
    {
      usys_ptrace(CHCORE_PTRACE_PEEKUSER, threads.ptrace_cap,
                  threads.curr->tid, (void *)(SZ * EXTRA_REG), &regdata);
      mem2hex((void *)&regdata, tmpbuf, EXTRA_SIZE);
      tmpbuf[EXTRA_SIZE * 2] = '\0';
    }
    else
    {
      usys_ptrace(CHCORE_PTRACE_PEEKUSER, threads.ptrace_cap,
                  threads.curr->tid, (void *)(long)(SZ * regs_map[i].idx), &regdata);
      mem2hex((void *)&regdata, tmpbuf, regs_map[i].size);
      tmpbuf[regs_map[i].size * 2] = '\0';
    }
    write_packet(tmpbuf);
    break;
  }
  case 'P':
  {
    int i = strtol(payload, &payload, 16);
    assert('=' == *payload++);
    if (i >= ARCH_REG_NUM && i != EXTRA_NUM)
    {
      write_packet("E01");
      break;
    }
    size_t regdata = 0;
    hex2mem(payload, (void *)&regdata, SZ * 2);
    if (i == EXTRA_NUM)
      usys_ptrace(CHCORE_PTRACE_POKEUSER, threads.ptrace_cap,
		  threads.curr->tid, (void *)(SZ * EXTRA_REG), (void *)regdata);
    else
      usys_ptrace(CHCORE_PTRACE_POKEUSER, threads.ptrace_cap, threads.curr->tid,
		  (void *)(long)(SZ * regs_map[i].idx), (void *)regdata);
    write_packet("OK");
    break;
  }
  case 'q':
    process_query(payload);
    break;
  case 'v':
    process_vpacket(payload);
    break;
  case 'X':
  {
    size_t maddr, mlen, mdata;
    int offset, new_len;
    assert(sscanf(payload, "%zx,%zx:%n", &maddr, &mlen, &offset) == 2);
    payload += offset;
    new_len = unescape(payload, (char *)packetend_ptr - payload);
    assert(new_len == mlen);
    for (int i = 0; i < mlen; i += SZ)
    {
      if (mlen - i >= SZ)
        memcpy((void *)&mdata, payload + i, SZ);
      else
      {
        usys_ptrace(CHCORE_PTRACE_PEEKDATA, threads.ptrace_cap,
                  threads.curr->tid, (void *)(maddr + i), &maddr);
        memcpy((void *)&mdata, payload + i, mlen - i);
      }
      usys_ptrace(CHCORE_PTRACE_POKEDATA, threads.ptrace_cap, threads.curr->tid,
		  (void *)(maddr + i), (void *)mdata);
    }
    write_packet("OK");
    break;
  }
  case 'Z':
  {
    size_t type, addr, length;
    assert(sscanf(payload, "%zx,%zx,%zx", &type, &addr, &length) == 3);
    if (type == 0 && sizeof(break_instr))
    {
      bool ret = set_breakpoint(threads.curr->tid, addr, length);
      if (ret)
        write_packet("OK");
      else
        write_packet("E01");
    }
    else
      write_packet("");
    break;
  }
  case 'z':
  {
    size_t type, addr, length;
    assert(sscanf(payload, "%zx,%zx,%zx", &type, &addr, &length) == 3);
    if (type == 0)
    {
      bool ret = remove_breakpoint(threads.curr->tid, addr, length);
      if (ret)
        write_packet("OK");
      else
        write_packet("E01");
    }
    else
      write_packet("");
    break;
  }
  case 'T':
    write_packet("OK");
    break;
  case '?':
    write_packet("S05");
    break;
  default:
    write_packet("");
  }

  inbuf_erase_head(packetend + 3);
}

void get_request()
{
  while (true)
  {
    read_packet();
    process_packet();
    write_flush();
  }
}

int main(int argc, char *argv[])
{
  pid_t pid;
  char **next_arg = &argv[1];
  char *arg_end, *target = NULL;
  int stat;

  if (*next_arg != NULL && strcmp(*next_arg, "--attach") == 0)
  {
    attach = true;
    next_arg++;
  }

  target = *next_arg;
  next_arg++;

  if (target == NULL || *next_arg == NULL)
  {
    printf("Usage : gdbserver 127.0.0.1:1234 a.out or gdbserver --attach 127.0.0.1:1234 2468\n");
    exit(-1);
  }

  if (attach)
  {
    /* not supported */
#if 0
    pid = atoi(*next_arg);
    init_tids(pid);
    for (int i = 0, n = 0; i < THREAD_NUMBER && n < threads.len; i++)
      if (threads.t[i].tid)
      {
        if (ptrace(PTRACE_ATTACH, threads.t[i].tid, NULL, NULL) < 0)
        {
          perror("ptrace()");
          return -1;
        }
        if (waitpid(threads.t[i].tid, &threads.t[i].stat, __WALL) < 0)
        {
          perror("waitpid");
          return -1;
        }
        ptrace(PTRACE_SETOPTIONS, threads.t[i].tid, NULL, PTRACE_O_TRACECLONE);
        n++;
      }
#endif
  }
  else
  {
    struct new_process_caps new_process_caps;
    create_traced_process(argc - 2, next_arg, &new_process_caps);
    threads.t[0].pid = 1;
    threads.t[0].tid = 1;
    threads.t[0].stat = 0;
    threads.len = 1;

    /* TODO: trace to pid */
    threads.ptrace_cap = usys_ptrace(CHCORE_PTRACE_ATTACH,
				     new_process_caps.mt_cap, 0, NULL, NULL);
    if (threads.ptrace_cap < 0) {
	    printf("failed to attach\n");
	    return threads.ptrace_cap;
    }
  }
  threads.curr = &threads.t[0];
  remote_prepare(target);
  stop_threads();
  get_request();
  return 0;
}

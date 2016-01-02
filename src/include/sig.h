#ifndef SIG_H
#define SIG_H

#include <syscall.h>
#include <unistd.h>
#include <signal.h>

#ifdef __x86_64__
#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004
static inline int dosemu_arch_prctl(int code, void *addr)
{
  return syscall(SYS_arch_prctl, code, addr);
}
#endif

extern void add_thread_callback(void (*cb)(void *), void *arg, const char *name);
#ifdef __linux__
extern void SIG_init(void);
extern void SIG_close(void);
#endif

/* signals for Linux's process control of consoles */
#define SIG_RELEASE     SIGUSR1
#define SIG_ACQUIRE     SIGUSR2

extern void SIGNAL_save( void (*signal_call)(void *), void *arg, size_t size,
	const char *name );
extern void handle_signals(void);
extern void do_periodic_stuff(void);
extern void sig_ctx_prepare(int tid);
extern void sig_ctx_restore(int tid);

extern int sigchld_register_handler(pid_t pid, void (*handler)(void));
extern int sigchld_enable_handler(pid_t pid, int on);
extern void registersig(int sig, void (*handler)(struct sigcontext *));
extern void init_handler(struct sigcontext *scp, int async);
#ifdef __x86_64__
extern void deinit_handler(struct sigcontext *scp);
#else
#define deinit_handler(scp)
#endif

void dosemu_fault(int, siginfo_t *, void *);

static inline pid_t gettid(void)
{
  return syscall(SYS_gettid);
}

extern pid_t dosemu_tid;

#endif

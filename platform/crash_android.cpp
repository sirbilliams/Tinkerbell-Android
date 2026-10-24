// Crash backtrace in the log (Android; also glibc desktop). A fatal signal writes the signal, the fault
// address and a backtrace (module + offset, for addr2line against the unstripped libtinkerbell.so) to
// tb_log_path() ("Share log" in the launcher sends it), then the default action runs.
// port: not in the original game; platform-only.
#include "tb.h"
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#ifdef __ANDROID__
#include <unwind.h>
#include <android/log.h>
#else
#include <execinfo.h>
#endif

namespace {
int g_fd = -1;

void out(const char *s) {
    if (g_fd >= 0) { ssize_t r = write(g_fd, s, strlen(s)); (void)r; }
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_FATAL, "TinkerBell", s);
#else
    ssize_t r2 = write(2, s, strlen(s)); (void)r2;
#endif
}

void outf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void outf(const char *fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    out(buf);
}

void frame(int n, void *pc) {
    Dl_info di;
    if (dladdr(pc, &di) && di.dli_fname) {
        const char *base = strrchr(di.dli_fname, '/');
        base = base ? base + 1 : di.dli_fname;
        outf("  #%02d pc %08lx  %s (%s+%lx)\n", n, (unsigned long)((char *)pc - (char *)di.dli_fbase), base,
             di.dli_sname ? di.dli_sname : "?", di.dli_saddr ? (unsigned long)((char *)pc - (char *)di.dli_saddr) : 0UL);
    } else {
        outf("  #%02d pc %p\n", n, pc);
    }
}

#ifdef __ANDROID__
struct Trace { int n; };
_Unwind_Reason_Code step(struct _Unwind_Context *c, void *arg) {
    Trace *t = (Trace *)arg;
    uintptr_t pc = _Unwind_GetIP(c);
    if (pc) frame(t->n++, (void *)pc);
    return t->n >= 48 ? _URC_END_OF_STACK : _URC_NO_REASON;
}
#endif

void handler(int sig, siginfo_t *si, void *) {
    out("\n=== CRASH ===\n");
    outf("signal %d (%s), fault address %p\n", sig, strsignal(sig), si ? si->si_addr : nullptr);
    out("backtrace:\n");
#ifdef __ANDROID__
    Trace t = {0};
    _Unwind_Backtrace(step, &t);
#else
    void *bt[48];
    int n = backtrace(bt, 48);
    for (int i = 0; i < n; i++) frame(i, bt[i]);
#endif
    if (g_fd >= 0) fsync(g_fd);
    signal(sig, SIG_DFL);   // the default action ends the process (and makes the system's own tombstone)
    raise(sig);
}

// Opens the log for appending when the first game thread runs; before main() writes tb_log_path().
__attribute__((constructor)) void install() {
    static char stack[64 * 1024];
    stack_t ss; ss.ss_sp = stack; ss.ss_size = sizeof stack; ss.ss_flags = 0;
    sigaltstack(&ss, nullptr);
    struct sigaction sa; memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    static const int sigs[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT, SIGTRAP };
    for (int s : sigs) sigaction(s, &sa, nullptr);
}
}

// Called by the process entry once the log path is known (platform/core.cpp, after fopen of the log).
extern "C" void tb_crash_open_log() {
    const char *p = tb_log_path();
    if (p && *p && g_fd < 0) g_fd = open(p, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (getenv("TB_CRASH_TEST")) { volatile int *bad = (volatile int *)(size_t)strtoul("16", 0, 10); *bad = 1; }   // test hook for the handler
}

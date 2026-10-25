#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <bleed/syscalls.h>
#include <common.h>

#define PID_SCAN 4096

static char state_char(task_state_t s) {
    switch (s) {
    case TASK_READY:   return 'R';
    case TASK_RUNNING: return 'R';
    case TASK_BLOCKED: return 'S';
    case TASK_STOPPED: return 'T';
    case TASK_ZOMBIE:  return 'Z';
    case TASK_DEAD:    return 'X';
    default:           return '?';
    }
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    int show_kernel = 0, wide = 0;

    // ps aux, ps -ef, ps -e all mean "everything" here
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (*a == '-')
            a++;
        for (; *a; a++) {
            if (*a == 'e' || *a == 'A' || *a == 'a' || *a == 'x')
                show_kernel = 1;
            else if (*a == 'f' || *a == 'u' || *a == 'l')
                wide = 1;
        }
    }

    uint64_t want = _taskcount();
    uint64_t seen = 0;

    if (wide)
        printf("%5s %5s %1s %9s %s\n", "PID", "PPID", "S", "RSS", "CMD");
    else
        printf("%5s %s\n", "PID", "CMD");

    for (uint64_t pid = 0; pid < PID_SCAN && seen < want; pid++) {
        user_task_info_t info;
        if (_taskinfo(pid, &info) != 0)
            continue;
        seen++;
        if (info.privilege_level == PRIVILEGE_KERNEL && !show_kernel)
            continue;

        const char *name = info.name[0] ? info.name : "?";
        if (!wide) {
            printf("%5llu %s\n", (unsigned long long)info.id, name);
            continue;
        }

        user_task_usage_t usage;
        if (_taskusage(pid, &usage) != 0)
            memset(&usage, 0, sizeof(usage));
        char rss[16];
        human_size(rss, sizeof(rss), usage.mem_pages * 4096ULL);
        printf("%5llu %5llu %c %9s %s%s%s\n", (unsigned long long)info.id, (unsigned long long)info.ppid,
               state_char(info.state), rss,
               info.privilege_level == PRIVILEGE_KERNEL ? "[" : "", name,
               info.privilege_level == PRIVILEGE_KERNEL ? "]" : "");
    }
    return 0;
}

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <common.h>

// 4k, 1M, 2x512 and so on
static long long parse_size(const char *s) {
    char *end;
    long long v = strtoll(s, &end, 10);
    switch (*end) {
    case 'c': end++; break;
    case 'w': v *= 2; end++; break;
    case 'b': v *= 512; end++; break;
    case 'k': case 'K': v *= 1024; end++; break;
    case 'M': v *= 1024 * 1024; end++; break;
    case 'G': v *= 1024LL * 1024 * 1024; end++; break;
    default: break;
    }
    if (*end == 'B')
        end++;
    if (*end == 'x')
        return v * parse_size(end + 1);
    if (*end) {
        warn("invalid number: '%s'", s);
        exit(1);
    }
    return v;
}

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    const char *in_path = NULL, *out_path = NULL;
    long long ibs = 512, obs = 512, count = -1, skip = 0, seek = 0;
    int notrunc = 0, sync = 0, noerror = 0, quiet = 0, progress = 0;

    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (!eq) {
            warn("unrecognized operand '%s'", argv[i]);
            return 1;
        }
        *eq = '\0';
        const char *key = argv[i], *val = eq + 1;

        if (!strcmp(key, "if"))         in_path = val;
        else if (!strcmp(key, "of"))    out_path = val;
        else if (!strcmp(key, "bs"))    ibs = obs = parse_size(val);
        else if (!strcmp(key, "ibs"))   ibs = parse_size(val);
        else if (!strcmp(key, "obs"))   obs = parse_size(val);
        else if (!strcmp(key, "count")) count = parse_size(val);
        else if (!strcmp(key, "skip") || !strcmp(key, "iseek")) skip = parse_size(val);
        else if (!strcmp(key, "seek") || !strcmp(key, "oseek")) seek = parse_size(val);
        else if (!strcmp(key, "conv")) {
            notrunc = strstr(val, "notrunc") != NULL;
            sync    = strstr(val, "sync") != NULL;
            noerror = strstr(val, "noerror") != NULL;
        } else if (!strcmp(key, "status")) {
            quiet    = !strcmp(val, "none");
            progress = !strcmp(val, "progress");
        } else if (!strcmp(key, "iflag") || !strcmp(key, "oflag")) {
        } else {
            warn("unrecognized operand '%s'", key);
            return 1;
        }
    }
    if (ibs <= 0 || obs <= 0) {
        warn("invalid block size");
        return 1;
    }

    int in = STDIN_FILENO, out = STDOUT_FILENO;
    if (in_path && (in = open(in_path, O_RDONLY)) < 0)
        return warn("failed to open '%s': %s", in_path, strerror(errno));

    if (out_path) {
        int flags = O_WRONLY | O_CREAT;
        if (!notrunc && !seek)
            flags |= O_TRUNC;
        if ((out = open(out_path, flags)) < 0)
            return warn("failed to open '%s': %s", out_path, strerror(errno));
    }

    // pipes cant seek so read the skipped part away instead
    char *ibuf = malloc((size_t)ibs);
    if (skip && lseek(in, skip * ibs, SEEK_SET) < 0) {
        for (long long i = 0; i < skip; i++) {
            if (read(in, ibuf, (size_t)ibs) <= 0)
                break;
        }
    }
    if (seek && lseek(out, seek * obs, SEEK_SET) < 0)
        return warn("cannot seek '%s': %s", out_path ? out_path : "stdout", strerror(errno));

    long long full_in = 0, part_in = 0, full_out = 0, part_out = 0, bytes = 0;
    char *obuf = malloc((size_t)obs);
    long long opos = 0;
    double start = now_seconds(), last_report = start;
    int status = 0;

    while (count < 0 || full_in + part_in < count) {
        ssize_t r = read(in, ibuf, (size_t)ibs);
        if (r < 0) {
            status = warn("error reading '%s': %s", in_path ? in_path : "stdin", strerror(errno));
            if (noerror)
                continue;
            break;
        }
        if (r == 0)
            break;

        if (r == ibs) {
            full_in++;
        } else {
            part_in++;
            if (sync) {
                memset(ibuf + r, 0, (size_t)(ibs - r));
                r = ibs;
            }
        }

        // regroup into obs sized writes
        for (ssize_t done = 0; done < r;) {
            size_t take = (size_t)(obs - opos);
            if ((ssize_t)take > r - done)
                take = (size_t)(r - done);
            memcpy(obuf + opos, ibuf + done, take);
            opos += (long long)take;
            done += (ssize_t)take;
            if (opos == obs) {
                if (write_all(out, obuf, (size_t)obs) < 0) {
                    status = warn("error writing '%s': %s", out_path ? out_path : "stdout", strerror(errno));
                    goto finish;
                }
                full_out++;
                bytes += obs;
                opos = 0;
            }
        }

        if (progress && now_seconds() - last_report >= 1.0) {
            last_report = now_seconds();
            fprintf(stderr, "\r%lld bytes copied, %.0f s", bytes, last_report - start);
        }
    }

    if (opos > 0) {
        if (write_all(out, obuf, (size_t)opos) < 0)
            status = warn("error writing '%s': %s", out_path ? out_path : "stdout", strerror(errno));
        else {
            part_out++;
            bytes += opos;
        }
    }

finish:
    if (progress)
        fputc('\n', stderr);
    if (!quiet) {
        double secs = now_seconds() - start;
        char rate[32];
        human_size(rate, sizeof(rate), secs > 0 ? (unsigned long long)((double)bytes / secs) : (unsigned long long)bytes);
        fprintf(stderr, "%lld+%lld records in\n%lld+%lld records out\n", full_in, part_in, full_out, part_out);
        fprintf(stderr, "%lld bytes copied, %.3f s, %s/s\n", bytes, secs, rate);
    }

    if (in != STDIN_FILENO)
        close(in);
    if (out != STDOUT_FILENO)
        close(out);
    free(ibuf);
    free(obuf);
    return status;
}

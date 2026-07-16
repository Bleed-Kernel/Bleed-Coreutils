#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fs/file.h>
#include <syscalls/ioctl.h>
#include <syscalls/open.h>
#include <syscalls/close.h>
#include <syscalls/exit.h>

#define EXPECTED_ARGS   4

static int parse_request(const char* str, unsigned long* out){
    if (!str || *str == '\0'){
        return -1;
    }

    char* end = NULL;
    unsigned long val = strtoul(str, &end, 0);
    if (end == str || *end != '\0'){
        return -1;
    }

    *out = val;
    return 0;
}

int main(int argc, const char** argv){
    if (argc != EXPECTED_ARGS){
        printf("Bad Argument Count, expected %i, got %i (ioctl <device> <request> <arg>)\n", EXPECTED_ARGS, argc);
        _exit(1);
    }

    char filepath[4096];
    strncpy(filepath, argv[1], sizeof(filepath) - 1);
    filepath[sizeof(filepath) - 1] = '\0';

    unsigned long request = 0;
    if (parse_request(argv[2], &request) != 0){
        printf("Bad request code '%s', expected decimal or 0x-prefixed hex value\n", argv[2]);
        _exit(1);
    }

    char argbuf[4096];
    strncpy(argbuf, argv[3], sizeof(argbuf) - 1);
    argbuf[sizeof(argbuf) - 1] = '\0';

    void* ioctl_arg;
    char* numeric_end = NULL;
    unsigned long numeric_val = strtoul(argbuf, &numeric_end, 0);
    if (numeric_end != argbuf && *numeric_end == '\0'){
        ioctl_arg = (void*)numeric_val;
    }else{
        ioctl_arg = (void*)argbuf;
    }

    int fd = (int)_open(filepath, O_RDWR);
    if (fd < 0){
        printf("File or device %s not found or could not be opened (fd=%d)!\n", filepath, fd);
        _exit(1);
    }

    int result = _ioctl(fd, request, ioctl_arg);
    if (result < 0){
        printf("ioctl failed with error %d\n", result);
        _close(fd);
        _exit(1);
    }

    printf("ioctl succeeded, result=%d\n", result);

    int close_result = _close(fd);
    if (close_result < 0){
        printf("Close failed with error %d\n", close_result);
        _exit(1);
    }

    return 0;
}
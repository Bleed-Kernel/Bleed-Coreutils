#include <bleed/audio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <common.h>

static void list(const struct audio_outputs *outs) {
    if (outs->selected == AUDIO_OUTPUT_AUTO)
        puts("selected: auto");
    else
        printf("selected: %d\n", outs->selected);

    for (uint32_t i = 0; i < outs->count; i++) {
        const struct audio_output *o = &outs->out[i];

        const char *jack = "";
        if (o->flags & AUDIO_OUTPUT_DETECT)
            jack = (o->flags & AUDIO_OUTPUT_PLUGGED) ? ", plugged in" : ", nothing plugged in";
        printf("%u  %-28s %s%s\n", i, o->name, (o->flags & AUDIO_OUTPUT_ACTIVE) ? "on" : "off", jack);
    }
}

static int type_of(const char *word) {
    if (!strcmp(word, "speakers") || !strcmp(word, "speaker"))
        return AUDIO_OUT_SPEAKER;
    if (!strcmp(word, "headphones"))
        return AUDIO_OUT_HEADPHONES;
    if (!strcmp(word, "line"))
        return AUDIO_OUT_LINE;
    return 0;
}

// a number, auto, or the first output of a kind
static int pick(const struct audio_outputs *outs, const char *word, int32_t *index) {
    if (!strcmp(word, "auto")) {
        *index = AUDIO_OUTPUT_AUTO;
        return 1;
    }

    if (isdigit((unsigned char)word[0])) {
        *index = atoi(word);
        return (uint32_t)*index < outs->count;
    }

    int type = type_of(word);
    for (uint32_t i = 0; type && i < outs->count; i++) {
        if (outs->out[i].type == type) {
            *index = (int32_t)i;
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    warn_init(argv[0]);
    if (argc > 2) {
        fprintf(stderr, "usage: audioout [auto | number | speakers | headphones | line]\n");
        return 1;
    }

    int fd = open("/dev/audio0", O_RDONLY);
    if (fd < 0)
        return warn("no sound card");

    struct audio_outputs outs;
    if (ioctl(fd, AUDIO_IOC_GET_OUTPUTS, &outs) < 0) {
        if (errno == ENOTTY)
            return warn("this sound card has no outputs to pick from");
        return warn("%s", strerror(errno));
    }

    if (argc == 2) {
        int32_t index;
        if (!pick(&outs, argv[1], &index))
            return warn("no output called %s", argv[1]);
        if (ioctl(fd, AUDIO_IOC_SET_OUTPUT, &index) < 0 || ioctl(fd, AUDIO_IOC_GET_OUTPUTS, &outs) < 0)
            return warn("%s", strerror(errno));
    }

    list(&outs);
    return 0;
}

#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    struct passwd *pw = getpwuid(geteuid());
    puts(pw && pw->pw_name ? pw->pw_name : "root");
    return 0;
}

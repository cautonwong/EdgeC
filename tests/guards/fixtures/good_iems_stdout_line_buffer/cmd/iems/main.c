#include <stdio.h>

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    return 0;
}

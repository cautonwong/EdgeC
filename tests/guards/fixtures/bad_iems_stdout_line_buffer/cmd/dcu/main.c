#include <stdio.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    return 0;
}

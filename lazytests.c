#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    char *p;

    printf(1, "Lazy allocation test\n");

    // Solicitar 10 páginas (40 KB)
    p = sbrk(4096 * 10);
    printf(1, "sbrk(40960) succeeded\n");

    // Acceder solo a dos páginas
    p[0] = 'A';
    printf(1, "Accessed page 1\n");

    p[4096] = 'B';
    printf(1, "Accessed page 2\n");

    // Las otras 8 páginas nunca se usan
    printf(1, "Test passed\n");

    exit();
}
#include <stdio.h>
#include <korax/scanner.h>

int main(void)
{
    printf("starting scan (Ctrl+C to stop)...\n");
    int res = scan();
    if (res != SCAN_OK){
        fprintf(stderr, "Scan failed: %d\n", res);
        return 1;
    }
    return 0;
}

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/curses/ui_csv.h"
static void check(const char *s, const char *want)
{
    FILE *f = tmpfile();
    char buf[512] = {0};
    size_t n;
    assert(f);
    csv_write_field(f, s);
    assert(fflush(f) == 0);
    rewind(f);
    n = fread(buf, 1, sizeof(buf) - 1, f);
    assert(n == strlen(want));
    assert(strcmp(buf, want) == 0);
    fclose(f);
}
int main(void)
{
    check("Call-ID", "\"Call-ID\"");
    check("John, Doe", "\"John, Doe\"");
    check("a\"b", "\"a\"\"b\"");
    check("a\r\nb", "\"a\r\nb\"");
    check("=1+1", "\"'=1+1\"");
    check("+123", "\"'+123\"");
    check("@SUM(A1)", "\"'@SUM(A1)\"");
    check(NULL, "\"\"");
    puts("CSV escaping OK");
    return 0;
}

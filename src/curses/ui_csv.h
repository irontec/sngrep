/* Safe CSV field writer; SIP headers are untrusted input. */
#ifndef SNGREP_UI_CSV_H
#define SNGREP_UI_CSV_H
#include <stdio.h>
static inline void csv_write_field(FILE *out, const char *s)
{
    const unsigned char *p = (const unsigned char *)(s ? s : "");
    fputc('"', out);
    /* Do not let Excel interpret untrusted SIP headers as formulas. */
    if (*p == '=' || *p == '+' || *p == '-' || *p == '@' ||
        *p == '\r' || *p == '\n' || *p == '\t')
        fputc('\'', out);
    for (; *p; p++) {
        if (*p == '"') fputc('"', out);
        fputc(*p, out);
    }
    fputc('"', out);
}
#endif

/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file test_017.c
 *
 * Unit tests for the CSV field writer (csv_write_field) used by the
 * call list CSV export of the save dialog.
 */
#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/util.h"

static void
check(const char *str, const char *want)
{
    FILE *f = tmpfile();
    char buf[512] = { 0 };
    size_t n;

    assert(f);
    csv_write_field(f, str);
    assert(fflush(f) == 0);
    rewind(f);
    n = fread(buf, 1, sizeof(buf) - 1, f);
    assert(n == strlen(want));
    assert(strcmp(buf, want) == 0);
    fclose(f);
}

int
main(void)
{
    // Plain values are always quoted
    check("Call-ID", "\"Call-ID\"");
    check("34600000000@sip.example.com", "\"34600000000@sip.example.com\"");
    // Separators and line breaks are kept inside the quotes
    check("John, Doe", "\"John, Doe\"");
    check("a\r\nb", "\"a\r\nb\"");
    // Double quotes are doubled
    check("a\"b", "\"a\"\"b\"");
#ifdef CSV_FORMULA_ESCAPE
    // Values that spreadsheets would treat as formulas are prefixed
    check("=1+1", "\"'=1+1\"");
    check("+34600000000@sip.example.com", "\"'+34600000000@sip.example.com\"");
    check("-1", "\"'-1\"");
    check("@SUM(A1)", "\"'@SUM(A1)\"");
    check("\tx", "\"'\tx\"");
#else
    // Values are written as they are, even if they look like formulas
    check("=1+1", "\"=1+1\"");
    check("+34600000000@sip.example.com", "\"+34600000000@sip.example.com\"");
    check("@SUM(A1)", "\"@SUM(A1)\"");
#endif
    // Empty and NULL values are written as an empty field
    check("", "\"\"");
    check(NULL, "\"\"");
    return 0;
}

#include <strings.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <limits.h>

bool mocked = false;
int mocked_rc = 0;

void mock(bool m) {
    mocked = m;
}
void mocked_value(int v)
{
    mocked_rc = v;
}

int _vsnprintf(char *str, size_t size, const char *format, ...)
{
    if (mocked) {
        return mocked_rc;
    }
    va_list ap;
    va_start(ap, format);
    int rc = vsnprintf(str, size, format, ap);
    va_end(ap);
    return rc;
}



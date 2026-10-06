#include "bignum.h"
#include <stdio.h>
#include <math.h>

void fmt_num(char *out, size_t cap, double v)
{
    static const char *const suffix[] = { "", "K", "M", "B", "T", "QA", "QI", "SX", "SP", "OC", "NO" };
    int tier = 0;
    double a;
    if (v != v) { /* NaN guard: never show garbage */
        snprintf(out, cap, "0");
        return;
    }
    a = fabs(v);
    if (a < 10000.0) {
        snprintf(out, cap, "%.0f", v);
        return;
    }
    while (a >= 1000.0 && tier < 10) {
        a /= 1000.0;
        v /= 1000.0;
        tier++;
    }
    if (a >= 1000.0) {
        snprintf(out, cap, "%.2fE%d", v / pow(10.0, floor(log10(a))), (int)floor(log10(a)) + tier * 3);
        return;
    }
    snprintf(out, cap, a < 10.0 ? "%.2f%s" : a < 100.0 ? "%.1f%s" : "%.0f%s", v, suffix[tier]);
}

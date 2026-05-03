#include "ADAA_Math.h"
#include <cmath>

namespace ADAA_Math
{
    double fx_cubic(double x)
    {
        return x - (x * x * x) / 3.0;
    }

    double F1_cubic(double x)
    {
        double x2 = x * x;
        return (x2 / 2.0) - (x2 * x2 / 12.0);
    }

    double fx_hardclip(double x)
    {
        if (x > 1.5) return 1.5;
        if (x < -1.5) return -1.5;
        return x;
    }

    double F1_hardclip(double x)
    {
        if (x > 1.5) return 1.5 * x - 1.125;
        if (x < -1.5) return -1.5 * x - 1.125;
        return 0.5 * x * x;
    }

} // namespace ADAA_Math
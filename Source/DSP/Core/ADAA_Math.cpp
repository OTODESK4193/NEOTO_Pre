#pragma execution_character_set("utf-8")
#include "ADAA_Math.h"
#include <cmath>
#include <algorithm>

namespace ADAA_Math
{
    // ==============================================================================
    // Šù‘¶‚ÌŠî–{ŠÖ”ŽÀ‘•
    // ==============================================================================
    double fx_cubic(double x)
    {
        if (x > 1.0)  return 2.0 / 3.0;
        if (x < -1.0) return -2.0 / 3.0;
        return x - (x * x * x) / 3.0;
    }

    double F1_cubic(double x)
    {
        if (x > 1.0)  return (2.0 / 3.0) * x - 0.25;
        if (x < -1.0) return (-2.0 / 3.0) * x - 0.25;
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

    // ==============================================================================
    // Chebyshev Even: f(x) = x + 0.15*x^2 - 0.1*x^3
    // ==============================================================================
    constexpr double ce_L_p = 2.0;
    constexpr double ce_L_n = -1.0;
    constexpr double ce_max_p = ce_L_p + 0.15 * (ce_L_p * ce_L_p) - 0.1 * (ce_L_p * ce_L_p * ce_L_p);
    constexpr double ce_max_n = ce_L_n + 0.15 * (ce_L_n * ce_L_n) - 0.1 * (ce_L_n * ce_L_n * ce_L_n);
    constexpr double ce_F1_Lp = (ce_L_p * ce_L_p) / 2.0 + 0.05 * (ce_L_p * ce_L_p * ce_L_p) - 0.025 * (ce_L_p * ce_L_p * ce_L_p * ce_L_p);
    constexpr double ce_F1_Ln = (ce_L_n * ce_L_n) / 2.0 + 0.05 * (ce_L_n * ce_L_n * ce_L_n) - 0.025 * (ce_L_n * ce_L_n * ce_L_n * ce_L_n);
    constexpr double ce_C1_p = ce_F1_Lp - ce_max_p * ce_L_p;
    constexpr double ce_C1_n = ce_F1_Ln - ce_max_n * ce_L_n;
    constexpr double ce_F2_Lp = (ce_L_p * ce_L_p * ce_L_p) / 6.0 + 0.0125 * (ce_L_p * ce_L_p * ce_L_p * ce_L_p) - 0.005 * (ce_L_p * ce_L_p * ce_L_p * ce_L_p * ce_L_p);
    constexpr double ce_F2_Ln = (ce_L_n * ce_L_n * ce_L_n) / 6.0 + 0.0125 * (ce_L_n * ce_L_n * ce_L_n * ce_L_n) - 0.005 * (ce_L_n * ce_L_n * ce_L_n * ce_L_n * ce_L_n);
    constexpr double ce_C2_p = ce_F2_Lp - (ce_max_p * ce_L_p * ce_L_p / 2.0 + ce_C1_p * ce_L_p);
    constexpr double ce_C2_n = ce_F2_Ln - (ce_max_n * ce_L_n * ce_L_n / 2.0 + ce_C1_n * ce_L_n);

    double fx_chebyshev_even(double x) {
        if (x > ce_L_p) return ce_max_p;
        if (x < ce_L_n) return ce_max_n;
        return x + 0.15 * (x * x) - 0.1 * (x * x * x);
    }
    double F1_chebyshev_even(double x) {
        if (x > ce_L_p) return ce_max_p * x + ce_C1_p;
        if (x < ce_L_n) return ce_max_n * x + ce_C1_n;
        double x2 = x * x; return (x2 / 2.0) + 0.05 * (x2 * x) - 0.025 * (x2 * x2);
    }
    double F2_chebyshev_even(double x) {
        if (x > ce_L_p) return ce_max_p * (x * x) / 2.0 + ce_C1_p * x + ce_C2_p;
        if (x < ce_L_n) return ce_max_n * (x * x) / 2.0 + ce_C1_n * x + ce_C2_n;
        double x2 = x * x; double x3 = x2 * x; return (x3 / 6.0) + 0.0125 * (x3 * x) - 0.005 * (x3 * x2);
    }

    // ==============================================================================
    // Chebyshev Odd: f(x) = x - 0.2*x^3
    // ==============================================================================
    constexpr double co_L = 1.29099;
    constexpr double co_max = co_L - 0.2 * (co_L * co_L * co_L);
    constexpr double co_F1_L = (co_L * co_L) / 2.0 - 0.05 * (co_L * co_L * co_L * co_L);
    constexpr double co_C1 = co_F1_L - co_max * co_L;
    constexpr double co_F2_L = (co_L * co_L * co_L) / 6.0 - 0.01 * (co_L * co_L * co_L * co_L * co_L);
    constexpr double co_C2 = co_F2_L - (co_max * co_L * co_L / 2.0 + co_C1 * co_L);

    double fx_chebyshev_odd(double x) {
        if (x > co_L) return co_max;
        if (x < -co_L) return -co_max;
        return x - 0.2 * (x * x * x);
    }
    double F1_chebyshev_odd(double x) {
        if (x > co_L) return co_max * x + co_C1;
        if (x < -co_L) return -co_max * x + co_C1;
        double x2 = x * x; return (x2 / 2.0) - 0.05 * (x2 * x2);
    }
    double F2_chebyshev_odd(double x) {
        if (x > co_L) return co_max * (x * x) / 2.0 + co_C1 * x + co_C2;
        if (x < -co_L) return -co_max * (x * x) / 2.0 + co_C1 * x - co_C2;
        double x2 = x * x; double x3 = x2 * x; return (x3 / 6.0) - 0.01 * (x3 * x2);
    }

    // ==============================================================================
    // Tube (V76s): f(x) = x - 0.2*x^3
    // ==============================================================================
    constexpr double tb_L_p = 1.5;
    constexpr double tb_L_n = -1.0;
    constexpr double tb_max_p = tb_L_p - 0.2 * (tb_L_p * tb_L_p * tb_L_p);
    constexpr double tb_max_n = tb_L_n - 0.2 * (tb_L_n * tb_L_n * tb_L_n);
    constexpr double tb_F1_Lp = (tb_L_p * tb_L_p) / 2.0 - 0.05 * (tb_L_p * tb_L_p * tb_L_p * tb_L_p);
    constexpr double tb_F1_Ln = (tb_L_n * tb_L_n) / 2.0 - 0.05 * (tb_L_n * tb_L_n * tb_L_n * tb_L_n);
    constexpr double tb_C1_p = tb_F1_Lp - tb_max_p * tb_L_p;
    constexpr double tb_C1_n = tb_F1_Ln - tb_max_n * tb_L_n;
    constexpr double tb_F2_Lp = (tb_L_p * tb_L_p * tb_L_p) / 6.0 - 0.01 * (tb_L_p * tb_L_p * tb_L_p * tb_L_p * tb_L_p);
    constexpr double tb_F2_Ln = (tb_L_n * tb_L_n * tb_L_n) / 6.0 - 0.01 * (tb_L_n * tb_L_n * tb_L_n * tb_L_n * tb_L_n);
    constexpr double tb_C2_p = tb_F2_Lp - (tb_max_p * tb_L_p * tb_L_p / 2.0 + tb_C1_p * tb_L_p);
    constexpr double tb_C2_n = tb_F2_Ln - (tb_max_n * tb_L_n * tb_L_n / 2.0 + tb_C1_n * tb_L_n);

    double fx_tube(double x) {
        if (x > tb_L_p) return tb_max_p;
        if (x < tb_L_n) return tb_max_n;
        return x - 0.2 * (x * x * x);
    }
    double F1_tube(double x) {
        if (x > tb_L_p) return tb_max_p * x + tb_C1_p;
        if (x < tb_L_n) return tb_max_n * x + tb_C1_n;
        double x2 = x * x; return (x2 / 2.0) - 0.05 * (x2 * x2);
    }
    double F2_tube(double x) {
        if (x > tb_L_p) return tb_max_p * (x * x) / 2.0 + tb_C1_p * x + tb_C2_p;
        if (x < tb_L_n) return tb_max_n * (x * x) / 2.0 + tb_C1_n * x + tb_C2_n;
        double x2 = x * x; double x3 = x2 * x; return (x3 / 6.0) - 0.01 * (x3 * x2);
    }

    // ==============================================================================
    // Hardknee (SSL): f(x) = x - 0.05*x^3
    // ==============================================================================
    constexpr double hk_L = 2.0;
    constexpr double hk_max = hk_L - 0.05 * (hk_L * hk_L * hk_L);
    constexpr double hk_F1_L = (hk_L * hk_L) / 2.0 - 0.0125 * (hk_L * hk_L * hk_L * hk_L);
    constexpr double hk_C1 = hk_F1_L - hk_max * hk_L;
    constexpr double hk_F2_L = (hk_L * hk_L * hk_L) / 6.0 - 0.0025 * (hk_L * hk_L * hk_L * hk_L * hk_L);
    constexpr double hk_C2 = hk_F2_L - (hk_max * hk_L * hk_L / 2.0 + hk_C1 * hk_L);

    double fx_hardknee(double x) {
        if (x > hk_L) return hk_max;
        if (x < -hk_L) return -hk_max;
        return x - 0.05 * (x * x * x);
    }
    double F1_hardknee(double x) {
        if (x > hk_L) return hk_max * x + hk_C1;
        if (x < -hk_L) return -hk_max * x + hk_C1;
        double x2 = x * x; return (x2 / 2.0) - 0.0125 * (x2 * x2);
    }
    double F2_hardknee(double x) {
        if (x > hk_L) return hk_max * (x * x) / 2.0 + hk_C1 * x + hk_C2;
        if (x < -hk_L) return -hk_max * (x * x) / 2.0 + hk_C1 * x - hk_C2;
        double x2 = x * x; double x3 = x2 * x; return (x3 / 6.0) - 0.0025 * (x3 * x2);
    }

    // ==============================================================================
    // Steel: f(x) = x - 0.1*x^3
    // ==============================================================================
    constexpr double st_L = 1.5;
    constexpr double st_max = st_L - 0.1 * (st_L * st_L * st_L);
    constexpr double st_F1_L = (st_L * st_L) / 2.0 - 0.025 * (st_L * st_L * st_L * st_L);
    constexpr double st_C1 = st_F1_L - st_max * st_L;
    constexpr double st_F2_L = (st_L * st_L * st_L) / 6.0 - 0.005 * (st_L * st_L * st_L * st_L * st_L);
    constexpr double st_C2 = st_F2_L - (st_max * st_L * st_L / 2.0 + st_C1 * st_L);

    double fx_steel(double x) {
        if (x > st_L) return st_max;
        if (x < -st_L) return -st_max;
        return x - 0.1 * (x * x * x);
    }
    double F1_steel(double x) {
        if (x > st_L) return st_max * x + st_C1;
        if (x < -st_L) return -st_max * x + st_C1;
        double x2 = x * x; return (x2 / 2.0) - 0.025 * (x2 * x2);
    }
    double F2_steel(double x) {
        if (x > st_L) return st_max * (x * x) / 2.0 + st_C1 * x + st_C2;
        if (x < -st_L) return -st_max * (x * x) / 2.0 + st_C1 * x - st_C2;
        double x2 = x * x; double x3 = x2 * x; return (x3 / 6.0) - 0.005 * (x3 * x2);
    }

    // ==============================================================================
    // Iron: f(x) = x + 0.05*x^2 - 0.15*x^3
    // ==============================================================================
    constexpr double ir_L_p = 1.5;
    constexpr double ir_L_n = -1.2;
    constexpr double ir_max_p = ir_L_p + 0.05 * (ir_L_p * ir_L_p) - 0.15 * (ir_L_p * ir_L_p * ir_L_p);
    constexpr double ir_max_n = ir_L_n + 0.05 * (ir_L_n * ir_L_n) - 0.15 * (ir_L_n * ir_L_n * ir_L_n);
    constexpr double ir_F1_Lp = (ir_L_p * ir_L_p) / 2.0 + (0.05 / 3.0) * (ir_L_p * ir_L_p * ir_L_p) - (0.15 / 4.0) * (ir_L_p * ir_L_p * ir_L_p * ir_L_p);
    constexpr double ir_F1_Ln = (ir_L_n * ir_L_n) / 2.0 + (0.05 / 3.0) * (ir_L_n * ir_L_n * ir_L_n) - (0.15 / 4.0) * (ir_L_n * ir_L_n * ir_L_n * ir_L_n);
    constexpr double ir_C1_p = ir_F1_Lp - ir_max_p * ir_L_p;
    constexpr double ir_C1_n = ir_F1_Ln - ir_max_n * ir_L_n;
    constexpr double ir_F2_Lp = (ir_L_p * ir_L_p * ir_L_p) / 6.0 + (0.05 / 12.0) * (ir_L_p * ir_L_p * ir_L_p * ir_L_p) - (0.15 / 20.0) * (ir_L_p * ir_L_p * ir_L_p * ir_L_p * ir_L_p);
    constexpr double ir_F2_Ln = (ir_L_n * ir_L_n * ir_L_n) / 6.0 + (0.05 / 12.0) * (ir_L_n * ir_L_n * ir_L_n * ir_L_n) - (0.15 / 20.0) * (ir_L_n * ir_L_n * ir_L_n * ir_L_n * ir_L_n);
    constexpr double ir_C2_p = ir_F2_Lp - (ir_max_p * ir_L_p * ir_L_p / 2.0 + ir_C1_p * ir_L_p);
    constexpr double ir_C2_n = ir_F2_Ln - (ir_max_n * ir_L_n * ir_L_n / 2.0 + ir_C1_n * ir_L_n);

    double fx_iron(double x) {
        if (x > ir_L_p) return ir_max_p;
        if (x < ir_L_n) return ir_max_n;
        return x + 0.05 * (x * x) - 0.15 * (x * x * x);
    }
    double F1_iron(double x) {
        if (x > ir_L_p) return ir_max_p * x + ir_C1_p;
        if (x < ir_L_n) return ir_max_n * x + ir_C1_n;
        double x2 = x * x; return (x2 / 2.0) + (0.05 / 3.0) * (x2 * x) - (0.15 / 4.0) * (x2 * x2);
    }
    double F2_iron(double x) {
        if (x > ir_L_p) return ir_max_p * (x * x) / 2.0 + ir_C1_p * x + ir_C2_p;
        if (x < ir_L_n) return ir_max_n * (x * x) / 2.0 + ir_C1_n * x + ir_C2_n;
        double x2 = x * x; double x3 = x2 * x; return (x3 / 6.0) + (0.05 / 12.0) * (x3 * x) - (0.15 / 20.0) * (x3 * x2);
    }

} // namespace ADAA_Math
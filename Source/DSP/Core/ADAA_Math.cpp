#pragma execution_character_set("utf-8") // ★ MSVC C4828 警告回避のためのBOM代替指定

#include "ADAA_Math.h"
#include <cmath>

namespace ADAA_Math
{
    // ==============================================================================
    // 既存の関数群
    // ==============================================================================
    double fx_cubic(double x)
    {
        if (x > 1.0)  return 2.0 / 3.0;
        if (x < -1.0) return -2.0 / 3.0;
        return x - (x * x * x) / 3.0;
    }

    double F1_cubic(double x)
    {
        if (x > 1.0)
        {
            return (2.0 / 3.0) * x - 0.25;
        }
        else if (x < -1.0)
        {
            return (-2.0 / 3.0) * x - 0.25;
        }
        else
        {
            double x2 = x * x;
            return (x2 / 2.0) - (x2 * x2 / 12.0);
        }
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
    // チェビシェフ多項式ベースの関数群
    // ==============================================================================
    double fx_chebyshev_odd(double x)
    {
        const double limit = 1.29099;
        const double max_val = limit - 0.2 * limit * limit * limit;

        if (x > limit)  return max_val;
        if (x < -limit) return -max_val;
        return x - 0.2 * (x * x * x);
    }

    double F1_chebyshev_odd(double x)
    {
        const double limit = 1.29099;
        const double max_val = limit - 0.2 * limit * limit * limit;
        const double C = -0.42359;

        if (x > limit)
        {
            return max_val * x + C;
        }
        else if (x < -limit)
        {
            return -max_val * x + C;
        }
        else
        {
            double x2 = x * x;
            return (x2 / 2.0) - 0.05 * (x2 * x2);
        }
    }

    double fx_chebyshev_even(double x)
    {
        const double limit_pos = 2.0;
        const double limit_neg = -1.0;

        const double max_pos = limit_pos + 0.15 * (limit_pos * limit_pos) - 0.1 * (limit_pos * limit_pos * limit_pos);
        const double max_neg = limit_neg + 0.15 * (limit_neg * limit_neg) - 0.1 * (limit_neg * limit_neg * limit_neg);

        if (x > limit_pos) return max_pos;
        if (x < limit_neg) return max_neg;
        return x + 0.15 * (x * x) - 0.1 * (x * x * x);
    }

    double F1_chebyshev_even(double x)
    {
        const double limit_pos = 2.0;
        const double limit_neg = -1.0;
        const double max_pos = 1.8;
        const double max_neg = -0.75;
        const double C_pos = -1.6;
        const double C_neg = -0.325;

        if (x > limit_pos)
        {
            return max_pos * x + C_pos;
        }
        else if (x < limit_neg)
        {
            return max_neg * x + C_neg;
        }
        else
        {
            double x2 = x * x;
            return (x2 / 2.0) + 0.05 * (x2 * x) - 0.025 * (x2 * x2);
        }
    }

    // ==============================================================================
    // 入力トランス素材別の非線形関数
    // ==============================================================================
    double fx_steel(double x)
    {
        const double limit = 1.5;
        const double max_val = limit - 0.1 * (limit * limit * limit);
        if (x > limit) return max_val;
        if (x < -limit) return -max_val;
        return x - 0.1 * (x * x * x);
    }

    double F1_steel(double x)
    {
        const double limit = 1.5;
        const double max_val = limit - 0.1 * (limit * limit * limit);
        const double C = -0.384375;
        if (x > limit) return max_val * x + C;
        if (x < -limit) return -max_val * x + C;
        double x2 = x * x;
        return (x2 / 2.0) - 0.025 * (x2 * x2);
    }

    double fx_iron(double x)
    {
        const double limit_pos = 1.5;
        const double limit_neg = -1.2;
        const double max_pos = limit_pos + 0.05 * (limit_pos * limit_pos) - 0.15 * (limit_pos * limit_pos * limit_pos);
        const double max_neg = limit_neg + 0.05 * (limit_neg * limit_neg) - 0.15 * (limit_neg * limit_neg * limit_neg);
        if (x > limit_pos) return max_pos;
        if (x < limit_neg) return max_neg;
        return x + 0.05 * (x * x) - 0.15 * (x * x * x);
    }

    double F1_iron(double x)
    {
        const double limit_pos = 1.5;
        const double limit_neg = -1.2;
        const double max_pos = 1.10625;
        const double max_neg = -0.8688;
        const double C_pos = -0.34453125;
        const double C_neg = -0.25056;
        if (x > limit_pos) return max_pos * x + C_pos;
        if (x < limit_neg) return max_neg * x + C_neg;
        double x2 = x * x;
        return (x2 / 2.0) + (0.05 / 3.0) * (x2 * x) - (0.15 / 4.0) * (x2 * x2);
    }

    // ==============================================================================
    // ★ 新規追加: プリアンプモデル固有の非線形関数
    // ==============================================================================
    double fx_tube(double x)
    {
        const double limit_pos = 1.5;
        const double limit_neg = -1.0;
        const double max_pos = limit_pos - 0.2 * (limit_pos * limit_pos * limit_pos); // 0.825
        const double max_neg = limit_neg - 0.2 * (limit_neg * limit_neg * limit_neg); // -0.8

        if (x > limit_pos) return max_pos;
        if (x < limit_neg) return max_neg;
        return x - 0.2 * (x * x * x);
    }

    double F1_tube(double x)
    {
        const double limit_pos = 1.5;
        const double limit_neg = -1.0;
        const double max_pos = 0.825;
        const double max_neg = -0.8;
        const double C_pos = -0.365625;
        const double C_neg = -0.35;

        if (x > limit_pos) return max_pos * x + C_pos;
        if (x < limit_neg) return max_neg * x + C_neg;
        double x2 = x * x;
        return (x2 / 2.0) - 0.05 * (x2 * x2);
    }

    double fx_hardknee(double x)
    {
        const double limit = 2.0;
        const double max_val = limit - 0.05 * (limit * limit * limit); // 1.6
        if (x > limit) return max_val;
        if (x < -limit) return -max_val;
        return x - 0.05 * (x * x * x);
    }

    double F1_hardknee(double x)
    {
        const double limit = 2.0;
        const double max_val = 1.6;
        const double C = -1.4;

        if (x > limit) return max_val * x + C;
        if (x < -limit) return -max_val * x + C;
        double x2 = x * x;
        return (x2 / 2.0) - 0.0125 * (x2 * x2);
    }

} // namespace ADAA_Math
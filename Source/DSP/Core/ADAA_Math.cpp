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
    // ★ 新規追加: チェビシェフ多項式ベースの関数群
    // ==============================================================================

    // ------------------------------------------------------------------------------
    // 3次倍音（Punch/Odd）ジェネレータ
    // 基本的なソフトクリップ特性に T3(x) = 4x^3 - 3x の要素をブレンドし、
    // エッジの効いた奇数次倍音を生成します。
    // f_odd(x) = x - 0.2 * x^3 (※クリッピング領域を滑らかに繋ぐためのスケーリング係数)
    // ------------------------------------------------------------------------------
    double fx_chebyshev_odd(double x)
    {
        // 飽和点 x = 1.29099 で微係数がゼロになるよう設計
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
        // x=limit での積分値 C を合わせる
        // F(limit) = limit^2 / 2 - 0.05 * limit^4 = 0.69444
        // max_val * limit + C = 0.69444 -> C = 0.69444 - 1.11803 = -0.42359
        const double C = -0.42359;

        if (x > limit)
        {
            return max_val * x + C;
        }
        else if (x < -limit)
        {
            return -max_val * x + C; // 偶関数的な積分補正
        }
        else
        {
            double x2 = x * x;
            return (x2 / 2.0) - 0.05 * (x2 * x2);
        }
    }

    // ------------------------------------------------------------------------------
    // 2次倍音（Smooth/Even）ジェネレータ
    // 基本の線形項 x に T2(x) = 2x^2 - 1 の要素（非対称性）を付加します。
    // f_even(x) = x + 0.15 * x^2 - 0.1 * x^3 (発散防止のための3次項)
    // ------------------------------------------------------------------------------
    double fx_chebyshev_even(double x)
    {
        const double limit_pos = 2.0; // 正側の飽和点
        const double limit_neg = -1.0; // 負側の飽和点 (非対称)

        const double max_pos = limit_pos + 0.15 * (limit_pos * limit_pos) - 0.1 * (limit_pos * limit_pos * limit_pos); // = 1.8
        const double max_neg = limit_neg + 0.15 * (limit_neg * limit_neg) - 0.1 * (limit_neg * limit_neg * limit_neg); // = -0.75

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

        // F(x) = x^2/2 + 0.05*x^3 - 0.025*x^4
        // F(2.0) = 2.0 + 0.4 - 0.4 = 2.0
        // max_pos * 2.0 + C_pos = 2.0 -> C_pos = -1.6
        const double C_pos = -1.6;

        // F(-1.0) = 0.5 - 0.05 - 0.025 = 0.425
        // max_neg * (-1.0) + C_neg = 0.425 -> C_neg = -0.325
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

} // namespace ADAA_Math
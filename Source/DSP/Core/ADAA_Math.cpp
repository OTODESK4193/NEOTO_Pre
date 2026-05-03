#include "ADAA_Math.h"
#include <cmath>

namespace ADAA_Math
{
    // APIスタイルの3次関数ソフトクリッパー (区分定義によるハードクリップ統合版)
    // f(x) = 
    //   x >  1.0 :  2/3
    //   |x| <= 1.0 :  x - (x^3 / 3)
    //   x < -1.0 : -2/3
    double fx_cubic(double x)
    {
        if (x > 1.0)  return 2.0 / 3.0;
        if (x < -1.0) return -2.0 / 3.0;
        return x - (x * x * x) / 3.0;
    }

    // 上記区分定義関数の第1次積分 (Antiderivative)
    // 境界点 x = 1.0, -1.0 で連続性を保つための積分定数を算出して結合
    double F1_cubic(double x)
    {
        if (x > 1.0)
        {
            // 区間 (1.0, inf) の積分: 2/3 * x + C
            // x=1.0 で (1^2/2 - 1^4/12) = 5/12 と一致させるため C = -1/4
            return (2.0 / 3.0) * x - 0.25;
        }
        else if (x < -1.0)
        {
            // 区間 (-inf, -1.0) の積分: -2/3 * x + C
            // x=-1.0 で ((-1)^2/2 - (-1)^4/12) = 5/12 と一致させるため C = -1/4
            return (-2.0 / 3.0) * x - 0.25;
        }
        else
        {
            // 区間 [-1.0, 1.0] の積分
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

} // namespace ADAA_Math
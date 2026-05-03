#pragma once

namespace ADAA_Math
{
    // APIスタイルの3次関数ソフトクリッパー (無次元)
    // f(x) = x - (x^3 / 3)
    double fx_cubic(double x);

    // 上記関数の第1次積分 (Antiderivative)
    // F1(x) = (x^2 / 2) - (x^4 / 12)
    double F1_cubic(double x);

    // ニッケルトランス等で使用する、ハードクリップを伴う非線形関数
    double fx_hardclip(double x);

    // 上記ハードクリップ関数の第1次積分
    double F1_hardclip(double x);

} // namespace ADAA_Math
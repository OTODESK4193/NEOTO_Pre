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

    // ==============================================================================
    // ★ 新規追加: チェビシェフ多項式ベースのハイブリッド・サチュレーション
    // ==============================================================================

    // 2次倍音（Smooth/Even）ジェネレータ: T2ベースの非線形関数
    double fx_chebyshev_even(double x);
    // 上記関数の第1次積分
    double F1_chebyshev_even(double x);

    // 3次倍音（Punch/Odd）ジェネレータ: T3ベースの非線形関数
    double fx_chebyshev_odd(double x);
    // 上記関数の第1次積分
    double F1_chebyshev_odd(double x);

    // ==============================================================================
    // ★ 新規追加: 入力トランス素材別の非線形関数
    // ==============================================================================

    // Steel用: 緩やかな対称ソフトクリップ (糊と温かみ)
    double fx_steel(double x);
    double F1_steel(double x);

    // Iron用: やや非対称でパンチのあるサチュレーション
    double fx_iron(double x);
    double F1_iron(double x);

} // namespace ADAA_Math
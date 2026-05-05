#pragma once

namespace ADAA_Math
{
    // ==============================================================================
    // 既存の基本関数
    // ==============================================================================
    double fx_cubic(double x);
    double F1_cubic(double x);

    double fx_hardclip(double x);
    double F1_hardclip(double x);

    // ==============================================================================
    // プリアンプ用関数 (ADAA1 & ADAA2)
    // ==============================================================================

    // 2次倍音主体 (Even)
    double fx_chebyshev_even(double x);
    double F1_chebyshev_even(double x);
    double F2_chebyshev_even(double x);

    // 3次倍音主体 (Odd)
    double fx_chebyshev_odd(double x);
    double F1_chebyshev_odd(double x);
    double F2_chebyshev_odd(double x);

    // Tube (V76s)
    double fx_tube(double x);
    double F1_tube(double x);
    double F2_tube(double x);

    // Hardknee (SSL)
    double fx_hardknee(double x);
    double F1_hardknee(double x);
    double F2_hardknee(double x);

    // ==============================================================================
    // トランスフォーマー用関数 (Steel / Iron)
    // ==============================================================================
    double fx_steel(double x);
    double F1_steel(double x);
    double F2_steel(double x);

    double fx_iron(double x);
    double F1_iron(double x);
    double F2_iron(double x);

} // namespace ADAA_Math
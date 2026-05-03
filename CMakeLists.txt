cmake_minimum_required(VERSION 3.22)
project(NEOTO_Pre VERSION 1.0.0 LANGUAGES C CXX)

# VST3の互換エラー回避と安全対策
set(JUCE_VST3_AUTO_MANIFEST OFF)
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)
add_compile_definitions(JUCE_USE_WINDOWS_MEDIA_FORMAT=1)
# [重要] グローバルAppConfigの無効化（依存関係の分離・クラッシュ対策）
add_compile_definitions(JUCE_USE_APP_CONFIG_HEADER=0)

# 1. C++20の有効化 (モダンDSPとテンプレートメタプログラミングに必須)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. JUCEディレクトリの指定 (環境に合わせて設定)
set(JUCE_DIR "C:/JUCE")
add_subdirectory(${JUCE_DIR} build_juce)

# 3. プラグインの定義
juce_add_plugin(NEOTO_Pre
    COMPANY_NAME "OTODESK"
    IS_SYNTH FALSE                                  
    NEEDS_MIDI_INPUT FALSE                          
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE 
    VST3_AUTO_MANIFEST FALSE
    PLUGIN_MANUFACTURER_CODE "Otdk"
    PLUGIN_CODE "NeoP"                              
    FORMATS VST3 Standalone
    PRODUCT_NAME "NEOTO_Pre"
    CHANNEL_CONFIGS 2,2                             
)

# 4. コンパイルオプション（SIMD最適化と文字コード設定）
target_compile_options(NEOTO_Pre PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Release>:/O2 /fp:fast> /arch:AVX2 /wd4819 /utf-8
    >
)

# 5. ソースコードの追加（役割ごとの完全分離）
target_sources(NEOTO_Pre PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h

    # DSP Core (安全な先読み・オーバーサンプリング基盤)
    Source/DSP/Core/AutoLevel.cpp
    Source/DSP/Core/AutoLevel.h
    Source/DSP/Core/Oversampler.cpp
    Source/DSP/Core/Oversampler.h
    

    # DSP Transformers (トランスモデリング)
    Source/DSP/Transformers/SteelTransformer.cpp
    Source/DSP/Transformers/SteelTransformer.h
    Source/DSP/Transformers/NickelTransformer.cpp
    Source/DSP/Transformers/NickelTransformer.h

    # DSP Algorithms (サチュレーション・アルゴリズム)
    Source/DSP/Algorithms/NeveStyleDrive.cpp
    Source/DSP/Algorithms/NeveStyleDrive.h
    Source/DSP/Algorithms/ApiStyleDrive.cpp
    Source/DSP/Algorithms/ApiStyleDrive.h

    # GUI (リアルタイム描画とUIコンポーネント)
    
    
    
    
    Source/GUI/ArcDial.cpp
    Source/GUI/ArcDial.h
 "Source/UI/Sections/InputSection.cpp" "Source/UI/Sections/InputSection.h" "Source/DSP/Core/ADAA_Math.h" "Source/DSP/Core/ADAA_Math.cpp" "Source/UI/Sections/DriveSection.h" "Source/UI/Sections/DriveSection.cpp" "Source/UI/Sections/TransformerSection.h" "Source/UI/Sections/TransformerSection.cpp" "Source/UI/Sections/OutputSection.h" "Source/UI/Sections/OutputSection.cpp" "Source/UI/Sections/AnalyzeSection.h" "Source/UI/Sections/AnalyzeSection.cpp" "Source/DSP/Core/EngineInterfaces.h" "Source/DSP/Algorithms/PreampModels.h" "Source/DSP/Algorithms/PreampModels.cpp" "Source/DSP/Transformers/TransformerModels.h" "Source/DSP/Transformers/TransformerModels.cpp")

# 6. インクルードパス
target_include_directories(NEOTO_Pre PRIVATE
    Source
    Source/DSP/Core
    Source/DSP/Transformers
    Source/DSP/Algorithms
    Source/GUI
    # IntelliSenseのエラーを回避
    ${CMAKE_CURRENT_BINARY_DIR}/NEOTO_Pre_artefacts/JuceLibraryCode
)

# 7. JUCEモジュールのリンク
target_link_libraries(NEOTO_Pre PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_core         
    juce::juce_events
    juce::juce_opengl   # 計画書のOpenGLビジュアライザ要件に対応       
)

# 8. JUCE 8 固有の設定
juce_generate_juce_header(NEOTO_Pre)

if(WIN32)  
    target_compile_definitions(NEOTO_Pre PRIVATE JUCE_DIRECT2D=1)
endif()
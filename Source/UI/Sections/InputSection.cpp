#include "InputSection.h"
#include "BinaryData.h"

InputSection::InputSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(inputGainSlider, inputGainLabel, "Input Gain");
    inputGainSlider.setLookAndFeel(&arcLnF);

    osModeCombo.addItemList({ "Off (1x)", "2x", "4x", "8x" }, 1);
    addAndMakeVisible(osModeCombo);

    osModeLabel.setText("Oversampling", juce::dontSendNotification);
    osModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osModeLabel);

    inTransCombo.addItemList({ "None", "Nickel", "Steel", "Iron", "Amorphous", "Carnhill", "Cinemag" }, 1);
    addAndMakeVisible(inTransCombo);

    inTransLabel.setText("Input Transformer", juce::dontSendNotification);
    inTransLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inTransLabel);

    inputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "input_gain", inputGainSlider);
    osModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "os_mode", osModeCombo);
    inTransAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "in_trans_type", inTransCombo);

    // ------------------------------------------------------------------
    // ★ クロップ済みロゴに合わせたロード処理
    // ------------------------------------------------------------------
    auto rawLogoImg = juce::ImageCache::getFromMemory(BinaryData::LOGO_png, BinaryData::LOGO_pngSize);
    if (rawLogoImg.isValid())
    {
        // すでにユーザー側でトリミング済みのため、コード側では意匠を保護する程度の最小限のカットを行います
        const float cropMarginLeft = 0.05f;
        const float cropMarginRight = 0.05f;
        const float cropMarginTop = 0.00f;
        const float cropMarginBottom = 0.00f; // 下端はそのまま使用

        int srcX = static_cast<int>(static_cast<double>(rawLogoImg.getWidth()) * cropMarginLeft);
        int srcY = static_cast<int>(static_cast<double>(rawLogoImg.getHeight()) * cropMarginTop);
        int srcW = static_cast<int>(static_cast<double>(rawLogoImg.getWidth()) * (1.0 - cropMarginLeft - cropMarginRight));
        int srcH = static_cast<int>(static_cast<double>(rawLogoImg.getHeight()) * (1.0 - cropMarginTop - cropMarginBottom));

        juce::Rectangle<int> cropArea(srcX, srcY, srcW, srcH);
        cropArea = cropArea.getIntersection(rawLogoImg.getBounds());

        if (!cropArea.isEmpty())
        {
            juce::Image croppedLogoImg = rawLogoImg.getClippedImage(cropArea);
            // 領域内で中央に配置
            logoComponent.setImage(croppedLogoImg, juce::RectanglePlacement::centred);
            addAndMakeVisible(logoComponent);
        }
    }

    else
    {
        juce::Logger::writeToLog("Warning: InputSection Logo Image is invalid or not found.");
    }

    auto updateColors = [this] {
        int id = inTransCombo.getSelectedItemIndex();
        juce::Colour c = juce::Colours::cyan;
        if (id == 0)      c = juce::Colour(0xff444444);      // None → チャコール
        else if (id == 1) c = juce::Colour(0xff0088aa);      // Nickel → ティール
        else if (id == 2) c = juce::Colour(0xffaa2222);      // Steel → 濃赤
        else if (id == 3) c = juce::Colour(0xffcc5500);      // Iron → 濃オレンジ
        else if (id == 4) c = juce::Colour(0xff7722cc);      // Amorphous → 深紫
        else if (id == 5) c = juce::Colour(0xffbbbbbb);      // Carnhill → シルバー
        else if (id == 6) c = juce::Colour(0xff0055dd);      // Cinemag → 濃青
        inputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        repaint();
        };

    inTransCombo.onChange = updateColors;
    updateColors();
}

InputSection::~InputSection()
{
    inputGainSlider.setLookAndFeel(nullptr);
}

void InputSection::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void InputSection::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(juce::FontOptions(15.0f));
    g.drawText("INPUT STAGE", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void InputSection::resized()
{
    // 全体マージンを5pxに戻し、枠線からの距離を確保
    auto area = getLocalBounds().reduced(5, 5);

    // 1. タイトルラベル (20px)
    area.removeFromTop(20);

    // 共通の余白設定（窮屈さを解消するための重要な値）
    const int itemGap = 2;

    // 2. Gainノブ領域 (100px)
    // ノブを少し小さく(70px)して、周囲の空気感を作ります
    auto topRow = area.removeFromTop(100);
    inputGainLabel.setBounds(topRow.removeFromTop(20));
    inputGainSlider.setBounds(topRow.withSizeKeepingCentre(70, 70));

    // 余白を挿入
    area.removeFromTop(itemGap);

    // 3. Oversampling領域 (42px)
    auto osRow = area.removeFromTop(42);
    osModeLabel.setBounds(osRow.removeFromTop(18).withSizeKeepingCentre(120, 18));
    osModeCombo.setBounds(osRow.removeFromTop(24).withSizeKeepingCentre(110, 24));

    // 余白を挿入
    area.removeFromTop(itemGap);

    // 4. Transformer領域 (42px)
    auto transRow = area.removeFromTop(42);
    inTransLabel.setBounds(transRow.removeFromTop(18).withSizeKeepingCentre(140, 18));
    inTransCombo.setBounds(transRow.removeFromTop(24).withSizeKeepingCentre(110, 24));

    // 5. ロゴ領域 (残りの下部すべて)
    if (logoComponent.isVisible())
    {
        // 最後の項目との間に少し余裕を持たせる
        area.removeFromTop(itemGap);
        // reduced(10, 5) でロゴを少し小ぶりにし、上品に配置します
        logoComponent.setBounds(area.reduced(5, 2));
    }
}
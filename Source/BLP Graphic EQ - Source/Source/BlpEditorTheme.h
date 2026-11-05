#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace blp
{
inline const juce::Colour canvas { 0xff151518 };
inline const juce::Colour header { 0xff1d1d21 };
inline const juce::Colour edge   { 0xff34343b };
inline const juce::Colour ink    { 0xfff6f5f7 };
inline const juce::Colour muted  { 0xffb9b8c0 };
inline const juce::Colour accent { 0xffee1c25 };
inline const juce::Colour headerAccent { 0xffffff00 };
inline const juce::Colour textWhite   { 0xffffffff };

class EditorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EditorLookAndFeel()
    {
        setColour(juce::Label::textColourId, textWhite);
        setColour(juce::Label::textWhenEditingColourId, textWhite);
        setColour(juce::TextButton::textColourOffId, textWhite);
        setColour(juce::TextButton::textColourOnId, textWhite);
        setColour(juce::ToggleButton::textColourId, textWhite);
        setColour(juce::ComboBox::textColourId, textWhite);
        setColour(juce::PopupMenu::textColourId, textWhite);
        setColour(juce::PopupMenu::highlightedTextColourId, textWhite);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour(juce::Slider::textBoxTextColourId, textWhite);
        setColour(juce::TextEditor::textColourId, textWhite);
        setColour(juce::TextEditor::highlightedTextColourId, textWhite);
        setColour(juce::TooltipWindow::textColourId, textWhite);
        setColour(juce::TooltipWindow::backgroundColourId, header);
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
        if (static_cast<bool>(slider.getProperties()["valueOnHover"]))
            label->setColour(juce::Label::textColourId, juce::Colours::transparentBlack);
        return label;
    }
};

inline void showValueOnHover(juce::Slider& slider, juce::Component& parent)
{
    slider.getProperties().set("valueOnHover", true);
    slider.setPopupDisplayEnabled(false, true, &parent, -1);
    slider.sendLookAndFeelChange();
}

inline void paintAdjustArrows(juce::Graphics& g, juce::Rectangle<float> pill)
{
    const juce::Graphics::ScopedSaveState state(g);
    const auto x = pill.getRight() - 12.0f;
    const auto y = pill.getCentreY();
    g.setColour(textWhite);
    g.drawLine(x - 3.0f, y - 2.0f, x, y - 5.0f, 1.0f);
    g.drawLine(x, y - 5.0f, x + 3.0f, y - 2.0f, 1.0f);
    g.drawLine(x - 3.0f, y + 2.0f, x, y + 5.0f, 1.0f);
    g.drawLine(x, y + 5.0f, x + 3.0f, y + 2.0f, 1.0f);
}

class ThemedEditor : public juce::AudioProcessorEditor
{
public:
    void setVisuallyBypassed(bool bypassed)
    {
        if (visuallyBypassed == bypassed)
            return;
        visuallyBypassed = bypassed;
        repaint();
    }

protected:
    void paintOverChildren(juce::Graphics& g) override
    {
        if (visuallyBypassed)
        {
            g.setColour(canvas.withAlpha(0.55f));
            g.fillRect(getLocalBounds().withTrimmedTop(34));
        }
    }

    explicit ThemedEditor(juce::AudioProcessor& processor) : ThemedEditor(&processor) {}

    explicit ThemedEditor(juce::AudioProcessor* processor) : juce::AudioProcessorEditor(processor)
    {
        setLookAndFeel(&lookAndFeel);
    }

    ~ThemedEditor() override
    {
        setLookAndFeel(nullptr);
    }

private:
    EditorLookAndFeel lookAndFeel;
    bool visuallyBypassed = false;
};

template <typename... Args>
void drawText(juce::Graphics& g, const Args&... args)
{
    const juce::Graphics::ScopedSaveState state(g);
    g.setColour(textWhite);
    g.drawText(args...);
}

template <typename... Args>
void drawFittedText(juce::Graphics& g, const Args&... args)
{
    const juce::Graphics::ScopedSaveState state(g);
    g.setColour(textWhite);
    g.drawFittedText(args...);
}


inline int menuValueStart(const juce::String& caption)
{
    return juce::jmax(58, 26 + 6 * caption.length());
}

inline void paintMenuField(juce::Graphics& g, int width, int height, const juce::String& caption)
{
    g.setColour(juce::Colour(0xff323238));
    g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 8.0f);
    g.setFont(juce::FontOptions(12.0f));
    drawText(g, caption, 13, 0, menuValueStart(caption) - 18, height, juce::Justification::centredLeft);

    const auto x = static_cast<float>(width) - 16.0f;
    const auto y = static_cast<float>(height) * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath(x - 4.0f, y - 2.0f);
    chevron.lineTo(x, y + 2.0f);
    chevron.lineTo(x + 4.0f, y - 2.0f);
    g.setColour(muted);
    g.strokePath(chevron, juce::PathStrokeType(1.5f));
}

class MenuLookAndFeel : public EditorLookAndFeel
{
public:
    MenuLookAndFeel()
    {
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff323238));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff47474f));
    }

    void getIdealPopupMenuItemSizeWithOptions(const juce::String& text, bool isSeparator,
                                              int standardHeight, int& width, int& height,
                                              const juce::PopupMenu::Options& options) override
    {
        juce::LookAndFeel_V4::getIdealPopupMenuItemSizeWithOptions(
            text, isSeparator, standardHeight, width, height, options);
        if (!isSeparator)
            height = 42;
    }

    void drawPopupMenuItemWithOptions(juce::Graphics& g, const juce::Rectangle<int>& area,
                                      bool highlighted, const juce::PopupMenu::Item& item,
                                      const juce::PopupMenu::Options& options) override
    {
        if (item.isSeparator)
        {
            juce::LookAndFeel_V4::drawPopupMenuItemWithOptions(g, area, highlighted, item, options);
            return;
        }

        if (highlighted)
        {
            g.setColour(juce::Colour(0xff47474f));
            g.fillRect(area);
        }
        if (item.isTicked)
        {
            g.setColour(accent);
            g.fillEllipse(static_cast<float>(area.getX() + 7), area.getCentreY() - 2.0f, 4.0f, 4.0f);
        }
        g.setFont(juce::FontOptions(14.0f));
        drawText(g, item.text, area.reduced(15, 2), juce::Justification::centredLeft);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox& box) override
    {
        paintMenuField(g, width, height, box.getTitle());
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        const auto left = menuValueStart(box.getTitle());
        label.setBounds(left, 0, box.getWidth() - left - 25, box.getHeight());
        label.setFont(juce::FontOptions(14.0f));
    }
};

class MenuBox final : public juce::ComboBox
{
public:
    explicit MenuBox(const juce::String& caption)
    {
        setTitle(caption);
        setLookAndFeel(&style);
    }

    ~MenuBox() override { setLookAndFeel(nullptr); }

private:
    MenuLookAndFeel style;
};

inline juce::Rectangle<int> bypassBounds(int width)
{
    return { width - 40, 5, 30, 24 };
}

inline void paintHeader(juce::Graphics& g, int width, const juce::String& title)
{
    g.setColour(header);
    g.fillRect(0, 0, width, 34);
    g.setColour(edge);
    g.fillRect(0, 33, width, 1);
    g.setColour(headerAccent);
    g.fillRoundedRectangle(10.0f, 10.0f, 3.0f, 14.0f, 1.5f);
    g.setColour(textWhite);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawFittedText(title, { 19, 0, width - 65, 33 }, juce::Justification::centredLeft, 1);
}

class BypassButton final : public juce::TextButton
{
public:
    BypassButton() : juce::TextButton("Power")
    {
        setClickingTogglesState(true);
        setTitle("Effect power");
        setTooltip("Toggle effect power");
    }

private:
    void updateEditor()
    {
        if (auto* editor = findParentComponentOfClass<ThemedEditor>())
            editor->setVisuallyBypassed(getToggleState());
    }

    void parentHierarchyChanged() override { updateEditor(); }
    void buttonStateChanged() override { updateEditor(); }

    void paintButton(juce::Graphics& g, bool hovered, bool pressed) override
    {
        const bool poweredOn = !getToggleState();
        const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const auto surface = juce::Colour(0xff28282d);
        g.setColour(surface.brighter(pressed ? 0.22f : hovered ? 0.1f : 0.0f));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(hasKeyboardFocus(false) ? textWhite : edge);
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

        const auto centre = bounds.getCentre();
        g.setColour(poweredOn ? headerAccent : muted);
        juce::Path power;
        power.addCentredArc(centre.x, centre.y, 5.0f, 5.0f, 0.0f, 0.65f, 5.63f, true);
        g.strokePath(power, juce::PathStrokeType(1.35f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        g.drawLine(centre.x, centre.y - 6.5f, centre.x, centre.y - 1.5f, 1.35f);
    }
};
}

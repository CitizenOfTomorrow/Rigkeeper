#pragma once

#include <JuceHeader.h>
#include "RigFile.h"

// Parameters of one effect slot, shown in a callout when a tile is clicked.
class EffectDetails : public Component
{
public:
    EffectDetails(const rigfile::Rig& rig, std::size_t slot)
        : title(String(rigfile::slotLabels[slot]) + "  " + String(rigfile::effectName(rig.slotType(slot)))
                + (rig.slotOn(slot) ? "" : "  (off)")),
          colour(categoryColour(rigfile::effectCategory(rig.slotType(slot))))
    {
        for (const auto& p : rigfile::effectParameters(rig.slotType(slot)))
        {
            const auto raw = rig.value(rigfile::slotPages[slot], p.number);
            if (raw >= 0) rows.push_back({ p.name, rigfile::formatValue(p.scale, raw), rigfile::barPosition(p.scale, raw),
                                           p.scale == rigfile::Scale::bipolar || p.scale == rigfile::Scale::semitones });
        }
        const auto lines = (static_cast<int>(rows.size()) + columns - 1) / columns;
        setSize(columns * columnWidth + (columns - 1) * columnGap + 2 * margin,
                titleHeight + std::max(lines, 1) * rowHeight + 2 * margin);
    }

    void paint(Graphics& g) override
    {
        auto area = getLocalBounds().reduced(margin);
        g.setColour(colour);
        g.setFont(FontOptions(14.0f).withStyle("Bold"));
        g.drawText(title, area.removeFromTop(titleHeight), Justification::topLeft);
        if (rows.empty())
        {
            g.setColour(Colours::white.withAlpha(0.6f));
            g.setFont(FontOptions(12.0f));
            g.drawText("No parameters known for this effect", area, Justification::topLeft);
            return;
        }
        const auto lines = (static_cast<int>(rows.size()) + columns - 1) / columns;
        for (int i = 0; i < static_cast<int>(rows.size()); ++i)
        {
            const auto& row = rows[static_cast<std::size_t>(i)];
            auto cell = Rectangle<int>(area.getX() + (i / lines) * (columnWidth + columnGap),
                                       area.getY() + (i % lines) * rowHeight, columnWidth, rowHeight);
            g.setColour(Colours::white.withAlpha(0.75f));
            g.setFont(FontOptions(12.0f));
            g.drawText(row.name, cell.removeFromLeft(108), Justification::centredLeft);
            g.setColour(Colours::white);
            g.drawText(row.value, cell.removeFromRight(56), Justification::centredRight);
            if (row.bar >= 0.0)
            {
                const auto track = cell.reduced(6, 0).withSizeKeepingCentre(cell.getWidth() - 12, 4).toFloat();
                g.setColour(Colours::white.withAlpha(0.12f));
                g.fillRoundedRectangle(track, 2.0f);
                const auto from = row.centred ? 0.5 : 0.0;
                const auto to = jlimit(0.0, 1.0, row.bar);
                const auto x1 = track.getX() + track.getWidth() * static_cast<float>(jmin(from, to));
                const auto x2 = track.getX() + track.getWidth() * static_cast<float>(jmax(from, to));
                g.setColour(colour);
                g.fillRoundedRectangle(Rectangle<float>(x1, track.getY(), jmax(2.0f, x2 - x1), track.getHeight()), 2.0f);
            }
        }
    }

    static Colour categoryColour(rigfile::Category c)
    {
        using rigfile::Category;
        switch (c)
        {
            case Category::wah:        return Colour(0xffe8862a);
            case Category::distortion: return Colour(0xffd9443a);
            case Category::dynamics:   return Colour(0xff2fb5c9);
            case Category::modulation: return Colour(0xff3d7fe0);
            case Category::equalizer:  return Colour(0xffe0c23a);
            case Category::booster:    return Colour(0xffd65a8a);
            case Category::loop:       return Colour(0xffb0b0b0);
            case Category::pitch:      return Colour(0xff9a5be0);
            case Category::delay:      return Colour(0xff4cbf5a);
            case Category::reverb:     return Colour(0xff2f9e7e);
            default:                   return Colour(0xff7a7a7a);
        }
    }

private:
    struct Row { String name, value; double bar; bool centred; };
    static constexpr int columns = 2, columnWidth = 230, columnGap = 20, rowHeight = 22,
                         titleHeight = 26, margin = 12;
    String title;
    Colour colour;
    std::vector<Row> rows;
};

// Shows the contents of a .krig file in Kemper signal-chain order:
// stomps A-D, stack (amp, EQ, cabinet), effects X/MOD/DLY/REV.
// Clicking an effect tile shows its parameters.
class RigView : public Component
{
public:
    void setRig(const rigfile::Rig* newRig)
    {
        hasRigData = newRig != nullptr;
        if (hasRigData) rig = *newRig;
        repaint();
    }

    bool hasRig() const { return hasRigData; }

    void paint(Graphics& g) override
    {
        if (!hasRigData) return;
        auto area = getLocalBounds();

        auto header = area.removeFromTop(46);
        g.setColour(Colours::white);
        g.setFont(FontOptions(19.0f).withStyle("Bold"));
        g.drawFittedText(String(rig.name()), header.removeFromTop(26), Justification::centredLeft, 1);
        StringArray details;
        for (const auto& s : { rig.author(), rig.date(), rig.firmware().empty() ? std::string() : "FW " + rig.firmware() })
            if (!s.empty()) details.add(String(s));
        g.setColour(Colours::white.withAlpha(0.6f));
        g.setFont(FontOptions(13.0f));
        g.drawFittedText(details.joinIntoString(String(CharPointer_UTF8("  \xc2\xb7  "))), header, Justification::centredLeft, 1);

        area.removeFromTop(gap);
        drawSlotRow(g, area.removeFromTop(rowHeight), "STOMPS", 0);
        area.removeFromTop(gap);
        drawStackRow(g, area.removeFromTop(rowHeight + 10));
        area.removeFromTop(gap);
        drawSlotRow(g, area.removeFromTop(rowHeight), "EFFECTS", 4);
        area.removeFromTop(gap);

        String footer = "Rig volume " + signedValue(rigfile::bipolar(rig.value(rigfile::rigPage, 1, 8192)));
        if (const auto tempo = rig.value(rigfile::rigPage, 0); tempo >= 0)
            footer << "   Tempo " << String(tempo / 64.0, tempo % 64 == 0 ? 0 : 1) << " bpm"
                   << (rig.value(rigfile::rigPage, 2, 0) != 0 ? "" : " (off)");
        g.setColour(Colours::white.withAlpha(0.6f));
        g.setFont(FontOptions(12.0f));
        g.drawFittedText(footer, area.removeFromTop(18).withTrimmedLeft(labelWidth), Justification::centredLeft, 1);
    }

    static int preferredHeight() { return 46 + 4 * gap + 3 * rowHeight + 10 + 18; }

    void mouseMove(const MouseEvent& e) override
    {
        setMouseCursor(slotAt(e.getPosition()) >= 0 ? MouseCursor::PointingHandCursor
                                                    : MouseCursor::NormalCursor);
    }

    void mouseUp(const MouseEvent& e) override
    {
        const auto slot = slotAt(e.getPosition());
        if (slot < 0 || e.mouseWasDraggedSinceMouseDown()) return;
        auto* parent = getTopLevelComponent();  // the plugin editor
        CallOutBox::launchAsynchronously(std::make_unique<EffectDetails>(rig, static_cast<std::size_t>(slot)),
                                         parent->getLocalArea(this, slotBounds(static_cast<std::size_t>(slot))),
                                         parent);
    }

private:
    static constexpr int gap = 10, rowHeight = 54, labelWidth = 74, tileGap = 6;

    static Colour categoryColour(rigfile::Category c) { return EffectDetails::categoryColour(c); }

    Rectangle<int> slotBounds(std::size_t slot) const
    {
        const auto width = (getWidth() - labelWidth - 3 * tileGap) / 4;
        const auto stompsY = 46 + gap;
        const auto effectsY = stompsY + rowHeight + gap + rowHeight + 10 + gap;
        const auto column = static_cast<int>(slot % 4);
        return { labelWidth + column * (width + tileGap), slot < 4 ? stompsY : effectsY, width, rowHeight };
    }

    // Index of the non-empty slot tile at a position, or -1.
    int slotAt(Point<int> position) const
    {
        if (!hasRigData) return -1;
        for (std::size_t i = 0; i < rigfile::slotPages.size(); ++i)
            if (slotBounds(i).contains(position) && rig.slotType(i) > 0)
                return static_cast<int>(i);
        return -1;
    }

    static String signedValue(double v)
    {
        return (v > 0.05 ? "+" : "") + String(v, 1);
    }

    static void drawRowLabel(Graphics& g, Rectangle<int>& row, const String& label)
    {
        g.setColour(Colours::white.withAlpha(0.45f));
        g.setFont(FontOptions(11.0f).withStyle("Bold"));
        g.drawText(label, row.removeFromLeft(labelWidth), Justification::centredLeft);
    }

    void drawSlotRow(Graphics& g, Rectangle<int> row, const String& label, std::size_t first) const
    {
        drawRowLabel(g, row, label);
        for (std::size_t i = first; i < first + 4; ++i)
        {
            const auto tile = slotBounds(i).toFloat();
            const auto type = rig.slotType(i);
            const bool empty = type <= 0;
            const bool on = !empty && rig.slotOn(i);
            const auto colour = categoryColour(rigfile::effectCategory(type));

            g.setColour(on ? colour : colour.withAlpha(0.12f));
            g.fillRoundedRectangle(tile, 6.0f);
            g.setColour(empty ? Colours::white.withAlpha(0.15f) : colour.withAlpha(on ? 1.0f : 0.6f));
            g.drawRoundedRectangle(tile.reduced(0.5f), 6.0f, 1.0f);

            const auto text = on ? Colour(0xff141414) : Colours::white.withAlpha(empty ? 0.3f : 0.6f);
            auto inner = tile.toNearestInt().reduced(7, 4);
            g.setColour(text.withMultipliedAlpha(0.7f));
            g.setFont(FontOptions(10.0f).withStyle("Bold"));
            g.drawText(String(rigfile::slotLabels[i]) + (on || empty ? "" : "  OFF"),
                       inner.removeFromTop(13), Justification::centredLeft);
            g.setColour(text);
            g.setFont(FontOptions(13.0f).withStyle("Bold"));
            g.drawFittedText(empty ? String("-") : String(rigfile::effectName(type)), inner,
                             Justification::centred, 2, 0.8f);
        }
    }

    void drawStackTile(Graphics& g, Rectangle<float> tile, const String& title,
                       const String& name, const String& values, bool on) const
    {
        g.setColour(Colours::white.withAlpha(on ? 0.08f : 0.03f));
        g.fillRoundedRectangle(tile, 6.0f);
        g.setColour(Colours::white.withAlpha(on ? 0.35f : 0.15f));
        g.drawRoundedRectangle(tile.reduced(0.5f), 6.0f, 1.0f);

        auto inner = tile.toNearestInt().reduced(7, 4);
        const auto alpha = on ? 1.0f : 0.4f;
        g.setColour(Colours::white.withAlpha(0.5f * alpha));
        g.setFont(FontOptions(10.0f).withStyle("Bold"));
        g.drawText(title + (on ? "" : "  OFF"), inner.removeFromTop(13), Justification::centredLeft);
        g.setColour(Colours::white.withAlpha(alpha));
        g.setFont(FontOptions(13.0f).withStyle("Bold"));
        if (name.isNotEmpty())
            g.drawFittedText(name, inner.removeFromTop(18), Justification::centredLeft, 1, 0.8f);
        g.setColour(Colours::white.withAlpha(0.7f * alpha));
        g.setFont(FontOptions(11.5f));
        g.drawFittedText(values, inner, Justification::topLeft, 2, 0.8f);
    }

    void drawStackRow(Graphics& g, Rectangle<int> row) const
    {
        using namespace rigfile;
        drawRowLabel(g, row, "STACK");
        const auto width = (row.getWidth() - 2 * tileGap) / 3;
        const auto uni = [this] (int page, int param) { return String(unipolar(rig.value(page, param, 0)), 1); };
        const auto bi = [this] (int page, int param) { return signedValue(bipolar(rig.value(page, param, 8192))); };

        drawStackTile(g, row.removeFromLeft(width).toFloat(), "AMP", String(rig.ampName()),
                      "Gain " + uni(ampPage, 4) + "   Def " + uni(ampPage, 6) + "   Clarity " + uni(ampPage, 7),
                      rig.value(ampPage, 2, 1) != 0);
        row.removeFromLeft(tileGap);
        drawStackTile(g, row.removeFromLeft(width).toFloat(), "EQ", {},
                      "Bass " + bi(eqPage, 4) + "   Mid " + bi(eqPage, 5)
                          + "\nTreble " + bi(eqPage, 6) + "   Presence " + bi(eqPage, 7),
                      true);
        row.removeFromLeft(tileGap);
        drawStackTile(g, row.toFloat(), "CAB", String(rig.cabName()),
                      "Character " + bi(cabPage, 6) + "   High " + bi(cabPage, 4) + "   Low " + bi(cabPage, 5),
                      rig.value(cabPage, 2, 1) != 0);
    }

    rigfile::Rig rig;
    bool hasRigData = false;
};

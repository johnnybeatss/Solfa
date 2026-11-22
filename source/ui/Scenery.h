#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

// Procedural artwork for Solfa's "jungle village at night" look: moonlit sky, canopy and
// stilt-hut silhouettes, firelight, fireflies, carved-wood panels and a seed-of-life glyph.
// Everything is drawn with paths and gradients (no bitmaps), so it scales to any size.
namespace solfa::ui::scenery
{
    // A HiDPI-aware cached image for static artwork. Re-renders only when the size or the
    // display scale changes, so the expensive scene costs nothing per frame.
    class CachedLayer
    {
    public:
        using Renderer = std::function<void (juce::Graphics&, juce::Rectangle<float>)>;
        void draw (juce::Graphics& g, juce::Rectangle<int> bounds, const Renderer& render);
        void invalidate() { image = {}; }

    private:
        juce::Image image;
        juce::Rectangle<int> cachedBounds;
        float cachedScale = 0.0f;
    };

    // Sky, stars, moon, distant hills, palm treeline and the stilt-hut village (static parts).
    void drawNightScene (juce::Graphics&, juce::Rectangle<float> area);

    // Hanging leaves and vines along the top edge, ferns in the bottom corners.
    void drawCanopy (juce::Graphics&, juce::Rectangle<float> area);

    // The village fire (animated): where it sits for a given scene area, and its flame and glow.
    juce::Point<float> firePosition (juce::Rectangle<float> area);
    float fireScale (juce::Rectangle<float> area);
    void drawFire (juce::Graphics&, juce::Point<float> base, float scale, float timeSeconds);

    // Seed-of-life glyph: seven overlapping circles inside an outer ring.
    void drawSeedOfLife (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour, float thickness);

    // A zigzag strip with diamonds, like a carved border.
    void drawCarvedBand (juce::Graphics&, juce::Rectangle<float> area, juce::Colour);

    // Dark carved-wood panel with grain, rim, a carved strip under the header and corner studs.
    void drawCarvedPanel (juce::Graphics&, juce::Rectangle<float> area, juce::Colour accent, float headerHeight);

    // Fireflies drifting and blinking. update() advances time; draw() is cheap.
    class Fireflies
    {
    public:
        explicit Fireflies (int count, int seed = 11);
        void update (float deltaSeconds) { time += deltaSeconds; }
        void draw (juce::Graphics&, juce::Rectangle<float> area) const;
        float getTime() const noexcept { return time; }

    private:
        struct Fly
        {
            float x, y, phase, rate, size, drift;
        };
        std::vector<Fly> flies;
        float time = 7.0f; // start mid-cycle so some are lit straight away
    };
}

#include "Scenery.h"
#include "Theme.h"

#include <cmath>
#include <numbers>
#include <tuple>
#include <utility>

namespace solfa::ui::scenery
{
    namespace
    {
        constexpr float pi = std::numbers::pi_v<float>;

        // A leaf: pointed at both ends, pointing along +y before rotation.
        juce::Path leaf (juce::Point<float> base, float length, float width, float angle)
        {
            juce::Path p;
            p.startNewSubPath (0.0f, 0.0f);
            p.quadraticTo (width, length * 0.45f, 0.0f, length);
            p.quadraticTo (-width, length * 0.45f, 0.0f, 0.0f);
            p.closeSubPath();
            p.applyTransform (juce::AffineTransform::rotation (angle).translated (base));
            return p;
        }

        float groundY (juce::Rectangle<float> r) { return r.getY() + r.getHeight() * 0.9f; }

        void drawPalm (juce::Graphics& g, juce::Point<float> base, float height, float lean, juce::Colour c)
        {
            const juce::Point<float> crown { base.x + lean, base.y - height };
            juce::Path trunk;
            trunk.startNewSubPath (base);
            trunk.quadraticTo (base.x + lean * 0.2f, base.y - height * 0.6f, crown.x, crown.y);
            g.setColour (c);
            g.strokePath (trunk, juce::PathStrokeType (juce::jmax (1.5f, height * 0.035f), juce::PathStrokeType::curved,
                                     juce::PathStrokeType::rounded));
            juce::Path fronds;
            for (int i = 0; i < 8; ++i)
            {
                // Fronds fan out upwards and droop: angles measured from straight down.
                const auto spread = -1.9f + 3.8f * (float) i / 7.0f;
                const auto angle = pi + spread;
                fronds.addPath (leaf (crown, height * (0.42f - 0.06f * std::abs (spread) / 1.9f), height * 0.06f, angle));
            }
            g.fillPath (fronds);
        }

        void drawHut (juce::Graphics& g, float centreX, float ground, float w, juce::Colour body, juce::Colour light)
        {
            const auto platform = ground - w * 0.28f;
            const auto wall = w * 0.42f;
            const auto bodyW = w * 0.72f;
            // Stilts and a ladder
            g.setColour (body);
            for (float k : { -0.32f, 0.0f, 0.32f })
                g.drawLine (centreX + k * w, ground, centreX + k * w, platform, juce::jmax (1.0f, w * 0.03f));
            g.fillRect (centreX - w * 0.42f, platform - w * 0.03f, w * 0.84f, w * 0.04f);
            g.drawLine (centreX + w * 0.42f, ground, centreX + w * 0.55f, platform, 1.0f);

            // Walls and a thatched cone roof with slightly concave sides
            g.fillRect (centreX - bodyW / 2, platform - wall, bodyW, wall);
            juce::Path roof;
            const auto eave = platform - wall + w * 0.04f;
            roof.startNewSubPath (centreX - w * 0.58f, eave);
            roof.quadraticTo (centreX - w * 0.18f, eave - w * 0.18f, centreX, eave - w * 0.62f);
            roof.quadraticTo (centreX + w * 0.18f, eave - w * 0.18f, centreX + w * 0.58f, eave);
            roof.closeSubPath();
            g.fillPath (roof);

            // Warm light in the doorway, with a soft spill around it
            const juce::Rectangle<float> door (centreX - w * 0.09f, platform - wall * 0.72f, w * 0.18f, wall * 0.72f);
            g.setGradientFill (juce::ColourGradient (light.withAlpha (0.28f), door.getCentreX(), door.getCentreY(),
                light.withAlpha (0.0f), door.getCentreX() + w * 0.6f, door.getCentreY(), true));
            g.fillEllipse (door.getCentreX() - w * 0.6f, door.getCentreY() - w * 0.6f, w * 1.2f, w * 1.2f);
            g.setColour (light.withAlpha (0.85f));
            g.fillRoundedRectangle (door, w * 0.03f);
        }
    }

    //==============================================================================
    void CachedLayer::draw (juce::Graphics& g, juce::Rectangle<int> bounds, const Renderer& render)
    {
        const auto scale = juce::jmax (0.5f, g.getInternalContext().getPhysicalPixelScaleFactor());
        if (image.isNull() || bounds != cachedBounds || std::abs (scale - cachedScale) > 1.0e-3f)
        {
            cachedBounds = bounds;
            cachedScale = scale;
            image = juce::Image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt ((float) bounds.getWidth() * scale)),
                juce::jmax (1, juce::roundToInt ((float) bounds.getHeight() * scale)), true);
            juce::Graphics ig (image);
            ig.addTransform (juce::AffineTransform::scale (scale));
            render (ig, bounds.withZeroOrigin().toFloat());
        }
        g.drawImageTransformed (image,
            juce::AffineTransform::scale (1.0f / scale).translated ((float) bounds.getX(), (float) bounds.getY()));
    }

    //==============================================================================
    void drawNightScene (juce::Graphics& g, juce::Rectangle<float> r)
    {
        using juce::Colour;
        juce::ColourGradient sky (Colour (0xff0e1c30), r.getX(), r.getY(), Colour (0xff060d0a), r.getX(), r.getBottom(), false);
        sky.addColour (0.5, Colour (0xff0c211f));
        g.setGradientFill (sky);
        g.fillRect (r);

        juce::Random rng (1729);

        // Stars, denser and brighter towards the top
        for (int i = 0; i < 130; ++i)
        {
            const auto fy = std::pow (rng.nextFloat(), 1.7f) * 0.62f;
            const auto x = r.getX() + rng.nextFloat() * r.getWidth();
            const auto y = r.getY() + fy * r.getHeight();
            const auto s = 0.5f + rng.nextFloat() * 1.3f;
            g.setColour (Colour (0xffe8efff).withAlpha ((0.12f + 0.55f * rng.nextFloat()) * (1.0f - fy)));
            g.fillEllipse (x - s / 2, y - s / 2, s, s);
        }

        // Moon with a wide halo
        const auto mr = juce::jlimit (12.0f, 30.0f, juce::jmin (r.getWidth(), r.getHeight()) * 0.055f);
        const juce::Point<float> mc { r.getRight() - r.getWidth() * 0.09f, r.getY() + r.getHeight() * 0.2f };
        g.setGradientFill (juce::ColourGradient (Colour (0xffcfe4ff).withAlpha (0.16f), mc.x, mc.y, Colour (0xffcfe4ff).withAlpha (0.0f),
            mc.x + mr * 6, mc.y, true));
        g.fillEllipse (mc.x - mr * 6, mc.y - mr * 6, mr * 12, mr * 12);
        g.setGradientFill (juce::ColourGradient (Colour (0xfffdf8e4), mc.x - mr * 0.35f, mc.y - mr * 0.35f, Colour (0xffc4cfbf),
            mc.x + mr, mc.y + mr, true));
        g.fillEllipse (mc.x - mr, mc.y - mr, mr * 2, mr * 2);
        g.setColour (Colour (0xff4a5848).withAlpha (0.18f));
        g.fillEllipse (mc.x - mr * 0.45f, mc.y - mr * 0.1f, mr * 0.42f, mr * 0.36f);
        g.fillEllipse (mc.x + mr * 0.15f, mc.y - mr * 0.5f, mr * 0.3f, mr * 0.26f);
        g.fillEllipse (mc.x + mr * 0.2f, mc.y + mr * 0.3f, mr * 0.22f, mr * 0.2f);

        // Distant hills
        {
            juce::Path hills;
            const auto base = r.getY() + r.getHeight() * 0.66f;
            hills.startNewSubPath (r.getX(), r.getBottom());
            hills.lineTo (r.getX(), base);
            const int n = 9;
            for (int i = 1; i <= n; ++i)
            {
                const auto x = r.getX() + r.getWidth() * (float) i / n;
                const auto peak = base - r.getHeight() * (0.03f + 0.07f * rng.nextFloat());
                hills.quadraticTo (x - r.getWidth() / (2.0f * n), peak, x, base - r.getHeight() * 0.02f * rng.nextFloat());
            }
            hills.lineTo (r.getRight(), r.getBottom());
            hills.closeSubPath();
            g.setColour (Colour (0xff10261f));
            g.fillPath (hills);
        }

        // Mist band in front of the hills
        g.setGradientFill (juce::ColourGradient (Colour (0xff6fa59a).withAlpha (0.0f), 0, r.getY() + r.getHeight() * 0.62f,
            Colour (0xff6fa59a).withAlpha (0.07f), 0, r.getY() + r.getHeight() * 0.74f, false));
        g.fillRect (r.withTop (r.getY() + r.getHeight() * 0.62f).withHeight (r.getHeight() * 0.12f));

        // Treeline of rounded crowns
        const auto treeColour = Colour (0xff0a1914);
        {
            juce::Path crowns;
            const auto line = r.getY() + r.getHeight() * 0.79f;
            for (float x = r.getX() - 10; x < r.getRight() + 10;)
            {
                const auto rad = r.getHeight() * (0.025f + 0.035f * rng.nextFloat());
                crowns.addEllipse (x - rad, line - rad * (1.0f + rng.nextFloat()), rad * 2, rad * 2.4f);
                x += rad * (1.1f + 0.6f * rng.nextFloat());
            }
            crowns.addRectangle (r.getX(), line, r.getWidth(), r.getBottom() - line);
            g.setColour (treeColour);
            g.fillPath (crowns);
        }

        // Palms rising above the treeline
        const auto ground = groundY (r);
        for (auto [fx, fh, lean] : { std::tuple { 0.05f, 0.42f, 0.04f }, std::tuple { 0.21f, 0.33f, -0.03f },
                 std::tuple { 0.58f, 0.38f, 0.05f }, std::tuple { 0.77f, 0.30f, -0.04f }, std::tuple { 0.94f, 0.45f, -0.05f } })
            drawPalm (g, { r.getX() + r.getWidth() * fx, ground }, r.getHeight() * fh, r.getHeight() * lean, treeColour.darker (0.2f));

        // Ground and the village
        g.setColour (Colour (0xff050c09));
        g.fillRect (r.withTop (ground));
        const auto hutW = juce::jlimit (34.0f, 92.0f, r.getWidth() * 0.065f);
        const auto hutColour = Colour (0xff040a08);
        const auto lamp = Colour (0xffffa94d);
        for (auto [fx, size] : { std::pair { 0.13f, 1.0f }, std::pair { 0.30f, 0.85f }, std::pair { 0.66f, 0.85f }, std::pair { 0.85f, 1.0f } })
            drawHut (g, r.getX() + r.getWidth() * fx, ground, hutW * size, hutColour, lamp);

        // Fire pit (stones and logs; the flame itself is animated)
        const auto fire = firePosition (r);
        const auto fs = fireScale (r);
        g.setColour (Colour (0xff1a120c));
        for (int i = -2; i <= 2; ++i)
            g.fillEllipse (fire.x + (float) i * fs * 0.32f - fs * 0.14f, fire.y - fs * 0.1f, fs * 0.28f, fs * 0.18f);
        g.setColour (Colour (0xff2a1a10));
        g.drawLine (fire.x - fs * 0.5f, fire.y, fire.x + fs * 0.4f, fire.y - fs * 0.2f, juce::jmax (1.5f, fs * 0.08f));
        g.drawLine (fire.x + fs * 0.5f, fire.y, fire.x - fs * 0.4f, fire.y - fs * 0.2f, juce::jmax (1.5f, fs * 0.08f));
    }

    void drawCanopy (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto c = juce::Colour (0xff030806);
        juce::Random rng (404);
        juce::Path leaves;

        // Clusters of leaves hanging from the top edge, sparser in the middle
        for (float x = r.getX(); x < r.getRight();)
        {
            const auto t = (x - r.getX()) / juce::jmax (1.0f, r.getWidth());
            const auto edge = std::abs (t - 0.5f) * 2.0f; // 0 in the centre, 1 at the sides
            const auto count = 1 + (int) (edge * 3.0f + rng.nextFloat());
            for (int k = 0; k < count; ++k)
            {
                const auto len = (10.0f + 36.0f * edge + 14.0f * rng.nextFloat()) * juce::jlimit (0.6f, 1.4f, r.getHeight() / 400.0f);
                leaves.addPath (leaf ({ x + rng.nextFloat() * 8.0f, r.getY() - 2.0f }, len, len * 0.22f, (rng.nextFloat() - 0.5f) * 1.2f));
            }
            x += 14.0f + 26.0f * (1.0f - edge) + 10.0f * rng.nextFloat();
        }

        // A few vines with small leaves near the sides
        for (float fx : { 0.03f, 0.11f, 0.89f, 0.97f })
        {
            const auto x0 = r.getX() + r.getWidth() * fx;
            const auto len = r.getHeight() * (0.18f + 0.14f * rng.nextFloat());
            juce::Path vine;
            vine.startNewSubPath (x0, r.getY());
            vine.cubicTo (x0 + 10, r.getY() + len * 0.3f, x0 - 10, r.getY() + len * 0.7f, x0 + 3, r.getY() + len);
            g.setColour (c);
            g.strokePath (vine, juce::PathStrokeType (1.3f));
            for (float s = 0.15f; s < 1.0f; s += 0.17f)
            {
                const auto y = r.getY() + len * s;
                leaves.addPath (leaf ({ x0 + 4.0f * std::sin (s * 9.0f), y }, 9.0f, 3.0f, (s * 37.0f - std::floor (s * 37.0f) > 0.5f ? 1.0f : -1.0f) * 0.9f));
            }
        }

        // Ferns in the bottom corners
        for (auto [fx, dir] : { std::pair { 0.0f, 1.0f }, std::pair { 1.0f, -1.0f } })
        {
            const juce::Point<float> base { r.getX() + r.getWidth() * fx, r.getBottom() + 4.0f };
            const auto size = juce::jlimit (40.0f, 110.0f, r.getHeight() * 0.22f);
            for (int i = 0; i < 6; ++i)
                leaves.addPath (leaf (base, size * (0.7f + 0.3f * rng.nextFloat()), size * 0.12f, pi + dir * (0.25f + 0.22f * (float) i)));
        }

        g.setColour (c.withAlpha (0.96f));
        g.fillPath (leaves);
    }

    juce::Point<float> firePosition (juce::Rectangle<float> r) { return { r.getX() + r.getWidth() * 0.41f, groundY (r) }; }

    float fireScale (juce::Rectangle<float> r) { return juce::jlimit (10.0f, 26.0f, r.getWidth() * 0.016f); }

    void drawFire (juce::Graphics& g, juce::Point<float> base, float scale, float t)
    {
        const auto flicker = 0.82f + 0.11f * std::sin (t * 9.3f) + 0.07f * std::sin (t * 23.1f + 1.3f);
        const auto glowR = scale * 7.0f * flicker;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff8a2a).withAlpha (0.30f * flicker), base.x, base.y - scale * 0.4f,
            juce::Colour (0xffff8a2a).withAlpha (0.0f), base.x + glowR, base.y, true));
        g.fillEllipse (base.x - glowR, base.y - scale * 0.4f - glowR, glowR * 2, glowR * 2);

        // Two flame tongues swaying out of phase
        for (int k = 0; k < 2; ++k)
        {
            const auto h = scale * (k == 0 ? 1.3f : 0.85f) * (0.85f + 0.2f * std::sin (t * (7.0f + 3.0f * (float) k) + (float) k));
            const auto w = scale * (k == 0 ? 0.42f : 0.3f);
            const auto sway = scale * 0.12f * std::sin (t * 5.3f + 2.0f * (float) k);
            juce::Path flame;
            flame.startNewSubPath (base.x - w, base.y - scale * 0.1f);
            flame.quadraticTo (base.x - w * 0.8f, base.y - h * 0.6f, base.x + sway, base.y - h);
            flame.quadraticTo (base.x + w * 0.8f, base.y - h * 0.6f, base.x + w, base.y - scale * 0.1f);
            flame.closeSubPath();
            g.setGradientFill (juce::ColourGradient (juce::Colour (k == 0 ? 0xffffd27a : 0xfffff1b8), base.x, base.y,
                juce::Colour (0xffff5a1f).withAlpha (0.0f), base.x, base.y - h, false));
            g.fillPath (flame);
        }
    }

    //==============================================================================
    void drawSeedOfLife (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour colour, float thickness)
    {
        const auto r = radius / 2.0f;
        juce::Path p;
        p.addEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
        for (int k = 0; k < 6; ++k)
        {
            const auto a = (float) k * pi / 3.0f;
            const auto x = c.x + r * std::cos (a), y = c.y + r * std::sin (a);
            p.addEllipse (x - r, y - r, 2 * r, 2 * r);
        }
        p.addEllipse (c.x - radius, c.y - radius, 2 * radius, 2 * radius);
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (thickness));
    }

    void drawCarvedBand (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        const auto h = r.getHeight();
        if (h <= 0.0f || r.getWidth() <= h)
            return;
        juce::Path zig, diamonds;
        int i = 0;
        zig.startNewSubPath (r.getX(), r.getBottom());
        for (float x = r.getX() + h; x <= r.getRight(); x += h, ++i)
        {
            zig.lineTo (x, (i % 2 == 0) ? r.getY() : r.getBottom());
            if (i % 8 == 4)
            {
                const auto cx = x, cy = r.getCentreY(), d = h * 0.7f;
                diamonds.startNewSubPath (cx, cy - d);
                diamonds.lineTo (cx + d, cy);
                diamonds.lineTo (cx, cy + d);
                diamonds.lineTo (cx - d, cy);
                diamonds.closeSubPath();
            }
        }
        g.setColour (c);
        g.strokePath (zig, juce::PathStrokeType (1.0f));
        g.fillPath (diamonds);
    }

    void drawCarvedPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, float headerHeight)
    {
        constexpr float corner = 7.0f;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), corner);

        g.setGradientFill (juce::ColourGradient (theme::panel.brighter (0.12f), r.getX(), r.getY(), theme::panel.darker (0.35f),
            r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);

        // Wood grain
        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path clip;
            clip.addRoundedRectangle (r, corner);
            g.reduceClipRegion (clip);
            juce::Random rng (97);
            for (float y = r.getY() + 5.0f; y < r.getBottom(); y += 6.0f + 5.0f * rng.nextFloat())
            {
                juce::Path grain;
                grain.startNewSubPath (r.getX(), y);
                for (float x = r.getX() + 40.0f; x < r.getRight() + 40.0f; x += 40.0f)
                    grain.quadraticTo (x - 20.0f, y + (rng.nextFloat() - 0.5f) * 4.0f, x, y + (rng.nextFloat() - 0.5f) * 2.0f);
                g.setColour (juce::Colour (0xffffe2b8).withAlpha (0.018f + 0.02f * rng.nextFloat()));
                g.strokePath (grain, juce::PathStrokeType (1.0f));
            }
        }

        g.setColour (theme::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.3f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r.reduced (2.5f), corner - 2.0f, 1.0f);

        if (headerHeight > 0.0f)
            drawCarvedBand (g, { r.getX() + 12.0f, r.getY() + headerHeight, r.getWidth() - 24.0f, 4.0f }, accent.withAlpha (0.3f));

        g.setColour (accent.withAlpha (0.55f));
        for (auto p : { r.getTopLeft() + juce::Point<float> (7, 7), r.getTopRight() + juce::Point<float> (-7, 7),
                 r.getBottomLeft() + juce::Point<float> (7, -7), r.getBottomRight() + juce::Point<float> (-7, -7) })
            g.fillEllipse (p.x - 1.6f, p.y - 1.6f, 3.2f, 3.2f);
    }

    //==============================================================================
    Fireflies::Fireflies (int count, int seed)
    {
        juce::Random rng (seed);
        flies.reserve ((size_t) count);
        for (int i = 0; i < count; ++i)
            flies.push_back ({ rng.nextFloat(), 0.3f + 0.65f * rng.nextFloat(), rng.nextFloat() * 2.0f * pi, 0.5f + 0.9f * rng.nextFloat(),
                0.7f + 0.6f * rng.nextFloat(), (rng.nextFloat() - 0.5f) * 0.012f });
    }

    void Fireflies::draw (juce::Graphics& g, juce::Rectangle<float> r) const
    {
        const auto glowColour = theme::firefly;
        for (const auto& f : flies)
        {
            const auto blink = std::sin (time * f.rate + f.phase);
            const auto b = blink > 0.0f ? blink * blink * blink : 0.0f;
            if (b < 0.02f)
                continue;
            auto fx = f.x + f.drift * time + 0.025f * std::sin (time * 0.23f * f.rate + f.phase) + 0.012f * std::sin (time * 0.61f + 2.0f * f.phase);
            fx -= std::floor (fx);
            const auto fy = f.y + 0.03f * std::sin (time * 0.31f + 1.7f * f.phase);
            const auto x = r.getX() + fx * r.getWidth(), y = r.getY() + fy * r.getHeight();
            const auto glow = 7.0f * f.size;
            g.setGradientFill (juce::ColourGradient (glowColour.withAlpha (0.45f * b), x, y, glowColour.withAlpha (0.0f), x + glow, y, true));
            g.fillEllipse (x - glow, y - glow, glow * 2, glow * 2);
            g.setColour (juce::Colour (0xfff6ffd0).withAlpha (0.9f * b));
            const auto core = 1.1f * f.size;
            g.fillEllipse (x - core, y - core, core * 2, core * 2);
        }
    }
}

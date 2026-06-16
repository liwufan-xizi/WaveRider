#pragma once

#include <QObject>
#include <QColor>

namespace WaveRider {

/// Dynamic theme colors computed from the user's background image.
/// All custom-painted widgets read their colors from this singleton.
struct ThemeColors {
    QColor accent;           // dominant hue from background, boosted
    QColor accentDim;        // same hue, lower saturation/alpha
    QColor textPrimary;      // near-white on dark bg, near-black on light bg
    QColor textSecondary;    // ~60% opacity of textPrimary
    QColor surface;          // semi-transparent panel background
    QColor surfaceBorder;    // subtle border for panels
    float  overlayAlpha = 0.30f;  // background dimming
    bool   isDarkBg     = true;    // true => light text on dark background
};

/// Singleton store for the active dynamic theme colors.
class ThemeConfig : public QObject {
    Q_OBJECT
public:
    static ThemeConfig* instance();

    const ThemeColors& colors() const { return m_colors; }
    void setColors(const ThemeColors& c);

    // ── Convenience accessors for paint code ──────────
    QColor accent()        const { return m_colors.accent; }
    QColor accentDim()     const { return m_colors.accentDim; }
    QColor textPrimary()   const { return m_colors.textPrimary; }
    QColor textSecondary() const { return m_colors.textSecondary; }
    QColor surface()       const { return m_colors.surface; }
    QColor surfaceBorder() const { return m_colors.surfaceBorder; }
    float  overlayAlpha()  const { return m_colors.overlayAlpha; }
    bool   isDarkBg()      const { return m_colors.isDarkBg; }

    /// Restore to Phigros default colors (used when no custom background).
    void resetToDefaults();

signals:
    /// Emitted when colors change. Custom-painted widgets connect to repaint.
    void themeColorsChanged();

private:
    explicit ThemeConfig(QObject* parent = nullptr);
    ThemeConfig(const ThemeConfig&) = delete;
    ThemeConfig& operator=(const ThemeConfig&) = delete;

    ThemeColors m_colors;
};

} // namespace WaveRider

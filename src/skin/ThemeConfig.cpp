#include "skin/ThemeConfig.h"

namespace WaveRider {

ThemeConfig::ThemeConfig(QObject* parent)
    : QObject(parent)
{
    resetToDefaults();
}

ThemeConfig* ThemeConfig::instance()
{
    static ThemeConfig cfg;
    return &cfg;
}

void ThemeConfig::setColors(const ThemeColors& c)
{
    m_colors = c;
    emit themeColorsChanged();
}

void ThemeConfig::resetToDefaults()
{
    // Phigros mint default — clean, cold, modern
    m_colors.accent        = QColor(0x00, 0xd4, 0xaa);   // #00d4aa mint
    m_colors.accentDim     = QColor(0x00, 0xd4, 0xaa, 80);
    m_colors.textPrimary   = QColor(0xe8, 0xe8, 0xe8);   // near-white
    m_colors.textSecondary = QColor(0x88, 0x88, 0x8e);   // medium gray
    m_colors.surface       = QColor(0x0f, 0x0f, 0x1e, 220);  // semi-transparent dark
    m_colors.surfaceBorder = QColor(0xff, 0xff, 0xff, 15);   // very subtle white
    m_colors.overlayAlpha  = 0.30f;
    m_colors.isDarkBg      = true;

    emit themeColorsChanged();
}

} // namespace WaveRider

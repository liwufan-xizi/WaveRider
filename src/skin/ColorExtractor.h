#pragma once

#include <QColor>
#include <QPixmap>

namespace WaveRider {

/// Result of color extraction from a background image.
struct ColorExtractionResult {
    QColor dominantAccent;   // most vibrant hue, saturation/value clamped
    float  averageValue;     // 0-1, determines light vs dark
    bool   isDark;           // true if averageValue < 0.5
};

/// Stateless utility: samples a QPixmap and extracts dominant color info.
class ColorExtractor {
public:
    /// Sample the pixmap on a grid and extract dominant colors.
    /// @param pixmap  source image (any size)
    /// @param gridSize  sampling grid dimension (e.g. 12 → 12×12 = 144 samples)
    static ColorExtractionResult extract(const QPixmap& pixmap,
                                          int gridSize = 12);
};

} // namespace WaveRider

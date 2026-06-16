#include "skin/ColorExtractor.h"
#include <QImage>
#include <QtMath>
#include <algorithm>

namespace WaveRider {

ColorExtractionResult ColorExtractor::extract(const QPixmap& pixmap, int gridSize)
{
    ColorExtractionResult result;

    if (pixmap.isNull()) {
        result.dominantAccent = QColor(0x00, 0xd4, 0xaa);
        result.averageValue   = 0.2f;
        result.isDark         = true;
        return result;
    }

    // ── 1. Downscale to grid size for cheap sampling ──────
    QImage img = pixmap.scaled(gridSize, gridSize,
                               Qt::IgnoreAspectRatio,
                               Qt::SmoothTransformation)
                     .toImage();

    // ── 2. Collect HSV values from all pixels ─────────────
    struct Sample {
        int   hue;        // 0-359
        float saturation; // 0-1
        float value;      // 0-1
        float score;
    };

    QVector<Sample> samples;
    samples.reserve(gridSize * gridSize);
    double totalValue = 0.0;

    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            QColor c = img.pixelColor(x, y);
            float s = c.hsvSaturationF();
            float v = c.valueF();

            // Skip very dark pixels (near black)
            if (v < 0.08f) continue;
            // Skip very light pixels (near white)
            if (v > 0.95f && s < 0.1f) continue;

            int h = c.hsvHue();
            if (h < 0) h = 0;

            Sample sample;
            sample.hue        = h;
            sample.saturation = s;
            sample.value      = v;

            // Score: favor saturated colors with medium brightness
            float brightnessBonus = 1.0f - qAbs(v - 0.55f) * 1.2f;
            sample.score = s * 0.7f + brightnessBonus * 0.3f;

            samples.append(sample);
            totalValue += v;
        }
    }

    if (samples.isEmpty()) {
        // All pixels too dark/light — default to dark theme
        result.dominantAccent = QColor(0x00, 0xd4, 0xaa);
        result.averageValue   = static_cast<float>(totalValue / (gridSize * gridSize));
        result.isDark         = true;
        return result;
    }

    // ── 3. Sort by score, pick the best hue ───────────────
    std::sort(samples.begin(), samples.end(),
        [](const Sample& a, const Sample& b) { return a.score > b.score; });

    // Take top 20% of samples, find the most common hue among them
    int topCount = qMax(1, samples.size() / 5);

    // Simple hue histogram (36 buckets, 10° each)
    int hueBuckets[36] = {};
    float hueSat[36] = {};
    for (int i = 0; i < topCount; ++i) {
        int bucket = samples[i].hue / 10;
        if (bucket >= 36) bucket = 35;
        hueBuckets[bucket]++;
        hueSat[bucket] += samples[i].saturation;
    }

    int bestBucket = 0;
    for (int i = 1; i < 36; ++i) {
        if (hueBuckets[i] > hueBuckets[bestBucket]) {
            bestBucket = i;
        }
    }

    float avgSat = hueBuckets[bestBucket] > 0
                   ? hueSat[bestBucket] / hueBuckets[bestBucket] : 0.6f;

    float dominantHue = bestBucket * 10.0f + 5.0f;  // center of bucket

    // ── 4. Clamp for readability ──────────────────────────
    float accentSat = qBound(0.55f, avgSat * 1.1f, 0.90f);
    float accentVal = qBound(0.45f, samples[0].value, 0.80f);

    result.dominantAccent = QColor::fromHsvF(
        dominantHue / 360.0f,
        accentSat,
        accentVal
    );

    // ── 5. Average brightness → light/dark decision ───────
    int pixelCount = gridSize * gridSize;
    result.averageValue = static_cast<float>(totalValue / pixelCount);
    result.isDark = result.averageValue < 0.50f;

    return result;
}

} // namespace WaveRider

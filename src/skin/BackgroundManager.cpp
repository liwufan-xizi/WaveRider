#include "skin/BackgroundManager.h"
#include "skin/ThemeConfig.h"
#include "skin/ColorExtractor.h"
#include "core/ConfigManager.h"
#include "core/SignalBus.h"
#include <QPainter>
#include <QFileInfo>
#include <QtMath>

namespace WaveRider {

BackgroundManager::BackgroundManager(QObject* parent)
    : QObject(parent)
{
}

BackgroundManager* BackgroundManager::instance()
{
    static BackgroundManager mgr;
    return &mgr;
}

// ── Core API ──────────────────────────────────────

bool BackgroundManager::setBackground(const QString& imagePath)
{
    if (!QFileInfo::exists(imagePath)) return false;

    QPixmap pix(imagePath);
    if (pix.isNull()) return false;

    m_imagePath = imagePath;
    m_originalPixmap = pix;

    // ── Extract dominant color and push to theme ───────
    auto extraction = ColorExtractor::extract(pix);
    ThemeColors tc = ThemeConfig::instance()->colors();
    tc.accent        = extraction.dominantAccent;
    tc.accentDim     = QColor(extraction.dominantAccent.red(),
                              extraction.dominantAccent.green(),
                              extraction.dominantAccent.blue(), 80);
    tc.isDarkBg      = extraction.isDark;
    tc.overlayAlpha  = m_overlayOpacity;
    if (extraction.isDark) {
        tc.textPrimary   = QColor(0xe8, 0xe8, 0xe8);
        tc.textSecondary = QColor(0x88, 0x88, 0x8e);
        tc.surface       = QColor(0x0f, 0x0f, 0x1e, 220);
        tc.surfaceBorder = QColor(0xff, 0xff, 0xff, 15);
    } else {
        tc.textPrimary   = QColor(0x1a, 0x1a, 0x1a);
        tc.textSecondary = QColor(0x66, 0x66, 0x6a);
        tc.surface       = QColor(0xf0, 0xf0, 0xf5, 220);
        tc.surfaceBorder = QColor(0x00, 0x00, 0x00, 30);
    }
    ThemeConfig::instance()->setColors(tc);

    saveSettings();
    emit backgroundChanged();
    SignalBus::instance()->backgroundChanged();
    return true;
}

void BackgroundManager::clearBackground()
{
    m_imagePath.clear();
    m_originalPixmap = QPixmap();
    ThemeConfig::instance()->resetToDefaults();
    saveSettings();
    emit backgroundChanged();
    SignalBus::instance()->backgroundChanged();
}

bool BackgroundManager::hasCustomBackground() const
{
    return !m_imagePath.isEmpty() && !m_originalPixmap.isNull();
}

QString BackgroundManager::currentBackgroundPath() const
{
    return m_imagePath;
}

// ── Display mode ──────────────────────────────────

void BackgroundManager::setDisplayMode(BackgroundDisplayMode mode)
{
    m_displayMode = mode;
    saveSettings();
    emit backgroundChanged();
}

BackgroundDisplayMode BackgroundManager::displayMode() const
{
    return m_displayMode;
}

// ── Visual effects ────────────────────────────────

void BackgroundManager::setBlurRadius(int radius)
{
    m_blurRadius = qBound(0, radius, 100);
    saveSettings();
    emit backgroundChanged();
}

int BackgroundManager::blurRadius() const
{
    return m_blurRadius;
}

void BackgroundManager::setOverlayOpacity(float opacity)
{
    m_overlayOpacity = qBound(0.0f, opacity, 1.0f);
    saveSettings();
    emit backgroundChanged();
}

float BackgroundManager::overlayOpacity() const
{
    return m_overlayOpacity;
}

// ── Rendering ─────────────────────────────────────

QPixmap BackgroundManager::renderBackground(const QSize& targetSize) const
{
    if (m_originalPixmap.isNull()) return QPixmap();

    QPixmap result(targetSize);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRect drawRect;
    QSize srcSize = m_originalPixmap.size();

    switch (m_displayMode) {
    case BackgroundDisplayMode::Fill: {
        // Scale and crop to fill
        double scaleW = static_cast<double>(targetSize.width()) / srcSize.width();
        double scaleH = static_cast<double>(targetSize.height()) / srcSize.height();
        double scale = qMax(scaleW, scaleH);
        int sw = static_cast<int>(srcSize.width() * scale);
        int sh = static_cast<int>(srcSize.height() * scale);
        drawRect = QRect((targetSize.width() - sw) / 2, (targetSize.height() - sh) / 2, sw, sh);
        break;
    }
    case BackgroundDisplayMode::Fit: {
        // Scale to fit (may leave blank areas)
        double scaleW = static_cast<double>(targetSize.width()) / srcSize.width();
        double scaleH = static_cast<double>(targetSize.height()) / srcSize.height();
        double scale = qMin(scaleW, scaleH);
        int sw = static_cast<int>(srcSize.width() * scale);
        int sh = static_cast<int>(srcSize.height() * scale);
        drawRect = QRect((targetSize.width() - sw) / 2, (targetSize.height() - sh) / 2, sw, sh);
        break;
    }
    case BackgroundDisplayMode::Stretch:
        drawRect = result.rect();
        break;
    case BackgroundDisplayMode::Tile: {
        // Draw tile by tile
        for (int x = 0; x < targetSize.width(); x += srcSize.width()) {
            for (int y = 0; y < targetSize.height(); y += srcSize.height()) {
                p.drawPixmap(x, y, m_originalPixmap);
            }
        }
        // Apply overlay
        if (m_overlayOpacity > 0.0f) {
            p.fillRect(result.rect(), QColor(0, 0, 0, static_cast<int>(m_overlayOpacity * 255)));
        }
        p.end();
        return result;
    }
    case BackgroundDisplayMode::Center: {
        drawRect = QRect((targetSize.width() - srcSize.width()) / 2,
                         (targetSize.height() - srcSize.height()) / 2,
                         srcSize.width(), srcSize.height());
        break;
    }
    }

    // Draw the scaled/cropped pixmap
    if (m_displayMode != BackgroundDisplayMode::Tile) {
        QPixmap scaled = m_originalPixmap.scaled(drawRect.size(), Qt::KeepAspectRatioByExpanding,
                                                   Qt::SmoothTransformation);
        QPixmap cropped = scaled.copy(QRect((scaled.width() - drawRect.width()) / 2,
                                              (scaled.height() - drawRect.height()) / 2,
                                              drawRect.width(), drawRect.height()));
        p.drawPixmap(drawRect.topLeft(), cropped);

        // Dark overlay for readability
        if (m_overlayOpacity > 0.0f) {
            p.fillRect(result.rect(), QColor(0, 0, 0, static_cast<int>(m_overlayOpacity * 255)));
        }
    }

    p.end();
    return result;
}

// ── Persistence ───────────────────────────────────

void BackgroundManager::saveSettings()
{
    auto* cfg = ConfigManager::instance();
    cfg->setBackgroundImagePath(m_imagePath);
    cfg->setBackgroundDisplayMode(static_cast<int>(m_displayMode));
    cfg->setBackgroundBlurRadius(m_blurRadius);
    cfg->setBackgroundOverlayOpacity(m_overlayOpacity);
}

void BackgroundManager::loadSettings()
{
    auto* cfg = ConfigManager::instance();
    QString path = cfg->backgroundImagePath();
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        setBackground(path);
    }
    m_displayMode = static_cast<BackgroundDisplayMode>(cfg->backgroundDisplayMode());
    m_blurRadius = cfg->backgroundBlurRadius();
    m_overlayOpacity = cfg->backgroundOverlayOpacity();
}

QPixmap BackgroundManager::applyBlur(const QPixmap& source, int radius) const
{
    if (radius <= 0) return source;
    // Simple downsample-upsample blur approximation
    int factor = qBound(1, radius / 10 + 1, 8);
    QSize small = source.size() / factor;
    QPixmap blurred = source.scaled(small, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return blurred.scaled(source.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

} // namespace WaveRider

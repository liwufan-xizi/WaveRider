#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>
#include "core/Constants.h"

namespace WaveRider {

/// Manages user-customizable background images.
/// Pure local file selection — no network required.
/// Singleton — use BackgroundManager::instance().
class BackgroundManager : public QObject {
    Q_OBJECT
public:
    static BackgroundManager* instance();

    // ── Core API (callable by client code) ────────────
    bool    setBackground(const QString& imagePath);
    void    clearBackground();
    bool    hasCustomBackground() const;
    QString currentBackgroundPath() const;

    // ── Display mode ──────────────────────────────────
    void            setDisplayMode(BackgroundDisplayMode mode);
    BackgroundDisplayMode displayMode() const;

    // ── Visual effects ────────────────────────────────
    void  setBlurRadius(int radius);       // 0–100, 0 = no blur
    int   blurRadius() const;
    void  setOverlayOpacity(float opacity); // 0.0–1.0
    float overlayOpacity() const;

    // ── Rendering ─────────────────────────────────────
    QPixmap renderBackground(const QSize& targetSize) const;

    // ── Persistence ───────────────────────────────────
    void saveSettings();
    void loadSettings();

signals:
    void backgroundChanged();

private:
    explicit BackgroundManager(QObject* parent = nullptr);
    BackgroundManager(const BackgroundManager&) = delete;
    BackgroundManager& operator=(const BackgroundManager&) = delete;

    QPixmap applyBlur(const QPixmap& source, int radius) const;

    QString                m_imagePath;
    QPixmap                m_originalPixmap;
    BackgroundDisplayMode  m_displayMode = BackgroundDisplayMode::Fill;
    int                    m_blurRadius = 0;
    float                  m_overlayOpacity = 0.3f;
};

} // namespace WaveRider

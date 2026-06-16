#pragma once

#include <QMainWindow>
#include "core/Constants.h"

namespace WaveRider {

class AudioEngine;
class PlaylistModel;
class PlaylistManager;
class PlayerControlBar;
class PlaylistPanel;
class LyricPanel;
class LyricModel;
class LyricFetcher;
class DSPChain;
class DSPPanel;
class FavoritesPanel;
class SkinSelectorDialog;
class BackgroundDialog;
class SpectrumWidget;
class TrackInfoWidget;
class VisualArea;

/// Main application window — 16:9 Phigros-style UI.
/// Fixed preset sizes, central visual area, compact controls.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool initialize();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onTrackStarted(const QString& filePath, const QString& title, const QString& artist);
    void onTrackFinished(const QString& filePath);
    void onPositionChanged(qint64 posMs, qint64 durMs);
    void onVolumeChanged(float level);
    void onPlayModeChanged(PlayMode mode);
    void onPlaylistDoubleClicked(int index);
    void onOpenFiles();
    void onOpenFolder();
    void onLoadM3u();
    void onSaveM3u();
    void onSkinChanged(const QString& skinName);
    void setPresetSize(int index);

private:
    void setupUi();
    void setupConnections();
    void setupMenus();
    void saveWindowState();
    void restoreWindowState();
    void applyBackground();
    qreal scaleFactor() const;
    void  applyScaleToChildren();

    // Core
    AudioEngine*      m_audioEngine   = nullptr;
    PlaylistModel*    m_playlistModel = nullptr;
    PlaylistManager*  m_playlistMgr   = nullptr;
    LyricModel*       m_lyricModel    = nullptr;
    LyricFetcher*     m_lyricFetcher  = nullptr;
    DSPChain*         m_dspChain      = nullptr;

    // State
    QString           m_currentFilePath;
    qreal             m_scale = 1.0;

    // UI
    PlayerControlBar* m_controlBar    = nullptr;
    PlaylistPanel*    m_playlistPanel = nullptr;
    LyricPanel*       m_lyricPanel    = nullptr;
    DSPPanel*         m_dspPanel      = nullptr;
    FavoritesPanel*   m_favPanel      = nullptr;
    SkinSelectorDialog* m_skinSelector = nullptr;
    BackgroundDialog* m_bgDialog      = nullptr;
    SpectrumWidget*   m_spectrum      = nullptr;
    TrackInfoWidget*  m_trackInfo     = nullptr;
    VisualArea*       m_visualArea    = nullptr;
};

} // namespace WaveRider

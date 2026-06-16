#include "ui/MainWindow.h"
#include "ui/PlayerControlBar.h"
#include "ui/PlaylistPanel.h"
#include "ui/LyricPanel.h"
#include "ui/DSPPanel.h"
#include "ui/FavoritesPanel.h"
#include "ui/SkinSelectorDialog.h"
#include "ui/BackgroundDialog.h"
#include "ui/SpectrumWidget.h"
#include "ui/TrackInfoWidget.h"
#include "ui/VisualArea.h"

#include "audio/AudioEngine.h"
#include "audio/AudioMetadata.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistManager.h"
#include "playlist/PlayModeEngine.h"
#include "core/SignalBus.h"
#include "core/ConfigManager.h"
#include "skin/SkinManager.h"
#include "skin/BackgroundManager.h"
#include "skin/ThemeConfig.h"
#include "lyrics/LyricModel.h"
#include "lyrics/LyricFetcher.h"
#include "lyrics/LyricParser.h"
#include "dsp/DSPChain.h"
#include "dsp/builtin/EqualizerEffect.h"
#include "dsp/builtin/CompressorEffect.h"
#include "dsp/builtin/ReverbEffect.h"
#include "favorites/FavoritesManager.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

namespace WaveRider {

// ── Preset sizes (16:9) ───────────────────────────────────
namespace {
    struct PresetSize { int w, h; const char* label; };
    const PresetSize kPresets[] = {
        { 800,  450,  "Small  (800×450)"   },
        { 960,  540,  "Medium (960×540)"   },
        { 1280, 720,  "Large  (1280×720)"  },
    };
    constexpr int kDefaultPreset = 1;  // 960×540
}

// ============================================================
// Construction
// ============================================================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("WaveRider");
    setMinimumSize(640, 360);
    setPresetSize(kDefaultPreset);
}

MainWindow::~MainWindow()
{
    saveWindowState();
}

bool MainWindow::initialize()
{
    // ── Core modules ────────────────────────────────
    m_audioEngine = new AudioEngine(this);
    if (!m_audioEngine->initialize(reinterpret_cast<HWND>(winId()))) {
        QMessageBox::critical(this, "Error", "Failed to initialize audio engine.");
        return false;
    }

    m_playlistModel = new PlaylistModel(this);
    m_playlistMgr   = new PlaylistManager(m_playlistModel, m_audioEngine, this);
    m_lyricModel    = new LyricModel(this);
    m_lyricFetcher  = new LyricFetcher(this);

    // ── DSP chain ────────────────────────────────────
    m_dspChain = new DSPChain(this);
    m_dspChain->setStream(m_audioEngine->streamHandle());
    // Pre-create built-in effects (added to chain on demand)
    auto* eqEffect = new EqualizerEffect(m_dspChain);
    m_dspChain->addEffect(eqEffect);
    // Compressor and reverb are created but bypassed by default
    auto* compEffect = new CompressorEffect(m_dspChain);
    compEffect->setBypass(true);
    m_dspChain->addEffect(compEffect);
    auto* reverbEffect = new ReverbEffect(m_dspChain);
    reverbEffect->setBypass(true);
    m_dspChain->addEffect(reverbEffect);

    // Restore volume
    float savedVol = ConfigManager::instance()->volume();
    m_audioEngine->setVolume(savedVol);

    // ── UI ──────────────────────────────────────────
    setupUi();
    setupMenus();
    setupConnections();
    restoreWindowState();

    // ── Restore last playlist ───────────────────────
    QString lastPlaylist = ConfigManager::instance()->lastPlaylistPath();
    if (!lastPlaylist.isEmpty() && QFile::exists(lastPlaylist)) {
        m_playlistMgr->loadM3u(lastPlaylist);
    }

    return true;
}

// ============================================================
// UI setup — 16:9 Phigros layout
// ============================================================
void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Track info (top, 56px) ──────────────────────
    m_trackInfo = new TrackInfoWidget(this);
    m_trackInfo->setFixedHeight(56);
    mainLayout->addWidget(m_trackInfo);

    // ── VisualArea (center, stretches) ──────────────
    m_visualArea = new VisualArea(this);

    m_spectrum = new SpectrumWidget(this);
    m_spectrum->setAudioEngine(m_audioEngine);
    m_visualArea->setSpectrumWidget(m_spectrum);

    m_lyricPanel = new LyricPanel(this);
    m_lyricPanel->setModel(m_lyricModel);
    m_visualArea->setLyricPanel(m_lyricPanel);

    mainLayout->addWidget(m_visualArea, 1);

    // ── Control bar (bottom) ─────────────────────────
    m_controlBar = new PlayerControlBar(this);
    mainLayout->addWidget(m_controlBar);

    // ── Playlist overlay (hidden by default, slides in from right) ──
    m_playlistPanel = new PlaylistPanel(m_playlistModel, this);
    m_playlistPanel->hide();

    // ── DSP panel overlay (hidden by default) ────────
    m_dspPanel = new DSPPanel(m_dspChain, this);
    m_dspPanel->setVisible(false);
    connect(m_dspPanel, &DSPPanel::panelClosed, this, [this]() {
        m_dspPanel->setVisible(false);
    });

    // ── Favorites overlay (hidden by default, slides in from right) ──
    m_favPanel = new FavoritesPanel(this);
    m_favPanel->hide();

    // ── Skin selector (hidden by default, floating dialog) ──
    m_skinSelector = new SkinSelectorDialog(this);

    // ── Background dialog (hidden by default, floating dialog) ──
    m_bgDialog = new BackgroundDialog(this);

    // Status bar
    statusBar()->showMessage("Ready");
}

// ============================================================
// Menus
// ============================================================
void MainWindow::setupMenus()
{
    auto* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // ── File ─────────────────────────────────────────
    auto* fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&Open Files...", this, &MainWindow::onOpenFiles, QKeySequence::Open);
    fileMenu->addAction("Open &Folder...", this, &MainWindow::onOpenFolder);
    fileMenu->addSeparator();
    fileMenu->addAction("&Load Playlist...", this, &MainWindow::onLoadM3u);
    fileMenu->addAction("&Save Playlist...", this, &MainWindow::onSaveM3u);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    // ── View ─────────────────────────────────────────
    auto* viewMenu = menuBar->addMenu("&View");

    // Preset sizes
    auto* sizeMenu = viewMenu->addMenu("Window &Size");
    auto* sizeGroup = new QActionGroup(this);
    sizeGroup->setExclusive(true);
    for (int i = 0; i < 3; ++i) {
        auto* action = sizeMenu->addAction(kPresets[i].label);
        action->setCheckable(true);
        action->setActionGroup(sizeGroup);
        if (i == kDefaultPreset) action->setChecked(true);
        connect(action, &QAction::triggered, this, [this, i]() { setPresetSize(i); });
    }

    viewMenu->addSeparator();

    // Skin
    viewMenu->addAction("Change &Skin...", this, [this]() {
        m_skinSelector->showDialog();
    });

    // Background
    viewMenu->addAction("&Background Settings...", this, [this]() {
        m_bgDialog->showDialog();
    });

    viewMenu->addSeparator();

    // DSP panel toggle
    QAction* dspAction = viewMenu->addAction("&DSP Effects...");
    dspAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(dspAction, &QAction::triggered, this, [this]() {
        bool visible = !m_dspPanel->isVisible();
        m_dspPanel->setVisible(visible);
        if (visible) {
            // Center the panel in the window's client area
            int x = (width()  - m_dspPanel->width())  / 2;
            int y = (height() - m_dspPanel->height()) / 2;
            m_dspPanel->move(x, y);
            m_dspPanel->raise();
        }
        statusBar()->showMessage(visible ? "DSP panel shown" : "DSP panel hidden", 2000);
    });

    // Favorites panel toggle
    QAction* favAction = viewMenu->addAction("&Favorites");
    favAction->setShortcut(QKeySequence("Ctrl+F"));
    connect(favAction, &QAction::triggered, this, [this]() {
        if (!m_favPanel) return;
        if (m_favPanel->isOpen()) {
            m_favPanel->slideOut();
        } else {
            m_favPanel->refreshFavorites();
            m_favPanel->slideIn();
        }
        statusBar()->showMessage(
            m_favPanel->isOpen() ? "Favorites panel shown" : "Favorites panel hidden", 2000);
    });

    // ── Help ─────────────────────────────────────────
    auto* helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About WaveRider...", this, [this]() {
        QMessageBox::about(this, "About WaveRider",
            "<h2>WaveRider v1.0</h2>"
            "<p>A customizable audio player built with Qt and BASS.</p>"
            "<p>Supports MP3, FLAC, WAV, AAC, OGG and more.</p>");
    });
}

// ============================================================
// Preset sizes
// ============================================================
void MainWindow::setPresetSize(int index)
{
    if (index < 0 || index > 2) index = kDefaultPreset;
    resize(kPresets[index].w, kPresets[index].h);
}

// ============================================================
// Connections
// ============================================================
void MainWindow::setupConnections()
{
    auto* bus = SignalBus::instance();

    // ── Control bar → Engine ─────────────────────────
    connect(m_controlBar, &PlayerControlBar::playClicked,
            m_audioEngine, &AudioEngine::play);
    connect(m_controlBar, &PlayerControlBar::pauseClicked,
            m_audioEngine, &AudioEngine::pause);
    connect(m_controlBar, &PlayerControlBar::stopClicked,
            m_audioEngine, &AudioEngine::stop);
    connect(m_controlBar, &PlayerControlBar::nextClicked,
            m_playlistMgr, &PlaylistManager::playNext);
    connect(m_controlBar, &PlayerControlBar::prevClicked,
            m_playlistMgr, &PlaylistManager::playPrev);
    connect(m_controlBar, &PlayerControlBar::seekRequested, this, [this](qint64 ms) {
        m_audioEngine->seek(ms / 1000.0);
    });
    connect(m_controlBar, &PlayerControlBar::volumeChanged,
            this, &MainWindow::onVolumeChanged);
    connect(m_controlBar, &PlayerControlBar::playModeCycleClicked,
            m_playlistMgr, &PlaylistManager::cyclePlayMode);

    // ── Playlist → Engine ────────────────────────────
    connect(m_playlistPanel, &PlaylistPanel::trackDoubleClicked,
            this, &MainWindow::onPlaylistDoubleClicked);

    // ── Audio engine → UI ────────────────────────────
    connect(m_audioEngine, &AudioEngine::positionUpdated,
            this, &MainWindow::onPositionChanged);
    connect(m_audioEngine, &AudioEngine::stateChanged, this, [this](PlaybackState state) {
        m_controlBar->setPlaying(state == PlaybackState::Playing);
    });

    // ── SignalBus → UI ──────────────────────────────
    connect(bus, &SignalBus::trackStarted, this, &MainWindow::onTrackStarted);
    connect(bus, &SignalBus::trackFinished, this, &MainWindow::onTrackFinished);
    connect(bus, &SignalBus::playModeChanged, this, &MainWindow::onPlayModeChanged);
    connect(bus, &SignalBus::backgroundChanged, this, &MainWindow::applyBackground);

    // ── LyricFetcher → LyricModel ────────────────────
    connect(m_lyricFetcher, &LyricFetcher::lyricsFetched, this,
        [this](const QString&, const QString&, const QString& lrcContent) {
            if (m_lyricModel->loadFromText(lrcContent)) {
                m_lyricPanel->setStatus(LyricStatus::Loaded);
            }
        });
    connect(m_lyricFetcher, &LyricFetcher::lyricsNotFound, this,
        [this](const QString&, const QString&) {
            m_lyricPanel->setStatus(LyricStatus::NotFound);
        });
    connect(m_lyricFetcher, &LyricFetcher::fetchError, this,
        [this](const QString&) {
            m_lyricPanel->setStatus(LyricStatus::NotFound);
        });

    // ── LyricModel → LyricPanel ──────────────────────
    connect(m_lyricModel, &LyricModel::modelLoaded,
            m_lyricPanel, [this]() { m_lyricPanel->update(); });
    connect(m_lyricModel, &LyricModel::modelCleared,
            m_lyricPanel, [this]() { m_lyricPanel->update(); });

    // ── Playlist toggle ──────────────────────────────
    connect(m_controlBar, &PlayerControlBar::playlistToggleClicked, this, [this]() {
        if (m_playlistPanel->isOpen()) {
            m_playlistPanel->slideOut();
        } else {
            m_playlistPanel->slideIn();
        }
    });

    // ── Favorite toggle ──────────────────────────────
    connect(m_controlBar, &PlayerControlBar::favoriteToggleClicked, this, [this]() {
        if (m_currentFilePath.isEmpty()) return;
        FavoritesManager::instance()->toggleFavorite(m_currentFilePath);
    });

    // ── SignalBus favorites → heart state ────────────
    connect(bus, &SignalBus::favoriteAdded, this, [this](const QString& filePath) {
        if (filePath == m_currentFilePath)
            m_controlBar->setCurrentFavorited(true);
    });
    connect(bus, &SignalBus::favoriteRemoved, this, [this](const QString& filePath) {
        if (filePath == m_currentFilePath)
            m_controlBar->setCurrentFavorited(false);
    });

    // ── Favorites panel → playback ───────────────────
    connect(m_favPanel, &FavoritesPanel::trackDoubleClicked, this, [this](const QString& filePath) {
        int idx = m_playlistModel->findTrackByPath(filePath);
        if (idx >= 0) {
            m_playlistMgr->playTrackAt(idx);
        } else {
            m_playlistModel->appendTrack(filePath);
            m_playlistMgr->playTrackAt(m_playlistModel->trackCount() - 1);
        }
        m_favPanel->slideOut();
    });

    // ── Skin selector ─────────────────────────────────
    connect(m_skinSelector, &SkinSelectorDialog::dialogClosed, this, [this]() {
        statusBar()->showMessage("Skin: " + SkinManager::instance()->currentSkin().displayName, 2000);
    });

    // ── Background dialog ────────────────────────────
    connect(m_bgDialog, &BackgroundDialog::dialogClosed, this, [this]() {
        statusBar()->showMessage("Background settings updated", 2000);
    });
    connect(m_bgDialog, &BackgroundDialog::backgroundChanged, this, [this]() {
        applyBackground();
    });

    // ── Theme changes → repaint all custom widgets ───
    connect(ThemeConfig::instance(), &ThemeConfig::themeColorsChanged, this, [this]() {
        m_controlBar->update();
        m_trackInfo->update();
        m_spectrum->update();
        m_lyricPanel->update();
        m_playlistPanel->update();
        m_dspPanel->update();
        m_favPanel->update();
        applyBackground();
    });

    // ── Initial play mode ────────────────────────────
    m_controlBar->setPlayMode(m_playlistMgr->playMode());
}

// ============================================================
// Slots
// ============================================================
void MainWindow::onTrackStarted(const QString& filePath, const QString&, const QString&)
{
    // Track current file for favorite state
    m_currentFilePath = filePath;
    m_controlBar->setCurrentFavorited(
        FavoritesManager::instance()->isFavorite(filePath));

    int idx = m_playlistModel->findTrackByPath(filePath);
    if (idx >= 0) {
        auto meta = m_playlistModel->metadataAt(idx);
        m_controlBar->setTrackInfo(meta.title, meta.artist, meta.durationMs);
        m_trackInfo->setMetadata(meta);
        m_controlBar->setPlaying(true);
        statusBar()->showMessage("Now playing: " + meta.title + " — " + meta.artist);

        // Lyrics: 3-tier loading
        QString lrcPath = LyricParser::findLrcFile(filePath);
        if (!lrcPath.isEmpty() && m_lyricModel->loadFromFile(lrcPath)) {
            m_lyricPanel->setStatus(LyricStatus::Loaded);
        } else if (m_lyricModel->loadFromText(
                       LyricFetcher::cachedLyrics(meta.artist, meta.title))) {
            m_lyricPanel->setStatus(LyricStatus::Loaded);
        } else {
            m_lyricPanel->setStatus(LyricStatus::Searching);
            m_lyricFetcher->fetchLyrics(meta.artist, meta.title);
        }

        // DSP: re-apply effects to new stream
        if (m_dspChain) {
            m_dspChain->setStream(m_audioEngine->streamHandle());
            m_dspChain->reapplyAll();
        }
    }
}

void MainWindow::onTrackFinished(const QString&)
{
    m_controlBar->setPlaying(false);
}

void MainWindow::onPositionChanged(qint64 posMs, qint64 durMs)
{
    m_controlBar->setPosition(posMs, durMs);
    m_lyricPanel->scrollToTime(posMs);
    if (m_spectrum->isVisible()) {
        m_spectrum->update();
    }
}

void MainWindow::onVolumeChanged(float level)
{
    m_audioEngine->setVolume(level);
    ConfigManager::instance()->setVolume(level);
}

void MainWindow::onPlayModeChanged(PlayMode mode)
{
    m_controlBar->setPlayMode(mode);
}

void MainWindow::onPlaylistDoubleClicked(int index)
{
    m_playlistMgr->playTrackAt(index);
}

void MainWindow::onOpenFiles()   { m_playlistMgr->openFiles(); }
void MainWindow::onOpenFolder()  { m_playlistMgr->openFolder(); }

void MainWindow::onLoadM3u()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Load Playlist", QString(),
        "Playlist Files (*.m3u *.m3u8);;All Files (*.*)");
    if (!path.isEmpty()) m_playlistMgr->loadM3u(path);
}

void MainWindow::onSaveM3u()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Playlist", "playlist.m3u",
        "M3U Playlist (*.m3u);;All Files (*.*)");
    if (!path.isEmpty()) m_playlistMgr->saveM3u(path);
}

void MainWindow::onSkinChanged(const QString&) {}

// ============================================================
// Window state
// ============================================================
void MainWindow::saveWindowState()
{
    auto* cfg = ConfigManager::instance();
    cfg->setWindowPosition(pos());
    cfg->setWindowSize(size());
}

void MainWindow::restoreWindowState()
{
    auto* cfg = ConfigManager::instance();
    QPoint savedPos = cfg->windowPosition();
    QSize  savedSize = cfg->windowSize();
    if (savedSize.width() > 0 && savedSize.height() > 0) {
        move(savedPos);
        resize(savedSize);
    }
}

void MainWindow::applyBackground()
{
    update();
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    auto* bgMgr = BackgroundManager::instance();
    if (bgMgr->hasCustomBackground()) {
        QPainter p(this);
        QPixmap bg = bgMgr->renderBackground(size());
        p.drawPixmap(rect(), bg);
    }
    QMainWindow::paintEvent(event);
}

// ============================================================
// Events
// ============================================================
void MainWindow::closeEvent(QCloseEvent* event)
{
    saveWindowState();
    if (m_audioEngine) {
        m_audioEngine->stop();
    }
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    applyBackground();

    // ── Dynamic scaling ─────────────────────────────
    applyScaleToChildren();

    // Keep overlay panels sized to the window client area
    if (m_playlistPanel) {
        m_playlistPanel->setGeometry(rect());
    }
    if (m_favPanel) {
        m_favPanel->setGeometry(rect());
    }
    if (m_dspPanel && m_dspPanel->isVisible()) {
        int x = (width()  - m_dspPanel->width())  / 2;
        int y = (height() - m_dspPanel->height()) / 2;
        m_dspPanel->move(x, y);
    }
}

// ═══════════════════════════════════════════════════════════
//  Dynamic scaling
// ═══════════════════════════════════════════════════════════

qreal MainWindow::scaleFactor() const
{
    // Baseline: smallest preset 800×450
    constexpr qreal kW = 800.0;
    constexpr qreal kH = 450.0;
    qreal s = qMin(width() / kW, height() / kH);
    return qBound(0.8, s, 2.0);
}

void MainWindow::applyScaleToChildren()
{
    qreal newScale = scaleFactor();
    if (qFuzzyCompare(newScale, m_scale)) return;
    m_scale = newScale;

    // Scale fixed-height custom widgets
    if (m_controlBar) {
        int barH = qRound(80.0 * m_scale);
        m_controlBar->setFixedHeight(barH);
        m_controlBar->setScale(m_scale);
    }
    if (m_trackInfo) {
        int infoH = qRound(56.0 * m_scale);
        m_trackInfo->setFixedHeight(infoH);
        m_trackInfo->setScale(m_scale);
    }
}

} // namespace WaveRider

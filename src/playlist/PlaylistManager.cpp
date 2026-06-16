#include "playlist/PlaylistManager.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlayModeEngine.h"
#include "audio/AudioEngine.h"
#include "core/SignalBus.h"
#include "core/ConfigManager.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

namespace WaveRider {

PlaylistManager::PlaylistManager(PlaylistModel* model, AudioEngine* engine, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_audioEngine(engine)
    , m_modeEngine(new PlayModeEngine(this))
{
    // Restore last play mode
    m_modeEngine->setMode(ConfigManager::instance()->playMode());

    // When a track finishes, play the next one
    connect(m_audioEngine, &AudioEngine::trackFinished, this, [this](const QString&) {
        int next = m_modeEngine->nextTrack(
            m_model->currentIndex(),
            m_model->trackCount()
        );
        if (next >= 0 && next < m_model->trackCount()) {
            playTrackAt(next);
        }
    });
}

// ── File operations ─────────────────────────────────────

void PlaylistManager::openFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        nullptr, "Open Audio Files", QString(),
        "Audio Files (*.mp3 *.wav *.flac *.aac *.ogg *.m4a *.wma *.ape *.wv);;"
        "MP3 Files (*.mp3);;FLAC Files (*.flac);;All Files (*.*)"
    );
    if (!files.isEmpty()) {
        addFiles(files);
    }
}

void PlaylistManager::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        nullptr, "Open Folder", QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (dir.isEmpty()) return;

    QDir qdir(dir);
    QStringList filters = {"*.mp3", "*.wav", "*.flac", "*.aac", "*.ogg", "*.m4a", "*.wma", "*.ape", "*.wv"};
    QStringList files;
    for (const auto& entry : qdir.entryInfoList(filters, QDir::Files)) {
        files.append(entry.absoluteFilePath());
    }
    if (!files.isEmpty()) {
        addFiles(files);
    }
}

void PlaylistManager::addFiles(const QStringList& paths)
{
    m_model->appendTracks(paths);
}

void PlaylistManager::removeSelected(const QList<int>& indices)
{
    m_model->removeTracks(indices);
}

void PlaylistManager::clearPlaylist()
{
    m_audioEngine->stop();
    m_model->clear();
}

// ── M3U I/O ────────────────────────────────────────────

bool PlaylistManager::loadM3u(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    m_model->clear();

    QTextStream in(&file);
    QStringList paths;
    QDir baseDir = QFileInfo(filePath).absoluteDir();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // Handle relative paths
        QFileInfo fi(line);
        if (fi.isRelative()) {
            line = baseDir.absoluteFilePath(line);
        }

        if (QFileInfo::exists(line)) {
            paths.append(line);
        }
    }

    file.close();

    if (!paths.isEmpty()) {
        m_model->appendTracks(paths);
        m_lastM3uPath = filePath;
        ConfigManager::instance()->setLastPlaylistPath(filePath);
        return true;
    }
    return false;
}

bool PlaylistManager::saveM3u(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "#EXTM3U\n";
    for (int i = 0; i < m_model->trackCount(); ++i) {
        auto meta = m_model->metadataAt(i);
        out << QString("#EXTINF:%1,%2 - %3\n")
                   .arg(meta.durationMs / 1000)
                   .arg(meta.artist.isEmpty() ? "Unknown" : meta.artist,
                        meta.title.isEmpty() ? "Unknown" : meta.title);
        out << m_model->filePathAt(i) << "\n";
    }

    file.close();
    m_lastM3uPath = filePath;
    ConfigManager::instance()->setLastPlaylistPath(filePath);
    return true;
}

// ── Play mode ──────────────────────────────────────────

PlayMode PlaylistManager::playMode() const
{
    return m_modeEngine->mode();
}

void PlaylistManager::setPlayMode(PlayMode mode)
{
    m_modeEngine->setMode(mode);
    ConfigManager::instance()->setPlayMode(mode);
    emit playModeChanged(mode);
    emit SignalBus::instance()->playModeChanged(mode);
}

void PlaylistManager::cyclePlayMode()
{
    PlayMode current = m_modeEngine->mode();
    PlayMode next;
    switch (current) {
    case PlayMode::Sequential: next = PlayMode::LoopAll;   break;
    case PlayMode::LoopAll:    next = PlayMode::LoopOne;   break;
    case PlayMode::LoopOne:    next = PlayMode::Shuffle;   break;
    case PlayMode::Shuffle:    next = PlayMode::Sequential; break;
    }
    setPlayMode(next);
}

// ── Navigation ─────────────────────────────────────────

int PlaylistManager::nextTrackIndex() const
{
    return m_modeEngine->nextTrack(m_model->currentIndex(), m_model->trackCount());
}

int PlaylistManager::prevTrackIndex() const
{
    if (m_model->trackCount() == 0) return -1;
    return m_model->prevIndex(m_model->currentIndex());
}

void PlaylistManager::playTrackAt(int index)
{
    if (index < 0 || index >= m_model->trackCount()) return;

    QString filePath = m_model->filePathAt(index);
    if (m_audioEngine->loadFile(filePath)) {
        m_model->setCurrentIndex(index);
        m_audioEngine->play();
        ConfigManager::instance()->setLastTrackPath(filePath);
    }
}

void PlaylistManager::playNext()
{
    int next = nextTrackIndex();
    if (next >= 0) {
        playTrackAt(next);
    }
}

void PlaylistManager::playPrev()
{
    int prev = prevTrackIndex();
    if (prev >= 0) {
        playTrackAt(prev);
    }
}

} // namespace WaveRider

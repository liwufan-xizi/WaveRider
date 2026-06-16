#include "dsp/DSPPluginLoader.h"
#include "dsp/IDSPEffect.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>

namespace WaveRider {

// ═══════════════════════════════════════════════════════════
//  Construction / destruction
// ═══════════════════════════════════════════════════════════

DSPPluginLoader::DSPPluginLoader(QObject* parent)
    : QObject(parent)
{
}

DSPPluginLoader::~DSPPluginLoader()
{
    unloadAll();
}

void DSPPluginLoader::unloadAll()
{
    for (auto& state : m_loadedPlugins) {
        if (state.library) {
            state.library->unload();
            delete state.library;
            state.library = nullptr;
        }
    }
    m_loadedPlugins.clear();
    m_libraries.clear();
}

// ═══════════════════════════════════════════════════════════
//  Manifest parsing
// ═══════════════════════════════════════════════════════════

bool DSPPluginLoader::parseManifest(const QString& jsonPath, DSPPluginInfo& info) const
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "DSPPluginLoader: cannot open manifest:" << jsonPath;
        return false;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    file.close();

    if (parseErr.error != QJsonParseError::NoError) {
        qWarning() << "DSPPluginLoader: JSON parse error in" << jsonPath
                    << ":" << parseErr.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    QString id = root.value("id").toString();
    if (id.isEmpty()) {
        qWarning() << "DSPPluginLoader: manifest missing required 'id' field:" << jsonPath;
        return false;
    }

    info.effectId    = id;
    info.displayName = root.value("displayName").toString(id);
    info.version     = root.value("version").toString("1.0.0");
    info.author      = root.value("author").toString();
    info.description = root.value("description").toString();
    info.manifestPath = jsonPath;

    return true;
}

// ═══════════════════════════════════════════════════════════
//  Factory validation
// ═══════════════════════════════════════════════════════════

bool DSPPluginLoader::validateFactory(const QString& dllPath) const
{
    QLibrary lib(dllPath);
    if (!lib.load()) {
        qWarning() << "DSPPluginLoader: failed to load DLL:" << dllPath
                    << "—" << lib.errorString();
        return false;
    }

    auto factory = reinterpret_cast<DspPluginFactory>(
        lib.resolve("waveRiderDspFactory"));
    lib.unload();

    if (!factory) {
        qWarning() << "DSPPluginLoader: DLL missing 'waveRiderDspFactory' export:"
                    << dllPath;
        return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════
//  Plugin discovery
// ═══════════════════════════════════════════════════════════

QStringList DSPPluginLoader::discoverPlugins(const QString& searchPath)
{
    m_plugins.clear();

    QDir dir(searchPath);
    if (!dir.exists()) {
        qDebug() << "DSPPluginLoader: search path does not exist:" << searchPath
                 << "— skipping plugin scan.";
        return {};
    }

    // Iterate .dll files in the directory
    const auto entries = dir.entryInfoList(QStringList() << "*.dll",
                                           QDir::Files | QDir::Readable);
    for (const auto& entry : entries) {
        QString dllPath = entry.absoluteFilePath();
        QString baseName = entry.completeBaseName();  // "MyEffect" (no .dll)

        // Look for a companion JSON manifest
        QString jsonPath = entry.absolutePath() + "/" + baseName + ".json";
        if (!QFile::exists(jsonPath)) {
            qDebug() << "DSPPluginLoader: skipping" << baseName
                     << "— no manifest found (expected" << jsonPath << ")";
            continue;
        }

        // Parse manifest
        DSPPluginInfo info;
        if (!parseManifest(jsonPath, info)) {
            emit pluginLoadFailed(baseName,
                                  "Invalid or missing manifest: " + jsonPath);
            continue;
        }

        // Validate that the DLL exports the factory symbol
        // (load-then-unload — just a lightweight check)
        if (!validateFactory(dllPath)) {
            emit pluginLoadFailed(info.effectId,
                                  "DLL missing factory export: " + dllPath);
            continue;
        }

        info.dllPath = dllPath;

        // Check for duplicate IDs
        if (m_plugins.contains(info.effectId)) {
            qWarning() << "DSPPluginLoader: duplicate effect ID '"
                       << info.effectId << "'— skipping second plugin at" << dllPath;
            continue;
        }

        m_plugins.insert(info.effectId, info);
        emit pluginDiscovered(info.effectId, info.displayName);
        qDebug() << "DSPPluginLoader: discovered plugin" << info.effectId
                 << "(" << info.displayName << "v" << info.version << ")";
    }

    return m_plugins.keys();
}

// ═══════════════════════════════════════════════════════════
//  Instantiation
// ═══════════════════════════════════════════════════════════

IDSPEffect* DSPPluginLoader::createPlugin(const QString& effectId)
{
    if (!m_plugins.contains(effectId)) {
        qWarning() << "DSPPluginLoader: unknown plugin ID:" << effectId;
        emit pluginLoadFailed(effectId, "Unknown plugin ID: " + effectId);
        return nullptr;
    }

    const DSPPluginInfo& info = m_plugins[effectId];

    // Load the library (kept alive in m_loadedPlugins)
    auto* lib = new QLibrary(info.dllPath);
    if (!lib->load()) {
        qWarning() << "DSPPluginLoader: failed to load plugin DLL:"
                    << info.dllPath << "—" << lib->errorString();
        emit pluginLoadFailed(effectId, lib->errorString());
        delete lib;
        return nullptr;
    }

    auto factory = reinterpret_cast<DspPluginFactory>(
        lib->resolve("waveRiderDspFactory"));
    if (!factory) {
        qWarning() << "DSPPluginLoader: factory symbol not found in"
                    << info.dllPath;
        emit pluginLoadFailed(effectId,
                              "Missing 'waveRiderDspFactory' export");
        lib->unload();
        delete lib;
        return nullptr;
    }

    // Call the factory to create the effect instance
    IDSPEffect* effect = factory();
    if (!effect) {
        qWarning() << "DSPPluginLoader: factory returned nullptr for" << effectId;
        emit pluginLoadFailed(effectId, "Factory returned nullptr");
        lib->unload();
        delete lib;
        return nullptr;
    }

    // Verify the effect reports the expected ID
    if (effect->effectId() != effectId) {
        qWarning() << "DSPPluginLoader: effect ID mismatch — expected"
                   << effectId << "but factory returned" << effect->effectId();
        // Don't fail on mismatch, just warn.  The manifest is canonical.
    }

    // Track the library so it stays loaded for the effect's lifetime
    PluginState state;
    state.library = lib;
    state.info    = info;
    m_loadedPlugins.append(state);
    m_libraries[effectId] = lib;

    qDebug() << "DSPPluginLoader: created plugin instance" << effectId;
    return effect;
}

// ═══════════════════════════════════════════════════════════
//  Accessors
// ═══════════════════════════════════════════════════════════

QStringList DSPPluginLoader::availablePlugins() const
{
    return m_plugins.keys();
}

const DSPPluginInfo* DSPPluginLoader::pluginInfo(const QString& effectId) const
{
    auto it = m_plugins.find(effectId);
    return (it != m_plugins.end()) ? &it.value() : nullptr;
}

} // namespace WaveRider

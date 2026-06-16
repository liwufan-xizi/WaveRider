#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QLibrary>

namespace WaveRider {

class IDSPEffect;

/// Metadata for a discovered external DSP plugin.
struct DSPPluginInfo {
    QString effectId;       // unique id, e.g. "my_reverb"
    QString displayName;    // human-readable, e.g. "My Reverb"
    QString version;
    QString author;
    QString description;
    QString dllPath;        // absolute path to the DLL
    QString manifestPath;   // absolute path to the JSON manifest
};

/// Plugin factory function signature.
/// Every external DSP DLL must export this function:
///   extern "C" __declspec(dllexport) IDSPEffect* waveRiderDspFactory();
using DspPluginFactory = IDSPEffect* (*)();

/// Scans a directory for external DSP effect plugin DLLs, parses
/// their JSON manifests, and instantiates effects via factory functions.
///
/// Plugin convention:
///   plugins/
///   ├── MyEffect.dll         — exports `waveRiderDspFactory`
///   ├── MyEffect.json        — { "id","displayName","version","author","description" }
///   ├── AnotherEffect.dll
///   └── AnotherEffect.json
///
/// Usage:
///   DSPPluginLoader loader;
///   loader.discoverPlugins("plugins/");
///   for (const auto& id : loader.availablePlugins()) {
///       IDSPEffect* fx = loader.createPlugin(id);
///       if (fx) dspChain->addEffect(fx);
///   }
class DSPPluginLoader : public QObject {
    Q_OBJECT
public:
    explicit DSPPluginLoader(QObject* parent = nullptr);
    ~DSPPluginLoader() override;

    /// Search directories for external effect plugins.
    /// Scans for .dll files, validates factory exports, parses .json manifests.
    /// Populates availablePlugins() and emits pluginDiscovered per valid plugin.
    /// @return list of effect IDs discovered.
    QStringList discoverPlugins(const QString& searchPath);

    /// Attempt to instantiate a plugin by its effect ID.
    /// Loads the DLL via QLibrary, resolves the factory, calls it.
    /// The caller owns the returned IDSPEffect*.
    /// @return a new IDSPEffect*, or nullptr on failure.
    IDSPEffect* createPlugin(const QString& effectId);

    /// List IDs of all plugins discovered during the last scan.
    QStringList availablePlugins() const;
    const DSPPluginInfo* pluginInfo(const QString& effectId) const;

signals:
    void pluginDiscovered(const QString& effectId, const QString& displayName);
    void pluginLoadFailed(const QString& effectId, const QString& error);

private:
    bool parseManifest(const QString& jsonPath, DSPPluginInfo& info) const;
    bool validateFactory(const QString& dllPath) const;
    void unloadAll();

    QMap<QString, DSPPluginInfo> m_plugins;      // effectId → metadata
    QMap<QString, QLibrary*>     m_libraries;     // effectId → loaded lib (for createPlugin)

    // Keep libraries alive for created plugins — keyed by effectId, one per instance.
    struct PluginState {
        QLibrary*   library = nullptr;
        DSPPluginInfo info;
    };
    QVector<PluginState> m_loadedPlugins;
};

} // namespace WaveRider

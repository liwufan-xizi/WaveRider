# src/CMakeLists.txt — WaveRider application sources
# Sources are added incrementally as each module is implemented.

set(SOURCES
    main.cpp
    # ── core ──
    core/SignalBus.cpp
    core/ConfigManager.cpp
    # ── audio ──
    audio/BASSInitGuard.cpp
    audio/AudioEngine.cpp
    audio/AudioMetadata.cpp
    # ── playlist (Day 2) ──
    playlist/PlaylistModel.cpp
    playlist/PlaylistManager.cpp
    playlist/PlayModeEngine.cpp
    # ── dsp (Day 3) ──
    dsp/IDSPEffect.cpp
    dsp/DSPChain.cpp
    dsp/DSPPluginLoader.cpp
    dsp/builtin/EqualizerEffect.cpp
    dsp/builtin/CompressorEffect.cpp
    dsp/builtin/ReverbEffect.cpp
    # ── skin (Day 4) ──
    skin/SkinManager.cpp
    skin/StyleSheetBuilder.cpp
    skin/BackgroundManager.cpp
    skin/ThemeConfig.cpp
    skin/ColorExtractor.cpp
    # ── lyrics (Day 5) ──
    lyrics/LyricParser.cpp
    lyrics/LyricFetcher.cpp
    lyrics/LyricModel.cpp
    # ── favorites (Day 5) ──
    favorites/FavoritesManager.cpp
    # ── ui (Days 2-6) ──
    ui/MainWindow.cpp
    ui/PlayerControlBar.cpp
    ui/PlaylistPanel.cpp
    ui/PlaylistDelegate.cpp
    ui/LyricPanel.cpp
    ui/VisualArea.cpp
    ui/EqualizerWidget.cpp
    ui/DSPPanel.cpp
    ui/FavoritesPanel.cpp
    ui/SpectrumWidget.cpp
    ui/SkinSelectorDialog.cpp
    ui/BackgroundDialog.cpp
    ui/TrackInfoWidget.cpp
    ui/widgets/SeekSlider.cpp
    ui/widgets/VolumeKnob.cpp
    ui/widgets/AnimatedButton.cpp
)

qt5_add_resources(RESOURCES ../resources/app.qrc)

add_executable(WaveRider ${SOURCES} ${RESOURCES})

target_include_directories(WaveRider PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor/bass
    ${BASS_INCLUDE_DIR}
)

target_link_libraries(WaveRider PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::Network
    ${BASS_LIBRARIES}
)

# Copy BASS DLLs to output directory (only if they exist)
foreach(_dll bass bass_fx bassflac bass_aac)
    set(_src "${CMAKE_SOURCE_DIR}/vendor/bass/${_dll}.dll")
    if(EXISTS "${_src}")
        add_custom_command(TARGET WaveRider POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_src}"
                "$<TARGET_FILE_DIR:WaveRider>"
            COMMENT "Copying ${_dll}.dll to output"
        )
    endif()
endforeach()

# Copy skins directory to output
add_custom_command(TARGET WaveRider POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/skins"
        "$<TARGET_FILE_DIR:WaveRider>/skins"
    COMMENT "Copying skins to output"
)

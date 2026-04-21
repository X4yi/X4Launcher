add_library(x4_ui STATIC
    Theme.cpp
    ConsoleWidget.cpp
    InstanceDelegate.cpp
    InstanceListWidget.cpp
    InstanceEditorDialog.cpp
    CreateInstanceDialog.cpp
    SettingsDialog.cpp
    JavaInstallerDialog.cpp
    ModrinthBrowserDialog.cpp
    AuthDialog.cpp
    KeyBindManager.cpp
    MainWindow.cpp
)

target_include_directories(x4_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_ui PUBLIC
    x4_core
    x4_network
    x4_modrinth
    x4_java
    x4_cache
    x4_instance
    x4_launch
    x4_auth
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

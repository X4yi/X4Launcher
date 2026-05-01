add_library(x4_core STATIC
        PathManager.cpp
        SettingsManager.cpp
        Logger.cpp
        GlobalEvents.h
        GlobalEvents.cpp
        Translator.cpp
        ErrorHandler.cpp
)

target_include_directories(x4_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_core PUBLIC Qt6::Core)

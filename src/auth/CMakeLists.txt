add_library(x4_auth STATIC
        AuthAccount.h
        AccountManager.cpp
        AccountManager.h
        AuthController.cpp
        AuthController.h
        LoopbackServer.cpp
        LoopbackServer.h
        MicrosoftOAuthClient.cpp
        MicrosoftOAuthClient.h
        MinecraftAuthClient.cpp
        MinecraftAuthClient.h
        OAuthSession.cpp
        OAuthSession.h
        PkceGenerator.cpp
        PkceGenerator.h
        TokenStore.cpp
        TokenStore.h
        WindowsTokenStore.cpp
        WindowsTokenStore.h
        LinuxTokenStore.cpp
        LinuxTokenStore.h
        XboxLiveClient.cpp
        XboxLiveClient.h
        XSTSClient.cpp
        XSTSClient.h
)

target_include_directories(x4_auth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_auth PUBLIC Qt6::Core Qt6::Network Qt6::Gui)

if (WIN32)
    target_link_libraries(x4_auth PRIVATE advapi32)
elseif (UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSECRET REQUIRED libsecret-1)
    target_include_directories(x4_auth PRIVATE ${LIBSECRET_INCLUDE_DIRS})
    target_link_libraries(x4_auth PRIVATE ${LIBSECRET_LIBRARIES})
    target_compile_options(x4_auth PRIVATE ${LIBSECRET_CFLAGS_OTHER})
endif ()

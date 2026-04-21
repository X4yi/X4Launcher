if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(/O1 /GL /Gy /GS-)
        add_link_options(/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO)
        add_compile_definitions(NDEBUG QT_NO_DEBUG)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(
            -Os
            -flto=auto
            -ffunction-sections
            -fdata-sections
            -fvisibility=hidden
            -fvisibility-inlines-hidden
            -fno-plt
            -fmerge-all-constants
        )
        add_link_options(
            -flto=auto
            -Wl,--gc-sections
            -Wl,-s
            -Wl,--as-needed
            -Wl,--no-undefined
        )
        add_compile_definitions(NDEBUG QT_NO_DEBUG)
    endif()
endif()

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
endif()

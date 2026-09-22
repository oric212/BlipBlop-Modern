find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)

aux_source_directory(. SRC)
aux_source_directory(menus MENUS_SRC)

if(WIN32)
    add_executable(blipblop WIN32 ${SRC} ${MENUS_SRC})
else()
    add_executable(blipblop ${SRC} ${MENUS_SRC})
endif()

target_compile_features(blipblop PRIVATE cxx_std_17)
target_link_libraries(blipblop PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer)

# The source deliberately exposes a normal main() rather than SDL_main.
# Keep the Windows GUI subsystem while using the standard C runtime entry point.
if(MSVC)
    target_link_options(blipblop PRIVATE "/ENTRY:mainCRTStartup")
endif()

install(DIRECTORY ./data DESTINATION .)
install(TARGETS blipblop RUNTIME DESTINATION .)

set(CPACK_PACKAGE_NAME "BlipnBlop")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Blip & Blop revamped")
set(CPACK_PACKAGE_VENDOR "Loaded Studios")
set(CPACK_DEBIAN_PACKAGE_NAME "blipnblop")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libSDL2, libSD2-mixer")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Blip & Blop revamped")
set(CPACK_PACKAGE_EXECUTABLES "blipblop" "BlipnBlop")

if(WIN32)
    set(CPACK_NSIS_DISPLAY_NAME "Blip & Blop")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
endif()

include(CPack)


find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)

aux_source_directory(. SRC)
aux_source_directory(menus MENUS_SRC)

if(WIN32)
    add_executable(blipblop WIN32
        ${SRC}
        ${MENUS_SRC}
        resources/windows/BlipnBlop.rc
    )
else()
    add_executable(blipblop ${SRC} ${MENUS_SRC})
endif()

target_compile_features(blipblop PRIVATE cxx_std_17)
target_link_libraries(blipblop PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer)
set_target_properties(blipblop PROPERTIES OUTPUT_NAME "BlipnBlop")

# The source deliberately exposes a normal main() rather than SDL_main.
# Keep the Windows GUI subsystem while using the standard C runtime entry point.
if(MSVC)
    target_link_options(blipblop PRIVATE "/ENTRY:mainCRTStartup")
endif()

install(DIRECTORY ./data DESTINATION .)
install(TARGETS blipblop RUNTIME DESTINATION .)

if(MSVC)
    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
    include(InstallRequiredSystemLibraries)
    foreach(BLIPBLOP_RUNTIME_FILE IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        get_filename_component(BLIPBLOP_RUNTIME_NAME
            "${BLIPBLOP_RUNTIME_FILE}" NAME)
        if(BLIPBLOP_RUNTIME_NAME MATCHES
            "^(msvcp140|vcruntime140|vcruntime140_1)\\.dll$")
            list(APPEND BLIPBLOP_MSVC_RUNTIME_LIBS "${BLIPBLOP_RUNTIME_FILE}")
        endif()
    endforeach()
endif()

set(BLIPBLOP_DEPLOY_DIRECTORY "${CMAKE_SOURCE_DIR}/game-build")
add_custom_target(deploy
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${BLIPBLOP_DEPLOY_DIRECTORY}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BLIPBLOP_DEPLOY_DIRECTORY}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:blipblop>" "${BLIPBLOP_DEPLOY_DIRECTORY}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:blipblop> "${BLIPBLOP_DEPLOY_DIRECTORY}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BLIPBLOP_MSVC_RUNTIME_LIBS} "${BLIPBLOP_DEPLOY_DIRECTORY}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "${BLIPBLOP_DEPLOY_DIRECTORY}/data"
    COMMAND_EXPAND_LISTS
    DEPENDS blipblop
    COMMENT "Creating standalone runtime directory at ${BLIPBLOP_DEPLOY_DIRECTORY}"
)

if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
    set(BLIPBLOP_VCPKG_RUNTIME_DIRECTORY
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    add_custom_command(TARGET deploy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${BLIPBLOP_VCPKG_RUNTIME_DIRECTORY}/vorbisfile.dll"
            "${BLIPBLOP_VCPKG_RUNTIME_DIRECTORY}/vorbis.dll"
            "${BLIPBLOP_VCPKG_RUNTIME_DIRECTORY}/ogg.dll"
            "${BLIPBLOP_VCPKG_RUNTIME_DIRECTORY}/mpg123.dll"
            "${BLIPBLOP_VCPKG_RUNTIME_DIRECTORY}/wavpackdll.dll"
            "${BLIPBLOP_DEPLOY_DIRECTORY}/"
        COMMENT "Copying SDL2_mixer codec runtime dependencies"
    )
endif()

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


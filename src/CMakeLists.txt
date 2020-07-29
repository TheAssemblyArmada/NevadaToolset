add_library(common STATIC
    common/b64pipe.cpp
    common/b64straw.cpp
    common/base64.cpp
    common/basefile.cpp
    common/blowfish.cpp
    common/blowpipe.cpp
    common/blowstraw.cpp
    common/buffer.cpp
    common/buffpipe.cpp
    common/buffstraw.cpp
    common/crc.cpp
    common/cstraw.cpp
    common/dipthong.cpp
    common/filepipe.cpp
    common/filestraw.cpp
    common/ini.cpp
    common/mpmath.cpp
    common/pipe.cpp
    common/pk.cpp
    common/pkpipe.cpp
    common/pkstraw.cpp
    common/ramfile.cpp
    common/random.cpp
    common/rawfile.cpp
    common/readline.cpp
    common/rndstraw.cpp
    common/sha.cpp
    common/shapipe.cpp
    common/shastraw.cpp
    common/straw.cpp
)

target_link_libraries(common PUBLIC base captnlog)
target_include_directories(common PUBLIC common)

# String table convertor program
add_library(ini2str STATIC ini2str/ini2str.cpp ini2str/strini.cpp)
target_link_libraries(ini2str PUBLIC common)

add_executable(strgen ini2str/strgen.cpp)
target_link_libraries(strgen ini2str)

if(wxWidgets_FOUND)
    if(NOT wxWidgets_wxrc_EXECUTABLE)
        message(FATAL_ERROR "wxrc resource compiler was not found, please set wxWidgets_wxrc_EXECUTABLE manually.")
    endif()

    # WX GUI version of the string table convertor.
    # Generate cpp/h files to embed xml resources in the binary.
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/wxstrgenui.cpp"
        COMMAND ${wxWidgets_wxrc_EXECUTABLE} --cpp-code --extra-cpp-code --output=${CMAKE_CURRENT_BINARY_DIR}/wxstrgenui.cpp ${CMAKE_CURRENT_SOURCE_DIR}/ini2str/wxstrgenui.xrc
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ini2str/wxstrgenui.xrc"
    )

    add_executable(wxstrgen ini2str/wxstrgen.cpp ${CMAKE_CURRENT_BINARY_DIR}/wxstrgenui.cpp)
    target_link_libraries(wxstrgen ${wxWidgets_LIBRARIES} ini2str)
    target_include_directories(wxstrgen PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${wxWidgets_INCLUDE_DIRS})
    target_compile_options(wxstrgen PRIVATE ${wxWidgets_CXX_FLAGS})
    target_compile_definitions(wxstrgen PRIVATE ${wxWidgets_DEFINITIONS})

    if(MSVC)
        set_target_properties(wxstrgen PROPERTIES LINK_FLAGS "/subsystem:windows /ENTRY:WinMainCRTStartup")
    endif()
endif()

# Mix creator program
add_executable(makemix makemix/makemix.cpp)
target_link_libraries(makemix PUBLIC common)

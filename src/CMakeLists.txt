add_library(common
    common/b64pipe.cpp
    common/b64straw.cpp
    common/base64.cpp
    common/basefile.cpp
    common/buffer.cpp
    common/buffpipe.cpp
    common/buffstraw.cpp
    common/crc.cpp
    common/dipthong.cpp
    common/filepipe.cpp
    common/filestraw.cpp
    common/ini.cpp
    common/pipe.cpp
    common/ramfile.cpp
    common/rawfile.cpp
    common/readline.cpp
    common/straw.cpp
)

target_link_libraries(common PUBLIC base captnlog)
target_include_directories(common PUBLIC common)

add_executable(ini2str ini2str/ini2str.cpp ini2str/strini.cpp)
target_link_libraries(ini2str common)

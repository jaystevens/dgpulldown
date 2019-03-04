# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.10)

# do not let cmake cache run in source folder
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
        "In-source builds are not permitted. Make a separate folder for building:\n"
        "mkdir build; cd build; cmake ..\n"
        "Before that, remove the files already created:\n"
        "rm -rf CMakeCache.txt CMakeFiles")
endif()
# do not let cmake cache run in src/ folder
if("${CMAKE_SOURCE_DIR}/src" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
            "In-source builds are not permitted. Make a separate folder for building:\n"
            "mkdir build; cd build; cmake ..\n"
            "Before that, remove the files already created:\n"
            "rm -rf CMakeCache.txt CMakeFiles")
endif()

# if no build type specified default to release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release"
            CACHE "Build type: Debug, Release, RelWithDebInfo or MinSizeRel" STRING
            FORCE)
endif()

# append cmake/modules to MAKE_MODULE_PATH variable
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

project("dgpulldown" LANGUAGES C)

# setup version number
include(GetGitRevisionDescription)
git_describe(VERSION)
if(VERSION MATCHES "-128-NOTFOUND")
    set(VERSION "v0.0-0-gmissing")
endif()
if (VERSION MATCHES "HEAD-HASH-NOTFOUND")
    set(VERSION "v0.0-0-gmissing")
endif()
message("git version   : ${VERSION}")
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" DGPULLDOWN_VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" DGPULLDOWN_VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\-([0-9]+)-.*" "\\1" DGPULLDOWN_VERSION_PATCH "${VERSION}")
set(DGPULLDOWN_VERSION_TWEAK 0)
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\-[0-9]+-(.*)" "\\1" DGPULLDOWN_VERSION_SHA1 "${VERSION}")
set(DGPULLDOWN_VERSION "${DGPULLDOWN_VERSION_MAJOR}.${DGPULLDOWN_VERSION_MINOR}.${DGPULLDOWN_VERSION_PATCH}.${DGPULLDOWN_VERSION_TWEAK}")
set(DGPULLDOWN_VERSION_FULL "${DGPULLDOWN_VERSION_MAJOR}.${DGPULLDOWN_VERSION_MINOR}.${DGPULLDOWN_VERSION_PATCH}.${DGPULLDOWN_VERSION_TWEAK} ${DGPULLDOWN_VERSION_SHA1}")
message("dgpulldown version:  ${DGPULLDOWN_VERSION}")
message("dgpulldown version:  ${DGPULLDOWN_VERSION_FULL}")
cmake_host_system_information(RESULT DGPULLDOWN_BUILD_HOST QUERY HOSTNAME)

# setup version number in application
configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/src/dgpulldown_version.c.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/dgpulldown_version.c"
)

# Visual Studio project generator x64 check
if(MSVC)
    if(CMAKE_GENERATOR MATCHES ".*Visual Studio.*")
        if(NOT CMAKE_GENERATOR_PLATFORM MATCHES "x64")
            message(FATAL_ERROR "\nVisual Studio x86 builds are not supported\nrebuild cmake cache with '-A x64'")
        endif()
    endif()
endif()

# compiler flags
include(compiler_flags)
if(MSVC)
    add_definitions(-DUNICODE -D_UNICODE) # Visual Studio - force unicode over multi-byte
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)  # Visual Studio - turn off use secure functions
    # Disable MSVC warnings that suggest making code non-portable.
    #add_compiler_flag_if_supported("/wd4996")
    add_definitions("/favor:INTEL64")  # Visual Studio - favor intel cpu, only works with 64 bit
else()
    # require_c_flag("-std=c11" YES)
    # these are from my ffmpeg build (fftools/config.mak)
    add_c_flag_if_supported("-fomit-frame-pointer")
    add_c_flag_if_supported("-fPIE")
    # compiler warnings
    add_c_flag_if_supported("-Wall")
    add_c_flag_if_supported("-Wextra")
    add_c_flag_if_supported("-Wdeclaration-after-statement")
    add_c_flag_if_supported("-Wdisabled-optimization")
    add_c_flag_if_supported("-Wpointer-arith")
    add_c_flag_if_supported("-Wredundant-decls")
    add_c_flag_if_supported("-Wwrite-strings")
    add_c_flag_if_supported("-Wtype-limits")
    add_c_flag_if_supported("-Wundef")
    add_c_flag_if_supported("-Wmissing-prototypes")
    add_c_flag_if_supported("-Wno-pointer-to-int-cast")
    add_c_flag_if_supported("-Wstrict-prototypes")
    add_c_flag_if_supported("-Wempty-body")
    add_c_flag_if_supported("-Wno-parentheses")
    add_c_flag_if_supported("-Wno-switch")
    add_c_flag_if_supported("-Wno-format-zero-length")
    add_c_flag_if_supported("-Wno-pointer-sign")
    add_c_flag_if_supported("-fno-math-errno")
    add_c_flag_if_supported("-fno-signed-zeros")
    add_c_flag_if_supported("-fno-tree-vectorize")
    add_c_flag_if_supported("-Werror=format-security")
    add_c_flag_if_supported("-Werror=implicit-function-declaration")
    add_c_flag_if_supported("-Werror=missing-prototypes")
    add_c_flag_if_supported("-Werror=return-types")
    # add direct, does not support test?
    append_c_flag("-Werror=vla")
    add_c_flag_if_supported("-Wformat")
    add_c_flag_if_supported("-fdiagnostics-color=auto")
    add_c_flag_if_supported("-Wno-maybe-uninitialized")
    # fixup
    #add_c_flag_if_supported("-Wno-undef")
    #add_c_flag_if_supported("-Wno-unknown-pragmas")

    # build staic
    if(NOT APPLE)
        add_c_flag_if_supported("-static")
        add_c_flag_if_supported("-static-libgcc")
    endif()
endif()


#if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
#    if (MSVC)
#        # Maximum optimization in Release mode.
#        add_compile_options("$<$<CONFIG:RELEASE>:/Ox>")
#    else ()
#        add_c_flag_if_supported("-O3")
#    endif ()
#endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DGPULLDOWN_C_FLAGS}")

# setup runtime folder
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(dgpulldown  "src/dgpulldown.c" "src/dgpulldown_version.c" "src/argparse.c" "src/av_log.c")
set_target_properties(dgpulldown PROPERTIES C_STANDARD 11)
set_target_properties(dgpulldown PROPERTIES C_STANDARD_REQUIRED ON)
set_target_properties(dgpulldown PROPERTIES C_EXTENSIONS ON)


# Configure CPack
set(CPACK_PACKAGE_NAME "dgpulldown")
set(CPACK_PACKAGE_VENDOR "TheFoundation")
#set(CPACK_PACKAGE_DIRECTORY "")
set(CPACK_PACKAGE_VERSION_MAJOR ${DGPULLDOWN_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DGPULLDOWN_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DGPULLDOWN_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION "Mpeg2 Pulldown Tool")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "it fixes stuff")
set(CPACK_PACKAGE_HOMEPAGE_URL "")
#set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "dgpulldown")  # for installer
#set(CPACK_PACKAGE_ICON "")
set(CPACK_PACKAGE_CHECKSUM "SHA256") # MD5/SHA1/SHA224/SHA256/SHA384/SHA512/SHA3_224/SHA3_256/SHA3_384/SHA3_512
#set(CPACK_RESOURCE_FILE_LICENSE "")  # used by installer
#set(CPACK_RESOURCE_FILE_README "")
#set(CPACK_RESOURCE_FILE_WELCOME "")
set(CPACK_MONOLITHIC_INSTALL TRUE)
# cpack output, see cpack --help
if(UNIX)
    set(CPACK_GENERATOR TXZ)
else()
    set(CPACK_GENERATOR ZIP)
endif()
#set(CPACK_PACKAGE_EXECUTABLES "fkoder;FKoder")  # start menu shortcuts
set(CPACK_STRIP_FILES TRUE)  # strip all files

# Source pack
#set(CPACK_SOURCE_PACKAGE_FILE_NAME "fkoder-")
#set(CPACK_SOURCE_STRIP_FILES FALSE)
#set(CPACK_SOURCE_GENERATOR TXZ)
#set(CPACK_SOURCE_IGNORE_FILES "/\\.idea/;/\\.vs/;;")

# Adv
set(CPACK_PACKAGE_VERSION ${DGPULLDOWN_VERSION})
include(CPack)

#message("Supported features = ${CMAKE_CXX_COMPILE_FEATURES}")
#message("Supported features = ${CMAKE_C_COMPILE_FEATURES}")

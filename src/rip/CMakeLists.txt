set(AppleClang_MinVersion 11.0.0)
set(Clang_MinVersion 11.0.0)
set(GNU_MinVersion 10.0.0)
set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fcoroutines-ts)
    message(NOTICE "rip: using clang, enable corresponding coroutine support")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-fcoroutines)
    message(NOTICE "rip: using gcc, enable corresponding coroutine support")
else ()
    message(SEND_ERROR "rip: unsupported c++ compiler, requires either gcc or clang with c++20 coroutines support")
endif ()

build_lib(
        LIBNAME rip
        IGNORE_PCH
        SOURCE_FILES
            model/rip-header-CoDES.cpp
            model/rip-CoDES.cpp
        HEADER_FILES
            model/rip-header-CoDES.h
            model/rip-CoDES.h
        LIBRARIES_TO_LINK
            ${libcore}
            ${libnetwork}
            ${libinternet}
            ${libcoroutine}
)

add_executable(
        rip-test test/test.cpp
)

target_link_libraries(
        rip-test
        ${librip}
        ${libcore}
        ${libnetwork}
        ${libinternet}
        ${libcoroutine}
)
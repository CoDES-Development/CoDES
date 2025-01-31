set(AppleClang_MinVersion 11.0.0)
set(Clang_MinVersion 11.0.0)
set(GNU_MinVersion 10.0.0)
set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fcoroutines-ts)
    message(NOTICE "rdma: using clang, enable corresponding coroutine support")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-fcoroutines)
    message(NOTICE "rdma: using gcc, enable corresponding coroutine support")
else ()
    message(SEND_ERROR "rdma: unsupported c++ compiler, requires either gcc or clang with c++20 coroutines support")
endif ()


build_lib(
        LIBNAME rdma
        IGNORE_PCH
        SOURCE_FILES
            model/infiniband-transport-header.cpp
            model/pfc-header.cpp
            model/pfc-queue-item.cpp
            model/pfc-tag.cpp
            model/rdma-device-driver.cpp
            model/rdma-node-driver.cpp
            model/rdma-utility.cpp
        HEADER_FILES
            model/infiniband-transport-header.h
            model/pfc-header.h
            model/pfc-queue-item.h
            model/pfc-tag.h
            model/rdma-device-driver.h
            model/rdma-node-driver.h
            model/rdma-utility.h
        LIBRARIES_TO_LINK
            ${libcore}
            ${libnetwork}
            ${libinternet}
            ${libcoroutine}
)
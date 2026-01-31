# Build scratch with MPI support
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

build_exec(
  EXECNAME demo-simple-distributed
  SOURCE_FILES simple-distributed.cc
               mpi-test-fixtures.cc
  LIBRARIES_TO_LINK
    ${libmpi}
    ${libpoint-to-point}
    ${libinternet}
    ${libnix-vector-routing}
    ${libapplications}
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/parallel-simulation-demo/
)

build_exec(
  EXECNAME demo-codes-simple-distributed
  SOURCE_FILES simple-distributed-codes.cc
               mpi-test-fixtures.cc
               ./codes-application/data-stream-application.cpp
               ./codes-application/data-stream-application-wrapper.cpp
  LIBRARIES_TO_LINK
    ${libmpi}
    ${libpoint-to-point}
    ${libinternet}
    ${libnix-vector-routing}
    ${libapplications}
    ${libcoroutine}
    ${libmpi-application}
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/parallel-simulation-demo/
)

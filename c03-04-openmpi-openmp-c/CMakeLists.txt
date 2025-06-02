cmake_minimum_required(VERSION 3.10)
project(MPIEncryption CXX)

find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(executable_mpi main.cpp)
target_link_libraries(executable_mpi PRIVATE 
    MPI::MPI_CXX 
    OpenMP::OpenMP_CXX
    OpenSSL::SSL
    OpenSSL::Crypto
)

set_property(TARGET executable_mpi PROPERTY CXX_STANDARD 17)
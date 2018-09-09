cmake_minimum_required(VERSION 3.10)
project(EE450_Socket C)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES
        aws.c
        serverA.c
        serverB.c
        serverC.c
        monitor.c
        client.c
        )
add_executable(EE450_Socket ${SOURCE_FILES})
add_executable(client client.c)
add_executable(monitor monitor.c)
add_executable(serverA serverA.c)
add_executable(serverB serverB.c)
add_executable(serverC serverC.c)
add_executable(aws aws.c)
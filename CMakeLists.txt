cmake_minimum_required(VERSION 2.8)

# Remove MinSizeRel and RelWithDebInfo targets
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE )

project(algo)

# Mysterious macro to remove default VS "Source Files" and "Header Files" filters.
MACRO (cmp_IDE_SOURCE_PROPERTIES SOURCE_PATH HEADERS SOURCES)
    STRING(REPLACE "/" "\\\\" source_group_path ${SOURCE_PATH}  )
    source_group(${source_group_path} FILES ${HEADERS} ${SOURCES})
ENDMACRO (cmp_IDE_SOURCE_PROPERTIES NAME HEADERS SOURCES INSTALL_FILES)

SET(headers algo.h platform.h test_common.h)

IF(MSVC)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /we4013 /we4020 /wd4996")
    SET(libs)
ELSEIF(UNIX)
	set(CMAKE_BUILD_TYPE Debug)
    #set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-std=c90 -march=native")
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-march=native")
    SET(libs m)
ENDIF()

set(exe_name test_allocpool)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

set(exe_name test_graph)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

set(exe_name test_hamming)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

set(exe_name test_heap)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

set(exe_name test_queue)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

set(exe_name test_stack)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

set(exe_name test_tictactoe)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

set(exe_name test_topo)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
target_link_libraries(${exe_name} ${libs})

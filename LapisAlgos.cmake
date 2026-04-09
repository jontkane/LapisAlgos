include_guard(GLOBAL)

set(LAPISGISCMAKE_PATH "${LAPIS_DIR}/src/gis/LapisGis.cmake" CACHE PATH "Path to LapisGis.cmake")
include(${LAPISGISCMAKE_PATH})

file(GLOB LAPISALGOS_SOURCES
	${LAPISALGOS_DIR}/src/*.hpp
	${LAPISALGOS_DIR}/src/*.cpp)

add_library(LapisAlgos STATIC ${LAPISALGOS_SOURCES})

set(LAPISALGOS_EXTERNAL_INCLUDES
	${LAPISGIS_INCLUDES}
	)

set(LAPISALGOS_EXTERNAL_LINKS
	${LAPISGIS_LINKS}
	)

target_include_directories(LapisAlgos PUBLIC ${LAPISALGOS_EXTERNAL_INCLUDES})
target_link_libraries(LapisAlgos PUBLIC ${LAPISALGOS_EXTERNAL_LINKS})
target_precompile_headers(LapisAlgos PRIVATE ${LAPISALGOS_DIR}/src/algos_pch.hpp)

set(LAPISALGOS_INCLUDES
	${LAPISALGOS_EXTERNAL_INCLUDES}
	${LAPISALGOS_DIR}/src
	)
set(LAPISALGOS_LINKS
	${LAPISALGOS_EXTERNAL_LINKS}
	LapisAlgos
	)

if (MSVC)
	target_compile_options(LapisAlgos PRIVATE /W3 /WX)
else()
	target_compile_options(LapisAlgos PRIVATE -Wall -Wextra -Werror)
endif()

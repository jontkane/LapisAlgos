file(GLOB LAPISALGOS_TEST_SOURCES
	${LAPISALGOS_DIR}/src/test/*.hpp
	${LAPISALGOS_DIR}/src/test/*.cpp)
	
add_compile_definitions(LAPISALGOSTESTFILES="${LAPISALGOS_DIR}/src/test/testfiles/")
add_executable(LapisAlgos_test ${LAPISALGOS_TEST_SOURCES})
find_package(GTest REQUIRED)
target_include_directories(LapisAlgos_test PRIVATE ${LAPISALGOS_EXTERNAL_INCLUDES})
target_link_libraries(LapisAlgos_test PRIVATE ${LAPISALGOS_EXTERNAL_LINKS})
target_link_libraries(LapisAlgos_test PRIVATE LapisAlgos)
target_include_directories(LapisAlgos_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(LapisAlgos_test PRIVATE ${GTEST_BOTH_LIBRARIES})
copy_proj_db_after_build(LapisAlgos_test)

if (MSVC)
	target_compile_options(LapisAlgos_test PRIVATE /W3 /WX)
else()
	target_compile_options(LapisAlgos_test PRIVATE -Wall -Wextra -Werror)
endif()

# all dependencies that are not directly included in the cpp-ethereum distribution are defined here
# for this to work, download the dependency via the cmake script in extdep or install them manually!

function(eth_show_dependency DEP NAME)
	get_property(DISPLAYED GLOBAL PROPERTY ETH_${DEP}_DISPLAYED)
	if (NOT DISPLAYED)
		set_property(GLOBAL PROPERTY ETH_${DEP}_DISPLAYED TRUE)
		message(STATUS "${NAME} headers: ${${DEP}_INCLUDE_DIRS}")
		message(STATUS "${NAME} lib   : ${${DEP}_LIBRARIES}")
		if (NOT("${${DEP}_DLLS}" STREQUAL ""))
			message(STATUS "${NAME} dll   : ${${DEP}_DLLS}")
		endif()
	endif()
endfunction()

if (DEFINED MSVC)
	# by defining CMAKE_PREFIX_PATH variable, cmake will look for dependencies first in our own repository before looking in system paths like /usr/local/ ...
	# this must be set to point to the same directory as $ETH_DEPENDENCY_INSTALL_DIR in /extdep directory

	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0.0)
		set (ETH_DEPENDENCY_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/../extdep/install/windows/x64")
	else()
		get_filename_component(DEPS_DIR "${CMAKE_CURRENT_LIST_DIR}/../deps/install" ABSOLUTE)
		set(ETH_DEPENDENCY_INSTALL_DIR
			"${DEPS_DIR}/x64"					# Old location for deps.
			"${DEPS_DIR}/win64"					# New location for deps.
			"${DEPS_DIR}/win64/Release/share"	# LLVM shared cmake files.
		)
	endif()
	set (CMAKE_PREFIX_PATH ${ETH_DEPENDENCY_INSTALL_DIR} ${CMAKE_PREFIX_PATH})
endif()

# custom cmake scripts
set(ETH_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(ETH_SCRIPTS_DIR ${ETH_CMAKE_DIR}/scripts)

find_program(CTEST_COMMAND ctest)

#message(STATUS "CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}")
#message(STATUS "CMake Helper Path: ${ETH_CMAKE_DIR}")
#message(STATUS "CMake Script Path: ${ETH_SCRIPTS_DIR}")
#message(STATUS "ctest path: ${CTEST_COMMAND}")

## use multithreaded boost libraries, with -mt suffix
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.54.0 QUIET REQUIRED COMPONENTS regex filesystem unit_test_framework program_options)

eth_show_dependency(Boost boost)

if (APPLE)
	link_directories(/usr/local/lib)
	include_directories(/usr/local/include)
endif()

include_directories(BEFORE "${PROJECT_BINARY_DIR}/include")

function(eth_use TARGET REQUIRED)
	if (NOT TARGET ${TARGET})
		message(FATAL_ERROR "eth_use called for non existing target ${TARGET}")
	endif()

	if (TARGET ${PROJECT_NAME}_BuildInfo.h)
		add_dependencies(${TARGET} ${PROJECT_NAME}_BuildInfo.h)
	endif()

	foreach(MODULE ${ARGN})
		string(REPLACE "::" ";" MODULE_PARTS "${MODULE}")
		list(GET MODULE_PARTS 0 MODULE_MAIN)
		list(LENGTH MODULE_PARTS MODULE_LENGTH)
		if (MODULE_LENGTH GREATER 1)
			list(GET MODULE_PARTS 1 MODULE_SUB)
		endif()
		# TODO: check if file exists if not, throws FATAL_ERROR with detailed description
		get_target_property(TARGET_APPLIED ${TARGET} TARGET_APPLIED_${MODULE_MAIN}_${MODULE_SUB})
		if (NOT TARGET_APPLIED)
			include(Use${MODULE_MAIN})
			set_target_properties(${TARGET} PROPERTIES TARGET_APPLIED_${MODULE_MAIN}_${MODULE_SUB} TRUE)
			eth_apply(${TARGET} ${REQUIRED} ${MODULE_SUB})
		endif()
	endforeach()
endfunction()

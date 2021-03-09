cmake_minimum_required(VERSION 3.9)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

project(xnvme
	LANGUAGES C CXX
	VERSION 0.0.22
)
message( STATUS "XNVME_VERSION: ${PROJECT_VERSION}" )
set(CMAKE_C_STANDARD 11)
include(FeatureSummary)
include(CheckCCompilerFlag)
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckIPOSupported)
include(bundle_libs)
if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|clang)$")
	message( "" )
	message( WARNING "You are using an untested toolchain" )
	message( "  The GNU toolchain is preferred on Linux" )
	message( "  The clang toolchain is preffered on FreeBSD" )
	message( "  You are in uncharted territory!" )
	message( "  Make sure you set CC and CXX" )
	message( "" )
	message( "  CC=gcc CXX=g++ ./configure --enable-<options>" )
	message( "    AND" )
	message( "  make CC=gcc CXX=g++" )
	message( "" )
	message( "" )
endif()

function(enable_c_flag flag)
	string(FIND "${CMAKE_C_FLAGS}" "${flag}" flag_already_set)
	if(flag_already_set EQUAL -1)
		check_c_compiler_flag("${flag}" flag_supported)
		if(flag_supported)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
		endif()
	endif()
endfunction()

enable_c_flag("-std=gnu11")
enable_c_flag("-Wall")
enable_c_flag("-Wextra")
enable_c_flag("-Werror")

# Due to a bug in clang
enable_c_flag("-Wno-missing-braces")

# For gprof traces
#enable_c_flag("-pg")

# NOTE: Cannot enable this due to third-party using non-ISO features
#enable_c_flag("-pedantic")

find_package(bash-completion QUIET)
message( STATUS "BASH_COMPLETION_FOUND: '${BASH_COMPLETION_FOUND}'" )
message( STATUS "BASH_COMPLETION_COMPLETIONSDIR: '${BASH_COMPLETION_COMPLETIONSDIR}'" )

# Find pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

check_ipo_supported(RESULT IPO_SUPPORTED)
check_ipo_supported(RESULT result OUTPUT output)
if(IPO_SUPPORTED)
	set(HAS_IPO TRUE)
else()
	set(HAS_IPO FALSE)
endif()

if(CMAKE_SYSTEM MATCHES "Linux")
	set(FREEBSD 0)
	set(LINUX 1)
elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
	set(FREEBSD 1)
	set(LINUX 0)
else()
	set(FREEBSD 0)
	set(LINUX 0)
endif()

message( STATUS "CMAKE_INTERPROCEDURAL_OPTIMIZATION: '${CMAKE_INTERPROCEDURAL_OPTIMIZATION}'" )
message( STATUS "IPO_SUPPORTED: '${IPO_SUPPORTED}'" )
message( STATUS "HAS_IPO: '${HAS_IPO}'" )

message( STATUS "CMAKE_C_FLAGS: '${CMAKE_C_FLAGS}'" )
message( STATUS "CMAKE_SYSTEM: '${CMAKE_SYSTEM}'" )
message( STATUS "CMAKE_SYSTEM_NAME: '${CMAKE_SYSTEM_NAME}'" )
message( STATUS "CMAKE_HOST_SYSTEM_NAME: '${CMAKE_HOST_SYSTEM_NAME}'" )
message( STATUS "CMAKE_C_COMPILER_AR: '${CMAKE_C_COMPILER_AR}'" )
message( STATUS "CMAKE_AR: '${CMAKE_AR}'" )

message( STATUS "UNIX(${UNIX}), FREEBSD(${FREEBSD}), LINUX(${LINUX}), WIN32(${WIN32})" )

if(WIN32)
	add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
endif()

#
# Check for struct timespec ts and clock_gettime
#
check_library_exists(c clock_gettime "" LIBC_HAS_CLOCK_GETTIME)
check_library_exists(rt clock_gettime "time.h" LIBRT_HAS_CLOCK_GETTIME)

# On Windows we assume it is available via the TDM-GCC compiler suite
if(WIN32)
	message( WARNING "Skipping check for clock_gettime..." )
else ()
	if((NOT LIBC_HAS_BLOCK_GETTIME) AND (NOT LIBRT_HAS_CLOCK_GETTIME))
		message( FATAL_ERROR "Cannot find clock_gettime" )
	endif()
endif()

# Add versioning
add_definitions(-DXNVME_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_definitions(-DXNVME_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_definitions(-DXNVME_VERSION_PATCH=${PROJECT_VERSION_PATCH})
add_definitions(-DXNVME_VERSION=${PROJECT_VERSION})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DXNVME_DEBUG_ENABLED")

set(XNVME_TRACE_ENABLED FALSE CACHE BOOL "Enable NVMe command tracing")
if(XNVME_TRACE_ENABLED)
	add_definitions(-DXNVME_TRACE_ENABLED)
endif()

#
# NOTE: CMake prefers that filenames are hard-coded in CMakeLists.txt and not
# globbed as it cannot detect, in the build-files genereated by CMake, when a
# new file is added. However, it seems that value of that feature is less than
# the convenience file-globbing.
#
file(GLOB PUBLIC_FILES  ${PROJECT_SOURCE_DIR}/include/lib*.h)
file(GLOB INTERNAL_FILES  ${PROJECT_SOURCE_DIR}/include/xnvme_*.h)
file(GLOB SOURCE_FILES  ${PROJECT_SOURCE_DIR}/src/*.c)

include_directories("${PROJECT_SOURCE_DIR}/include")

# Default 'clang' on FreeBSD does not look here, so we tell it to probably
# another symptom of 'base-system-compiler-we-do-not-care'
link_directories("/usr/local/lib")

#
# LIBS are the libraries which are "submoduled" by xNVMe and linked statically,
# and for the bundled version of the library embedded in the archive
#
list(APPEND LIBS "")

#
# AUX_LIB_PATHS non-system paths to search for libraries, search for 'LIBS here
#
list(APPEND AUX_LIB_PATHS "")

#
# LIBS_STATIC to be populated with absolute paths to 'LIBS' for bundling
#
list(APPEND LIBS_STATIC "")

#
# LIBS_SYSTEM are libraries which are expected to be available on the system on
# which xNVMe is built and at runtime when using the shared version of xNVMe
#
list(APPEND LIBS_SYSTEM Threads::Threads)

set(FIO_SRC_PATH "${PROJECT_SOURCE_DIR}/third-party/fio/repos"
	CACHE STRING "fio-engines: path to fio source repos")

#
# BACKENDS -- begin
#

#
# XNVME_BE_SPDK
#
set(XNVME_BE_SPDK_ENABLED ${UNIX} CACHE BOOL "be_spdk: SPDK backend")
set(XNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED ${UNIX} CACHE BOOL "be:spdk: PCIe Transport")
set(XNVME_BE_SPDK_TRANSPORT_TCP_ENABLED ${UNIX} CACHE BOOL "be:spdk: TCP Transport")
set(XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED ${UNIX} CACHE BOOL "be:spdk: RDMA Transport")
set(XNVME_BE_SPDK_TRANSPORT_FC_ENABLED ${UNIX} CACHE BOOL "be:spdk: FC Transport")
set(SPDK_REPOS
	"${PROJECT_SOURCE_DIR}/third-party/spdk/repos"
	CACHE PATH "SPDK Repository path")
set(SPDK_INCLUDE_PATH
	"${SPDK_REPOS}/build/include"
	CACHE PATH "SPDK include path")
set(SPDK_LIBRARY_PATH
	"${SPDK_REPOS}/build/lib"
	CACHE PATH "SPDK library path")
set(DPDK_INCLUDE_PATH
	"${SPDK_REPOS}/dpdk/build/include"
	CACHE PATH "DPDK include path")
set(DPDK_LIBRARY_PATH
	"${SPDK_REPOS}/dpdk/build/lib"
	CACHE PATH "DPDK library path")

if(XNVME_BE_SPDK_ENABLED)
	message( STATUS "SPDK_LIBRARY_PATH(${SPDK_LIBRARY_PATH})" )
	message( STATUS "SPDK_INCLUDE_PATH(${SPDK_INCLUDE_PATH})" )
	message( STATUS "DPDK_LIBRARY_PATH(${DPDK_LIBRARY_PATH})" )
	message( STATUS "DPDK_INCLUDE_PATH(${DPDK_INCLUDE_PATH})" )

	# This ensures that arch-conf headers are available
	if(NOT EXISTS "${FIO_SRC_PATH}/fio")
		message("Building fio")

		execute_process(
			COMMAND make third-party-fio
			RESULT_VARIABLE res
			WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
		if(res)
			message( FATAL_ERROR "Failed building fio" )
		endif()
	endif()

	# Fetch SPDK/DPDK if it is not there
	if(NOT EXISTS "${SPDK_INCLUDE_PATH}")
		message( STATUS "Preparing SPDK/DPDK" )
		execute_process(
			COMMAND make third-party-update
			RESULT_VARIABLE res
			WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
		if(res)
			message( FATAL_ERROR "Failed fetching SPDK/DPDK" )
		endif()
	endif()

	# Build SPDK/DPDK if SPDK libraries are not there
	if(NOT EXISTS "${SPDK_LIBRARY_PATH}/libspdk_nvme.a")
		message( STATUS "Preparing SPDK/DPDK dependency" )

		message( STATUS "Patching SPDK/DPDK" )
		execute_process(
			COMMAND make third-party-spdk-patch
			RESULT_VARIABLE res
			WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
		if(res)
			message( FATAL_ERROR "Failed patching SPDK/DPDK" )
		endif()

		message( STATUS "Configuring SPDK/DPDK" )
		execute_process(
			COMMAND make third-party-spdk-configure
			RESULT_VARIABLE res
			WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
		if(res)
			message( FATAL_ERROR "Failed configuring SPDK/DPDK" )
		endif()

		message( STATUS "Building SPDK/DPDK" )
		execute_process(
			COMMAND make third-party-spdk-build
			RESULT_VARIABLE res
			WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
		if(res)
			message( FATAL_ERROR "Failed building SPDK/DPDK" )
		endif()
	endif()

	add_definitions(-DXNVME_BE_SPDK_ENABLED)
	if(XNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED)
		add_definitions(-DXNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED)
	endif()
	if(XNVME_BE_SPDK_TRANSPORT_TCP_ENABLED)
		add_definitions(-DXNVME_BE_SPDK_TRANSPORT_TCP_ENABLED)
	endif()
	if(XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED)
		add_definitions(-DXNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED)
	endif()
	if(XNVME_BE_SPDK_TRANSPORT_FC_ENABLED)
		add_definitions(-DXNVME_BE_SPDK_TRANSPORT_FC_ENABLED)
	endif()

	message( STATUS "ENABLING ASYNC SUPPORT" )
	add_definitions(-DXNVME_ASYNC_ENABLED)

	# These are expected to be available on the system
	list(APPEND LIBS_SYSTEM rt)
	if (LINUX)
		list(APPEND LIBS_SYSTEM numa)
	elseif (FREEBSD)
		list(APPEND LIBS_SYSTEM execinfo)
		list(APPEND LIBS_SYSTEM elf)
	endif()
	list(APPEND LIBS_SYSTEM uuid)

	#
	# Must be built and available in SPDK_LIBRARY_PATH
	#
	list(APPEND LIBS spdk_nvme)
	list(APPEND LIBS spdk_env_dpdk)
	list(APPEND LIBS spdk_sock)
	list(APPEND LIBS spdk_sock_posix)
	list(APPEND LIBS spdk_rpc)
	list(APPEND LIBS spdk_jsonrpc)
	list(APPEND LIBS spdk_json)
	list(APPEND LIBS spdk_util)
	list(APPEND LIBS spdk_log)

	#
	# Must available in DPDK_LIBRARY_PATH
	#
	list(APPEND LIBS rte_eal)
	list(APPEND LIBS rte_telemetry)
	list(APPEND LIBS rte_bus_pci)
	list(APPEND LIBS rte_pci)
	list(APPEND LIBS rte_ring)
	list(APPEND LIBS rte_mempool)
	list(APPEND LIBS rte_kvargs)

	include_directories("${SPDK_INCLUDE_PATH}")
	include_directories("${DPDK_INCLUDE_PATH}")
	link_directories("${SPDK_LIBRARY_PATH}")
	link_directories("${DPDK_LIBRARY_PATH}")

	list(APPEND AUX_LIB_PATHS ${SPDK_LIBRARY_PATH})
	list(APPEND AUX_LIB_PATHS ${DPDK_LIBRARY_PATH})
endif()
message( STATUS "BE:SPDK ENABLED(${XNVME_BE_SPDK_ENABLED})" )

#
# XNVME_BE_FBSD
#
set(XNVME_BE_FBSD_ENABLED ${FREEBSD} CACHE BOOL
	"be_fbsd: FreeBSD backend")
if(XNVME_BE_FBSD_ENABLED)
	add_definitions(-DXNVME_BE_FBSD_ENABLED)
endif()
message( STATUS "BE:FBSD ENABLED(${XNVME_BE_FBSD_ENABLED})" )

#
# XNVME_BE_LINUX
#
set(XNVME_BE_LINUX_ENABLED ${LINUX} CACHE BOOL
	"be_linux: Linux backend")
#set(XNVME_BE_LINUX_THR_ENABLED OFF CACHE BOOL
set(XNVME_BE_LINUX_THR_ENABLED ${LINUX} CACHE BOOL
	"be_linux_async_thr: thread support in Linux backend")
set(XNVME_BE_LINUX_AIO_ENABLED ${LINUX} CACHE BOOL
	"be_linux_async_aio: aio supportin Linux backend")
set(XNVME_BE_LINUX_IOU_ENABLED ${LINUX} CACHE BOOL
	"be_linux_async_iou: io_uring support in Linux backend")
set(XNVME_BE_LINUX_NIL_ENABLED ${LINUX} CACHE BOOL
	"be_linux_async_nil: null-io support in Linux backend")
set(XNVME_BE_LINUX_BLOCK_ENABLED ${LINUX} CACHE BOOL
	"be_linux_block: support for non-NVMe devices via the Linux Block Layer")
set(XNVME_BE_LINUX_BLOCK_ZONED_ENABLED ${LINUX} CACHE BOOL
	"be_linux_block_zoned: support Linux Zoned Block Devices")
set(XNVME_BE_LINUX_FS_ENABLED ${LINUX} CACHE BOOL
	"be_linux_fs: support for traditional file-system file-IO via the Linux Block Layer")

# TODO: consider expanding compile-time checking of Linux Block layer features
if(XNVME_BE_LINUX_BLOCK_ZONED_ENABLED)
	CHECK_INCLUDE_FILE(linux/blkzoned.h HAVE_LINUX_BLKZONED_HEADERS)
	if(NOT HAVE_LINUX_BLKZONED_HEADERS)
		message( WARNING "disabling Linux Zoned Block device support, cannot find header: linux/blkzoned.h " )
		set(XNVME_BE_LINUX_BLOCK_ENABLED off)
	endif()
endif()

set(LIBURING_REPOS "${PROJECT_SOURCE_DIR}/third-party/liburing/repos")
set(LIBURING_INCLUDE_PATH
	"${LIBURING_REPOS}/src/include"
	CACHE PATH "LIBURING include path")
set(LIBURING_LIBRARY_PATH
	"${LIBURING_REPOS}/src"
	CACHE PATH "LIBURING library path")

if(XNVME_BE_LINUX_ENABLED)

	add_definitions(-DXNVME_BE_LINUX_ENABLED)

	if(XNVME_BE_LINUX_FS_ENABLED)
		add_definitions(-DXNVME_BE_LINUX_FS_ENABLED)
	endif()

	if(XNVME_BE_LINUX_BLOCK_ENABLED)
		add_definitions(-DXNVME_BE_LINUX_BLOCK_ENABLED)

		if(XNVME_BE_LINUX_BLOCK_ZONED_ENABLED)
			add_definitions(-DXNVME_BE_LINUX_BLOCK_ZONED_ENABLED)
		endif()
	endif()

	if(XNVME_BE_LINUX_THR_ENABLED)
		add_definitions(-DXNVME_BE_LINUX_THR_ENABLED)

		list(APPEND LIBS_SYSTEM pthread)

		message( STATUS "ENABLING ASYNC SUPPORT" )
		add_definitions(-DXNVME_ASYNC_ENABLED)
	endif()

	if(XNVME_BE_LINUX_AIO_ENABLED)
		add_definitions(-DXNVME_BE_LINUX_AIO_ENABLED)

		list(APPEND LIBS_SYSTEM aio)

		message( STATUS "ENABLING ASYNC SUPPORT" )
		add_definitions(-DXNVME_ASYNC_ENABLED)
	endif()

	if(XNVME_BE_LINUX_IOU_ENABLED)
		message( STATUS "LIBURING_LIBRARY_PATH(${LIBURING_LIBRARY_PATH})" )
		message( STATUS "LIBURING_INCLUDE_PATH(${LIBURING_INCLUDE_PATH})" )

		if(NOT (EXISTS ${LIBURING_LIBRARY_PATH} AND
			EXISTS ${LIBURING_INCLUDE_PATH}))
			execute_process(
				COMMAND make third-party-update
				RESULT_VARIABLE res
				WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
			if(res)
				message( FATAL_ERROR "Failed fetching liburing" )
			endif()
		endif()

		if(NOT EXISTS "${LIBURING_LIBRARY_PATH}/liburing.a")
			execute_process(
				COMMAND make third-party-liburing-build
				RESULT_VARIABLE res
				WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
			if(res)
				message( FATAL_ERROR "Failed building liburing" )
			endif()
		endif()

		add_definitions(-DXNVME_BE_LINUX_IOU_ENABLED)

		list(APPEND LIBS uring)

		# Add liburing to library and include
		include_directories("${LIBURING_INCLUDE_PATH}")
		link_directories("${LIBURING_LIBRARY_PATH}")

		list(APPEND AUX_LIB_PATHS ${LIBURING_LIBRARY_PATH})

		message( STATUS "ENABLING ASYNC SUPPORT" )
		add_definitions(-DXNVME_ASYNC_ENABLED)
	endif()

	if(XNVME_BE_LINUX_NIL_ENABLED)
		add_definitions(-DXNVME_BE_LINUX_NIL_ENABLED)

		message( STATUS "ENABLING ASYNC SUPPORT" )
		add_definitions(-DXNVME_ASYNC_ENABLED)
	endif()

endif()
message( STATUS "BE:LINUX ENABLED(${XNVME_BE_LINUX_ENABLED})" )
message( STATUS "BE:LINUX_AIO ENABLED(${XNVME_BE_LINUX_AIO_ENABLED})" )
message( STATUS "BE:LINUX_IOU ENABLED(${XNVME_BE_LINUX_IOU_ENABLED})" )
message( STATUS "BE:LINUX_NIL ENABLED(${XNVME_BE_LINUX_NIL_ENABLED})" )

#
# BACKENDS -- end
#

# Find the static version of LIBS
foreach(lib IN LISTS LIBS)
	unset(lib_path CACHE)
	find_library(lib_path
		NAMES "lib${lib}.a"
		PATHS ${AUX_LIB_PATHS}
	)

	list(APPEND LIBS_STATIC "${lib_path}")
endforeach()

message( STATUS "LIBS(${LIBS})" )
message( STATUS "LIBS_STATIC(${LIBS_STATIC})" )
message( STATUS "LIBS_SYSTEM(${LIBS_SYSTEM})" )

#
# The xNVMe library incantations: shared, static, and "bundled"
#
set(LIB_SHARED "${PROJECT_NAME}-shared")

set(LIB_STATIC "${PROJECT_NAME}-static")
set(LIB_STATIC_FN "${CMAKE_STATIC_LIBRARY_PREFIX}${LIB_STATIC}${CMAKE_STATIC_LIBRARY_SUFFIX}")

set(LIB_BUNDLE "${PROJECT_NAME}")

# Static library
add_library(${LIB_STATIC} STATIC ${PUBLIC_FILES} ${INTERNAL_FILES} ${SOURCE_FILES})
set_target_properties(${LIB_STATIC} PROPERTIES
	INTERPROCEDURAL_OPTIMIZATION HAS_IPO
)
target_compile_options(${LIB_STATIC} PUBLIC -fPIC)
target_link_libraries(${LIB_STATIC} ${LIBS})
target_link_libraries(${LIB_STATIC} ${LIBS_SYSTEM})
install(TARGETS ${LIB_STATIC} DESTINATION lib COMPONENT dev)

# Bundled library
list(APPEND BUNDLED_LIBS ${LIB_STATIC_FN})
list(APPEND BUNDLED_LIBS ${LIBS_STATIC})
message( STATUS "BUNDLED_LIBS(${BUNDLED_LIBS})" )

bundle_libs("${LIB_STATIC}" "${LIB_BUNDLE}")
message(STATUS "bundle_libs(${BUNDLE_LIBS})")
if(NOT "${BUNDLE_LIBS}" STREQUAL "SUCCESS")
	message( FATAL_ERROR "bundling libs failed" )
endif()

# Shared library
add_library(${LIB_SHARED} SHARED ${PUBLIC_FILES} ${INTERNAL_FILES} ${SOURCE_FILES})
set_target_properties(${LIB_SHARED} PROPERTIES
	INTERPROCEDURAL_OPTIMIZATION HAS_IPO
)
target_compile_options(${LIB_SHARED} PUBLIC -fPIC)
target_link_libraries(${LIB_SHARED} ${LIBS})
target_link_libraries(${LIB_SHARED} ${LIBS_SYSTEM})
install(TARGETS ${LIB_SHARED} DESTINATION lib COMPONENT dev)
install(FILES ${PUBLIC_FILES} DESTINATION include COMPONENT dev)

add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(tools/fio-engine)

#
# Packages
#
if(WIN32)
	set(CPACK_GENERATOR "NSIS" "ZIP")
else()
	set(CPACK_GENERATOR "TGZ" "DEB")
endif()
#set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}.bin")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Simon A. F. Lund")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_COMPONENT_DEV_DESCRIPTION "xnvme-dev: headers and libraries for xNVMe APIs")
if(TOOLS)
	set(CPACK_COMPONENT_TOOLS_DESCRIPTION "xnvme-tools: suite of xNVMe command-line tools")
endif()
if(EXAMPLES)
	set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "xnvme-examples: xNVMe examples provided as binaries")
endif()
if(TESTS)
	set(CPACK_COMPONENT_TESTS_DESCRIPTION "xnvme-tests: xNVMe test programs")
endif()

include(CPack)

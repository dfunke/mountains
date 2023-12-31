cmake_minimum_required(VERSION 3.0)

project(mountains,
    VERSION 1.0
    DESCRIPTION "Mountain calculation scripts"
    LANGUAGES CXX
)


set( CMAKE_C_FLAGS_DEBUG "" CACHE STRING "" )       
set( CMAKE_CXX_FLAGS_DEBUG "" CACHE STRING "" )
set( CMAKE_C_FLAGS_RELEASE "" CACHE STRING "" )
set( CMAKE_CXX_FLAGS_RELEASE "" CACHE STRING "" )

#tbb
# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/external/comake-modules)
# find_package(TBB COMPONENTS tbbmalloc tbbmalloc_proxy tbb_preview)


# External dependencies for sweepline-isolation-finder

# glm
include_directories(external/glm)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
# set( IPS2RA_DISABLE_PARALLEL "true")
# include_directories(ips2ra/extern/tlx)
set(IPS4O_DISABLE_PARALLEL "true")
add_subdirectory(external/ips4o)

set(
	BUILD_TARGETS
	isolation
	converter
)

set(
	BUILD_LIBS
	coordlib
	loaderlib
	pointlib
)

set (
	COMMONFLAGS
	-DPLATFORM_LINUX 
	-Wall
	-Wno-unused-variable 
	-DELPP_NO_DEFAULT_LOG_FILE 
	-DELPP_THREAD_SAFE 
	-pthread
)

set(
    POINTLIB_OBJS 
	code/latlng.cpp
	code/quadtree.cpp
	code/util.cpp
)

set(
    LOADERLIB_OBJS
	code/flt_loader.cpp 
	code/glo_loader.cpp 
	code/hgt_loader.cpp 
	code/tiff_loader.cpp 
)
set(COORDLIB_OBJS
	code/coordinate_system.cpp 
	code/degree_coordinate_system.cpp 
	code/utm_coordinate_system.cpp )

set(
    POINTLIB_OBJS 
	code/latlng.cpp
	code/quadtree.cpp
	code/util.cpp
)

add_library(
	pointlib SHARED
	code/latlng.cpp
	code/quadtree.cpp
	code/util.cpp
)

add_library(
	loaderlib SHARED
	code/flt_loader.cpp
	code/glo_loader.cpp
	code/hgt_loader.cpp
	code/tiff_loader.cpp 
)

target_link_libraries(
       loaderlib PRIVATE
       tiff
)

add_library(
	coordlib
	code/coordinate_system.cpp
	code/degree_coordinate_system.cpp
	code/utm_coordinate_system.cpp
)


add_executable(
    isolation
	code/easylogging++.cc
	code/file_format.cpp 
	code/isolation.cpp 
	code/isolation_finder.cpp 
	code/isolation_results.cpp 
	code/isolation_task.cpp 
	code/peak_finder.cpp 
	code/point_map.cpp 
	code/tile.cpp 
	code/tile_cache.cpp 
	code/tile_loading_policy.cpp
	code/fabdem_loader.cpp
	code/isolation_finder_sl/cell_memory_manager.cpp
	code/isolation_finder_sl/cell.cpp
	code/isolation_finder_sl/concurrent_isolation_results.cpp
	code/isolation_finder_sl/ilp_search_area_tree.cpp
	code/isolation_finder_sl/isolation_finder_sl.cpp
	code/isolation_finder_sl/isolation_sl_processor.cpp
	code/isolation_finder_sl/sweepline_datastruct_naive.cpp
	code/isolation_finder_sl/sweepline_datastruct_kd.cpp
	code/isolation_finder_sl/sweepline_datastruct.cpp
	code/isolation_finder_sl/sweepline_primitives.cpp
	code/isolation_finder_sl/tile_cell.cpp
)

target_link_libraries(
	isolation PRIVATE
	pointlib 
	loaderlib 
	coordlib
	tbb
	pthread
	ips4o
)

add_executable(
    converter
	code/easylogging++.cc
	code/file_format.cpp 
	code/tile.cpp 
	code/tile_cache.cpp 
	code/tile_loading_policy.cpp
	code/fabdem_loader.cpp
	code/converter.cpp
	code/hgt_writer.cpp
)

target_link_libraries(
    converter PRIVATE
	pointlib 
	loaderlib 
	coordlib
)


add_executable(
    speedtesting
	code/easylogging++.cc
	code/file_format.cpp 
	code/speedtesting.cpp 
	code/isolation_finder.cpp 
	code/isolation_results.cpp 
	code/isolation_task.cpp 
	code/peak_finder.cpp 
	code/point_map.cpp 
	code/tile.cpp 
	code/tile_cache.cpp 
	code/tile_loading_policy.cpp
	code/fabdem_loader.cpp
	code/isolation_finder_sl/cell_memory_manager.cpp
	code/isolation_finder_sl/cell.cpp
	code/isolation_finder_sl/concurrent_isolation_results.cpp
	code/isolation_finder_sl/ilp_search_area_tree.cpp
	code/isolation_finder_sl/isolation_finder_sl.cpp
	code/isolation_finder_sl/isolation_sl_processor.cpp
	code/isolation_finder_sl/sweepline_datastruct_naive.cpp
	code/isolation_finder_sl/sweepline_datastruct_kd.cpp
	code/isolation_finder_sl/sweepline_datastruct.cpp
	code/isolation_finder_sl/sweepline_primitives.cpp
	code/isolation_finder_sl/tile_cell.cpp
)

target_link_libraries(
	speedtesting PRIVATE
	pointlib 
	loaderlib 
	coordlib
	tbb
	pthread
	ips4o
)

add_executable(
   check_tile 
	code/easylogging++.cc
	code/file_format.cpp 
	code/check_tile.cpp 
	code/isolation_finder.cpp 
	code/isolation_results.cpp 
	code/isolation_task.cpp 
	code/peak_finder.cpp 
	code/point_map.cpp 
	code/tile.cpp 
	code/tile_cache.cpp 
	code/tile_loading_policy.cpp
	code/fabdem_loader.cpp
	code/isolation_finder_sl/cell_memory_manager.cpp
	code/isolation_finder_sl/cell.cpp
	code/isolation_finder_sl/concurrent_isolation_results.cpp
	code/isolation_finder_sl/ilp_search_area_tree.cpp
	code/isolation_finder_sl/isolation_finder_sl.cpp
	code/isolation_finder_sl/isolation_sl_processor.cpp
	code/isolation_finder_sl/sweepline_datastruct_naive.cpp
	code/isolation_finder_sl/sweepline_datastruct_kd.cpp
	code/isolation_finder_sl/sweepline_datastruct.cpp
	code/isolation_finder_sl/sweepline_primitives.cpp
	code/isolation_finder_sl/tile_cell.cpp
)

target_link_libraries(
	check_tile PRIVATE
	pointlib 
	loaderlib 
	coordlib
	tbb
	pthread
	ips4o
)
 add_executable(
 	prominence
 	code/divide_tree.cpp
 	code/domain_map.cpp 
 	code/easylogging++.cc 
 	code/filter.cpp 
 	code/file_format.cpp 
 	code/fabdem_loader.cpp
 	code/island_tree.cpp 
 	code/kml_writer.cpp 
 	code/line_tree.cpp 
 	code/point_map.cpp 
 	code/prominence.cpp 
 	code/prominence_task.cpp 
 	code/tile.cpp 
 	code/tile_cache.cpp 
 	code/tile_loading_policy.cpp 
 	code/tree_builder.cpp 
 )
 
 target_link_libraries(
 	prominence
 	pointlib 
 	loaderlib 
 	coordlib
 	pthread
 )
 
 add_executable(
 	merge_divide_trees
 	code/divide_tree.cpp
 	code/easylogging++.cc
 	code/island_tree.cpp
 	code/kml_writer.cpp
 	code/line_tree.cpp
 	code/merge_divide_trees.cpp
 	code/tile.cpp
 )
 
 target_link_libraries(
 	merge_divide_trees
 	pointlib  
 	coordlib
 )
 
 add_executable(
 	filter_points
 	code/easylogging++.cc
 	code/filter.cpp
 	code/filter_points.cpp
 )
 
 target_link_libraries(
 	filter_points
 	pointlib  
 )
 
 add_executable(
 	compute_parents
 	code/divide_tree.cpp
 	code/easylogging++.cc
 	code/island_tree.cpp
 	code/kml_writer.cpp
 	code/line_tree.cpp
 	code/compute_parents.cpp
 )
 
 target_link_libraries(
 	compute_parents
 	pointlib
 	coordlib
 )

# Build libraries
foreach(X IN LISTS BUILD_LIBS)
	target_compile_features(${X} PUBLIC cxx_std_17)
	target_compile_options(${X} PUBLIC ${COMMONFLAGS})
endforeach()

# Build executables
foreach(X IN LISTS BUILD_TARGETS)
	target_compile_features(${X} PRIVATE cxx_std_17)
	target_compile_options(${X} PRIVATE ${COMMONFLAGS})
endforeach()

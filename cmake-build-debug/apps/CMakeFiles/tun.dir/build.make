# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/bingbong/compiler/network

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/bingbong/compiler/network/cmake-build-debug

# Include any dependencies generated for this target.
include apps/CMakeFiles/tun.dir/depend.make

# Include the progress variables for this target.
include apps/CMakeFiles/tun.dir/progress.make

# Include the compile flags for this target's objects.
include apps/CMakeFiles/tun.dir/flags.make

apps/CMakeFiles/tun.dir/tun.cc.o: apps/CMakeFiles/tun.dir/flags.make
apps/CMakeFiles/tun.dir/tun.cc.o: ../apps/tun.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bingbong/compiler/network/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object apps/CMakeFiles/tun.dir/tun.cc.o"
	cd /home/bingbong/compiler/network/cmake-build-debug/apps && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/tun.dir/tun.cc.o -c /home/bingbong/compiler/network/apps/tun.cc

apps/CMakeFiles/tun.dir/tun.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tun.dir/tun.cc.i"
	cd /home/bingbong/compiler/network/cmake-build-debug/apps && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bingbong/compiler/network/apps/tun.cc > CMakeFiles/tun.dir/tun.cc.i

apps/CMakeFiles/tun.dir/tun.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tun.dir/tun.cc.s"
	cd /home/bingbong/compiler/network/cmake-build-debug/apps && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bingbong/compiler/network/apps/tun.cc -o CMakeFiles/tun.dir/tun.cc.s

# Object files for target tun
tun_OBJECTS = \
"CMakeFiles/tun.dir/tun.cc.o"

# External object files for target tun
tun_EXTERNAL_OBJECTS =

apps/tun: apps/CMakeFiles/tun.dir/tun.cc.o
apps/tun: apps/CMakeFiles/tun.dir/build.make
apps/tun: src/libsponge.a
apps/tun: /usr/lib/x86_64-linux-gnu/libpthread.so
apps/tun: apps/CMakeFiles/tun.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/bingbong/compiler/network/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable tun"
	cd /home/bingbong/compiler/network/cmake-build-debug/apps && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tun.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
apps/CMakeFiles/tun.dir/build: apps/tun

.PHONY : apps/CMakeFiles/tun.dir/build

apps/CMakeFiles/tun.dir/clean:
	cd /home/bingbong/compiler/network/cmake-build-debug/apps && $(CMAKE_COMMAND) -P CMakeFiles/tun.dir/cmake_clean.cmake
.PHONY : apps/CMakeFiles/tun.dir/clean

apps/CMakeFiles/tun.dir/depend:
	cd /home/bingbong/compiler/network/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bingbong/compiler/network /home/bingbong/compiler/network/apps /home/bingbong/compiler/network/cmake-build-debug /home/bingbong/compiler/network/cmake-build-debug/apps /home/bingbong/compiler/network/cmake-build-debug/apps/CMakeFiles/tun.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : apps/CMakeFiles/tun.dir/depend


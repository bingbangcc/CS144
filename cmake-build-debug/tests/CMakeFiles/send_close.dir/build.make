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
include tests/CMakeFiles/send_close.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/send_close.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/send_close.dir/flags.make

tests/CMakeFiles/send_close.dir/send_close.cc.o: tests/CMakeFiles/send_close.dir/flags.make
tests/CMakeFiles/send_close.dir/send_close.cc.o: ../tests/send_close.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bingbong/compiler/network/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/send_close.dir/send_close.cc.o"
	cd /home/bingbong/compiler/network/cmake-build-debug/tests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/send_close.dir/send_close.cc.o -c /home/bingbong/compiler/network/tests/send_close.cc

tests/CMakeFiles/send_close.dir/send_close.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/send_close.dir/send_close.cc.i"
	cd /home/bingbong/compiler/network/cmake-build-debug/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bingbong/compiler/network/tests/send_close.cc > CMakeFiles/send_close.dir/send_close.cc.i

tests/CMakeFiles/send_close.dir/send_close.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/send_close.dir/send_close.cc.s"
	cd /home/bingbong/compiler/network/cmake-build-debug/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bingbong/compiler/network/tests/send_close.cc -o CMakeFiles/send_close.dir/send_close.cc.s

# Object files for target send_close
send_close_OBJECTS = \
"CMakeFiles/send_close.dir/send_close.cc.o"

# External object files for target send_close
send_close_EXTERNAL_OBJECTS =

tests/send_close: tests/CMakeFiles/send_close.dir/send_close.cc.o
tests/send_close: tests/CMakeFiles/send_close.dir/build.make
tests/send_close: tests/libspongechecks.a
tests/send_close: src/libsponge.a
tests/send_close: tests/CMakeFiles/send_close.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/bingbong/compiler/network/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable send_close"
	cd /home/bingbong/compiler/network/cmake-build-debug/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/send_close.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/send_close.dir/build: tests/send_close

.PHONY : tests/CMakeFiles/send_close.dir/build

tests/CMakeFiles/send_close.dir/clean:
	cd /home/bingbong/compiler/network/cmake-build-debug/tests && $(CMAKE_COMMAND) -P CMakeFiles/send_close.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/send_close.dir/clean

tests/CMakeFiles/send_close.dir/depend:
	cd /home/bingbong/compiler/network/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bingbong/compiler/network /home/bingbong/compiler/network/tests /home/bingbong/compiler/network/cmake-build-debug /home/bingbong/compiler/network/cmake-build-debug/tests /home/bingbong/compiler/network/cmake-build-debug/tests/CMakeFiles/send_close.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/send_close.dir/depend


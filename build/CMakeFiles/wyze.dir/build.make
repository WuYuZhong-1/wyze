# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

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
CMAKE_SOURCE_DIR = /home/wuyz/learn/wyze

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/wuyz/learn/wyze/build

# Include any dependencies generated for this target.
include CMakeFiles/wyze.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/wyze.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/wyze.dir/flags.make

CMakeFiles/wyze.dir/wyze/config.cpp.o: CMakeFiles/wyze.dir/flags.make
CMakeFiles/wyze.dir/wyze/config.cpp.o: ../wyze/config.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/wyze.dir/wyze/config.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/wyze.dir/wyze/config.cpp.o -c /home/wuyz/learn/wyze/wyze/config.cpp

CMakeFiles/wyze.dir/wyze/config.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wyze.dir/wyze/config.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wuyz/learn/wyze/wyze/config.cpp > CMakeFiles/wyze.dir/wyze/config.cpp.i

CMakeFiles/wyze.dir/wyze/config.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wyze.dir/wyze/config.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wuyz/learn/wyze/wyze/config.cpp -o CMakeFiles/wyze.dir/wyze/config.cpp.s

CMakeFiles/wyze.dir/wyze/config.cpp.o.requires:

.PHONY : CMakeFiles/wyze.dir/wyze/config.cpp.o.requires

CMakeFiles/wyze.dir/wyze/config.cpp.o.provides: CMakeFiles/wyze.dir/wyze/config.cpp.o.requires
	$(MAKE) -f CMakeFiles/wyze.dir/build.make CMakeFiles/wyze.dir/wyze/config.cpp.o.provides.build
.PHONY : CMakeFiles/wyze.dir/wyze/config.cpp.o.provides

CMakeFiles/wyze.dir/wyze/config.cpp.o.provides.build: CMakeFiles/wyze.dir/wyze/config.cpp.o


CMakeFiles/wyze.dir/wyze/fiber.cpp.o: CMakeFiles/wyze.dir/flags.make
CMakeFiles/wyze.dir/wyze/fiber.cpp.o: ../wyze/fiber.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/wyze.dir/wyze/fiber.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/wyze.dir/wyze/fiber.cpp.o -c /home/wuyz/learn/wyze/wyze/fiber.cpp

CMakeFiles/wyze.dir/wyze/fiber.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wyze.dir/wyze/fiber.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wuyz/learn/wyze/wyze/fiber.cpp > CMakeFiles/wyze.dir/wyze/fiber.cpp.i

CMakeFiles/wyze.dir/wyze/fiber.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wyze.dir/wyze/fiber.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wuyz/learn/wyze/wyze/fiber.cpp -o CMakeFiles/wyze.dir/wyze/fiber.cpp.s

CMakeFiles/wyze.dir/wyze/fiber.cpp.o.requires:

.PHONY : CMakeFiles/wyze.dir/wyze/fiber.cpp.o.requires

CMakeFiles/wyze.dir/wyze/fiber.cpp.o.provides: CMakeFiles/wyze.dir/wyze/fiber.cpp.o.requires
	$(MAKE) -f CMakeFiles/wyze.dir/build.make CMakeFiles/wyze.dir/wyze/fiber.cpp.o.provides.build
.PHONY : CMakeFiles/wyze.dir/wyze/fiber.cpp.o.provides

CMakeFiles/wyze.dir/wyze/fiber.cpp.o.provides.build: CMakeFiles/wyze.dir/wyze/fiber.cpp.o


CMakeFiles/wyze.dir/wyze/iomanager.cpp.o: CMakeFiles/wyze.dir/flags.make
CMakeFiles/wyze.dir/wyze/iomanager.cpp.o: ../wyze/iomanager.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/wyze.dir/wyze/iomanager.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/wyze.dir/wyze/iomanager.cpp.o -c /home/wuyz/learn/wyze/wyze/iomanager.cpp

CMakeFiles/wyze.dir/wyze/iomanager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wyze.dir/wyze/iomanager.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wuyz/learn/wyze/wyze/iomanager.cpp > CMakeFiles/wyze.dir/wyze/iomanager.cpp.i

CMakeFiles/wyze.dir/wyze/iomanager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wyze.dir/wyze/iomanager.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wuyz/learn/wyze/wyze/iomanager.cpp -o CMakeFiles/wyze.dir/wyze/iomanager.cpp.s

CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.requires:

.PHONY : CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.requires

CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.provides: CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.requires
	$(MAKE) -f CMakeFiles/wyze.dir/build.make CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.provides.build
.PHONY : CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.provides

CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.provides.build: CMakeFiles/wyze.dir/wyze/iomanager.cpp.o


CMakeFiles/wyze.dir/wyze/log.cpp.o: CMakeFiles/wyze.dir/flags.make
CMakeFiles/wyze.dir/wyze/log.cpp.o: ../wyze/log.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/wyze.dir/wyze/log.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/wyze.dir/wyze/log.cpp.o -c /home/wuyz/learn/wyze/wyze/log.cpp

CMakeFiles/wyze.dir/wyze/log.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wyze.dir/wyze/log.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wuyz/learn/wyze/wyze/log.cpp > CMakeFiles/wyze.dir/wyze/log.cpp.i

CMakeFiles/wyze.dir/wyze/log.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wyze.dir/wyze/log.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wuyz/learn/wyze/wyze/log.cpp -o CMakeFiles/wyze.dir/wyze/log.cpp.s

CMakeFiles/wyze.dir/wyze/log.cpp.o.requires:

.PHONY : CMakeFiles/wyze.dir/wyze/log.cpp.o.requires

CMakeFiles/wyze.dir/wyze/log.cpp.o.provides: CMakeFiles/wyze.dir/wyze/log.cpp.o.requires
	$(MAKE) -f CMakeFiles/wyze.dir/build.make CMakeFiles/wyze.dir/wyze/log.cpp.o.provides.build
.PHONY : CMakeFiles/wyze.dir/wyze/log.cpp.o.provides

CMakeFiles/wyze.dir/wyze/log.cpp.o.provides.build: CMakeFiles/wyze.dir/wyze/log.cpp.o


CMakeFiles/wyze.dir/wyze/scheduler.cpp.o: CMakeFiles/wyze.dir/flags.make
CMakeFiles/wyze.dir/wyze/scheduler.cpp.o: ../wyze/scheduler.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/wyze.dir/wyze/scheduler.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/wyze.dir/wyze/scheduler.cpp.o -c /home/wuyz/learn/wyze/wyze/scheduler.cpp

CMakeFiles/wyze.dir/wyze/scheduler.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wyze.dir/wyze/scheduler.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wuyz/learn/wyze/wyze/scheduler.cpp > CMakeFiles/wyze.dir/wyze/scheduler.cpp.i

CMakeFiles/wyze.dir/wyze/scheduler.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wyze.dir/wyze/scheduler.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wuyz/learn/wyze/wyze/scheduler.cpp -o CMakeFiles/wyze.dir/wyze/scheduler.cpp.s

CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.requires:

.PHONY : CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.requires

CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.provides: CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.requires
	$(MAKE) -f CMakeFiles/wyze.dir/build.make CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.provides.build
.PHONY : CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.provides

CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.provides.build: CMakeFiles/wyze.dir/wyze/scheduler.cpp.o


CMakeFiles/wyze.dir/wyze/thread.cpp.o: CMakeFiles/wyze.dir/flags.make
CMakeFiles/wyze.dir/wyze/thread.cpp.o: ../wyze/thread.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/wyze.dir/wyze/thread.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/wyze.dir/wyze/thread.cpp.o -c /home/wuyz/learn/wyze/wyze/thread.cpp

CMakeFiles/wyze.dir/wyze/thread.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wyze.dir/wyze/thread.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wuyz/learn/wyze/wyze/thread.cpp > CMakeFiles/wyze.dir/wyze/thread.cpp.i

CMakeFiles/wyze.dir/wyze/thread.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wyze.dir/wyze/thread.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wuyz/learn/wyze/wyze/thread.cpp -o CMakeFiles/wyze.dir/wyze/thread.cpp.s

CMakeFiles/wyze.dir/wyze/thread.cpp.o.requires:

.PHONY : CMakeFiles/wyze.dir/wyze/thread.cpp.o.requires

CMakeFiles/wyze.dir/wyze/thread.cpp.o.provides: CMakeFiles/wyze.dir/wyze/thread.cpp.o.requires
	$(MAKE) -f CMakeFiles/wyze.dir/build.make CMakeFiles/wyze.dir/wyze/thread.cpp.o.provides.build
.PHONY : CMakeFiles/wyze.dir/wyze/thread.cpp.o.provides

CMakeFiles/wyze.dir/wyze/thread.cpp.o.provides.build: CMakeFiles/wyze.dir/wyze/thread.cpp.o


CMakeFiles/wyze.dir/wyze/util.cpp.o: CMakeFiles/wyze.dir/flags.make
CMakeFiles/wyze.dir/wyze/util.cpp.o: ../wyze/util.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/wyze.dir/wyze/util.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/wyze.dir/wyze/util.cpp.o -c /home/wuyz/learn/wyze/wyze/util.cpp

CMakeFiles/wyze.dir/wyze/util.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wyze.dir/wyze/util.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wuyz/learn/wyze/wyze/util.cpp > CMakeFiles/wyze.dir/wyze/util.cpp.i

CMakeFiles/wyze.dir/wyze/util.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wyze.dir/wyze/util.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wuyz/learn/wyze/wyze/util.cpp -o CMakeFiles/wyze.dir/wyze/util.cpp.s

CMakeFiles/wyze.dir/wyze/util.cpp.o.requires:

.PHONY : CMakeFiles/wyze.dir/wyze/util.cpp.o.requires

CMakeFiles/wyze.dir/wyze/util.cpp.o.provides: CMakeFiles/wyze.dir/wyze/util.cpp.o.requires
	$(MAKE) -f CMakeFiles/wyze.dir/build.make CMakeFiles/wyze.dir/wyze/util.cpp.o.provides.build
.PHONY : CMakeFiles/wyze.dir/wyze/util.cpp.o.provides

CMakeFiles/wyze.dir/wyze/util.cpp.o.provides.build: CMakeFiles/wyze.dir/wyze/util.cpp.o


# Object files for target wyze
wyze_OBJECTS = \
"CMakeFiles/wyze.dir/wyze/config.cpp.o" \
"CMakeFiles/wyze.dir/wyze/fiber.cpp.o" \
"CMakeFiles/wyze.dir/wyze/iomanager.cpp.o" \
"CMakeFiles/wyze.dir/wyze/log.cpp.o" \
"CMakeFiles/wyze.dir/wyze/scheduler.cpp.o" \
"CMakeFiles/wyze.dir/wyze/thread.cpp.o" \
"CMakeFiles/wyze.dir/wyze/util.cpp.o"

# External object files for target wyze
wyze_EXTERNAL_OBJECTS =

../lib/libwyze.so: CMakeFiles/wyze.dir/wyze/config.cpp.o
../lib/libwyze.so: CMakeFiles/wyze.dir/wyze/fiber.cpp.o
../lib/libwyze.so: CMakeFiles/wyze.dir/wyze/iomanager.cpp.o
../lib/libwyze.so: CMakeFiles/wyze.dir/wyze/log.cpp.o
../lib/libwyze.so: CMakeFiles/wyze.dir/wyze/scheduler.cpp.o
../lib/libwyze.so: CMakeFiles/wyze.dir/wyze/thread.cpp.o
../lib/libwyze.so: CMakeFiles/wyze.dir/wyze/util.cpp.o
../lib/libwyze.so: CMakeFiles/wyze.dir/build.make
../lib/libwyze.so: CMakeFiles/wyze.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wuyz/learn/wyze/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking CXX shared library ../lib/libwyze.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/wyze.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/wyze.dir/build: ../lib/libwyze.so

.PHONY : CMakeFiles/wyze.dir/build

CMakeFiles/wyze.dir/requires: CMakeFiles/wyze.dir/wyze/config.cpp.o.requires
CMakeFiles/wyze.dir/requires: CMakeFiles/wyze.dir/wyze/fiber.cpp.o.requires
CMakeFiles/wyze.dir/requires: CMakeFiles/wyze.dir/wyze/iomanager.cpp.o.requires
CMakeFiles/wyze.dir/requires: CMakeFiles/wyze.dir/wyze/log.cpp.o.requires
CMakeFiles/wyze.dir/requires: CMakeFiles/wyze.dir/wyze/scheduler.cpp.o.requires
CMakeFiles/wyze.dir/requires: CMakeFiles/wyze.dir/wyze/thread.cpp.o.requires
CMakeFiles/wyze.dir/requires: CMakeFiles/wyze.dir/wyze/util.cpp.o.requires

.PHONY : CMakeFiles/wyze.dir/requires

CMakeFiles/wyze.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/wyze.dir/cmake_clean.cmake
.PHONY : CMakeFiles/wyze.dir/clean

CMakeFiles/wyze.dir/depend:
	cd /home/wuyz/learn/wyze/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wuyz/learn/wyze /home/wuyz/learn/wyze /home/wuyz/learn/wyze/build /home/wuyz/learn/wyze/build /home/wuyz/learn/wyze/build/CMakeFiles/wyze.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/wyze.dir/depend

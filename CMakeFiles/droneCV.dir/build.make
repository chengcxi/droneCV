# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 4.0

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/cheng/Downloads/DroneVision/droneCV

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/cheng/Downloads/DroneVision/droneCV

# Include any dependencies generated for this target.
include CMakeFiles/droneCV.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/droneCV.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/droneCV.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/droneCV.dir/flags.make

CMakeFiles/droneCV.dir/codegen:
.PHONY : CMakeFiles/droneCV.dir/codegen

CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o: CMakeFiles/droneCV.dir/flags.make
CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o: DISOpticalFlow.cpp
CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o: CMakeFiles/droneCV.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/cheng/Downloads/DroneVision/droneCV/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o -MF CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o.d -o CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o -c /home/cheng/Downloads/DroneVision/droneCV/DISOpticalFlow.cpp

CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/cheng/Downloads/DroneVision/droneCV/DISOpticalFlow.cpp > CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.i

CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/cheng/Downloads/DroneVision/droneCV/DISOpticalFlow.cpp -o CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.s

# Object files for target droneCV
droneCV_OBJECTS = \
"CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o"

# External object files for target droneCV
droneCV_EXTERNAL_OBJECTS =

droneCV: CMakeFiles/droneCV.dir/DISOpticalFlow.cpp.o
droneCV: CMakeFiles/droneCV.dir/build.make
droneCV: CMakeFiles/droneCV.dir/compiler_depend.ts
droneCV: /usr/lib/libopencv_gapi.so.4.11.0
droneCV: /usr/lib/libopencv_stitching.so.4.11.0
droneCV: /usr/lib/libopencv_alphamat.so.4.11.0
droneCV: /usr/lib/libopencv_aruco.so.4.11.0
droneCV: /usr/lib/libopencv_bgsegm.so.4.11.0
droneCV: /usr/lib/libopencv_bioinspired.so.4.11.0
droneCV: /usr/lib/libopencv_ccalib.so.4.11.0
droneCV: /usr/lib/libopencv_cvv.so.4.11.0
droneCV: /usr/lib/libopencv_dnn_objdetect.so.4.11.0
droneCV: /usr/lib/libopencv_dnn_superres.so.4.11.0
droneCV: /usr/lib/libopencv_dpm.so.4.11.0
droneCV: /usr/lib/libopencv_face.so.4.11.0
droneCV: /usr/lib/libopencv_freetype.so.4.11.0
droneCV: /usr/lib/libopencv_fuzzy.so.4.11.0
droneCV: /usr/lib/libopencv_hdf.so.4.11.0
droneCV: /usr/lib/libopencv_hfs.so.4.11.0
droneCV: /usr/lib/libopencv_img_hash.so.4.11.0
droneCV: /usr/lib/libopencv_intensity_transform.so.4.11.0
droneCV: /usr/lib/libopencv_line_descriptor.so.4.11.0
droneCV: /usr/lib/libopencv_mcc.so.4.11.0
droneCV: /usr/lib/libopencv_quality.so.4.11.0
droneCV: /usr/lib/libopencv_rapid.so.4.11.0
droneCV: /usr/lib/libopencv_reg.so.4.11.0
droneCV: /usr/lib/libopencv_rgbd.so.4.11.0
droneCV: /usr/lib/libopencv_saliency.so.4.11.0
droneCV: /usr/lib/libopencv_signal.so.4.11.0
droneCV: /usr/lib/libopencv_stereo.so.4.11.0
droneCV: /usr/lib/libopencv_structured_light.so.4.11.0
droneCV: /usr/lib/libopencv_superres.so.4.11.0
droneCV: /usr/lib/libopencv_surface_matching.so.4.11.0
droneCV: /usr/lib/libopencv_tracking.so.4.11.0
droneCV: /usr/lib/libopencv_videostab.so.4.11.0
droneCV: /usr/lib/libopencv_viz.so.4.11.0
droneCV: /usr/lib/libopencv_wechat_qrcode.so.4.11.0
droneCV: /usr/lib/libopencv_xfeatures2d.so.4.11.0
droneCV: /usr/lib/libopencv_xobjdetect.so.4.11.0
droneCV: /usr/lib/libopencv_xphoto.so.4.11.0
droneCV: /usr/lib/libopencv_shape.so.4.11.0
droneCV: /usr/lib/libopencv_highgui.so.4.11.0
droneCV: /usr/lib/libopencv_datasets.so.4.11.0
droneCV: /usr/lib/libopencv_plot.so.4.11.0
droneCV: /usr/lib/libopencv_text.so.4.11.0
droneCV: /usr/lib/libopencv_ml.so.4.11.0
droneCV: /usr/lib/libopencv_phase_unwrapping.so.4.11.0
droneCV: /usr/lib/libopencv_optflow.so.4.11.0
droneCV: /usr/lib/libopencv_ximgproc.so.4.11.0
droneCV: /usr/lib/libopencv_video.so.4.11.0
droneCV: /usr/lib/libopencv_videoio.so.4.11.0
droneCV: /usr/lib/libopencv_imgcodecs.so.4.11.0
droneCV: /usr/lib/libopencv_objdetect.so.4.11.0
droneCV: /usr/lib/libopencv_calib3d.so.4.11.0
droneCV: /usr/lib/libopencv_dnn.so.4.11.0
droneCV: /usr/lib/libopencv_features2d.so.4.11.0
droneCV: /usr/lib/libopencv_flann.so.4.11.0
droneCV: /usr/lib/libopencv_photo.so.4.11.0
droneCV: /usr/lib/libopencv_imgproc.so.4.11.0
droneCV: /usr/lib/libopencv_core.so.4.11.0
droneCV: CMakeFiles/droneCV.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/cheng/Downloads/DroneVision/droneCV/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable droneCV"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/droneCV.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/droneCV.dir/build: droneCV
.PHONY : CMakeFiles/droneCV.dir/build

CMakeFiles/droneCV.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/droneCV.dir/cmake_clean.cmake
.PHONY : CMakeFiles/droneCV.dir/clean

CMakeFiles/droneCV.dir/depend:
	cd /home/cheng/Downloads/DroneVision/droneCV && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/cheng/Downloads/DroneVision/droneCV /home/cheng/Downloads/DroneVision/droneCV /home/cheng/Downloads/DroneVision/droneCV /home/cheng/Downloads/DroneVision/droneCV /home/cheng/Downloads/DroneVision/droneCV/CMakeFiles/droneCV.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/droneCV.dir/depend


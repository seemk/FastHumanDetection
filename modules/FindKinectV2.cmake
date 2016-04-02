# Find KinectV2 SDK
#
# This module defines:
#
#  KINECTV2_FOUND
#  KINECTV2_LIBRARY
#  KINECTV2_INCLUDE_DIR
#

find_library(KINECTV2_LIBRARY Kinect20
    PATH_SUFFIXES Lib/x64 Lib/x86)
set(KINECTV2_LIBRARY_NEEDED KINECTV2_LIBRARY)

# Include dir
find_path(KINECTV2_INCLUDE_DIR
    NAMES Kinect.h
    PATH_SUFFIXES Inc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("KinectV2" DEFAULT_MSG
    ${KINECTV2_LIBRARY_NEEDED}
    KINECTV2_INCLUDE_DIR)
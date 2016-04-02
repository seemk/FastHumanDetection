#pragma once

#include "fhd_frame_source.h"

struct IKinectSensor;
struct IDepthFrameReader;

struct fhd_kinect_source : fhd_frame_source {
  fhd_kinect_source();
  ~fhd_kinect_source();

  const uint16_t* get_frame() override;
  int current_frame() override 
  int total_frames() const override { return -1; }

  IKinectSensor* kinect = nullptr;
  IDepthFrameReader* depth_reader = nullptr;

  std::vector<uint16_t> depth_buffer;
};

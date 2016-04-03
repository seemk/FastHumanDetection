#include "fhd_kinect_source.h"
#include <Kinect.h>
#include <assert.h>
#include <float.h>

namespace {

  template <class Interface>
  void interface_release(Interface*& i) {
    if (i != nullptr) {
      i->Release();
      i = nullptr;
    }
  }

  bool read_depth_data(fhd_kinect_source* source) {
    bool got_data = false;

    IDepthFrame* frame = nullptr;

    HRESULT hr = source->depth_reader->AcquireLatestFrame(&frame);

    if (SUCCEEDED(hr)) {
      uint32_t buffer_size = 0;
      uint16_t* buffer = nullptr;
      hr = frame->AccessUnderlyingBuffer(&buffer_size, &buffer);

      if (SUCCEEDED(hr)) {
        source->depth_buffer.clear();

        const uint16_t* begin = (const uint16_t*)buffer;
        const uint16_t* end = (const uint16_t*)buffer + buffer_size;

        source->depth_buffer.insert(source->depth_buffer.end(), begin, end);

        got_data = true;
      }
    }

    interface_release(frame);

    return got_data;
  }

}

fhd_kinect_source::fhd_kinect_source() {
  HRESULT hr = GetDefaultKinectSensor(&kinect);
  if (FAILED(hr)) {
    fprintf(stderr, "Failed to get the kinect sensor\n");
  }

  if (kinect) {
    hr = kinect->Open();
    if (FAILED(hr)) {
      fprintf(stderr, "Failed to open kinect sensor\n");
    }

    IDepthFrameSource* depth_source = nullptr;
    hr = kinect->get_DepthFrameSource(&depth_source);

    if (SUCCEEDED(hr)) {
      hr = depth_source->OpenReader(&depth_reader);
    }

    interface_release(depth_source);
  }
}

fhd_kinect_source::~fhd_kinect_source() {
  if (kinect) {
    kinect->Close();
    interface_release(kinect);
  }
}

const uint16_t* fhd_kinect_source::get_frame() {
  if (read_depth_data(this)) {
    return depth_buffer.data();
  }

  return nullptr;
}
#include "../fhd.h"
#include "../fhd_classifier.h"
#include "../fhd_candidate_db.h"
#include "../tools/fhd_debug_frame_source.h"
#include <memory>
#include <stdio.h>

#if WIN32
#include "../tools/fhd_kinect_source.h"
#endif

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("usage: example_detect classifier.nn\n");
    return 1;
  }

  fhd_classifier* classifier = fhd_classifier_create(argv[1]);
  if (!classifier) {
    printf("invalid classifier file\n");
    return 1;
  }

  fhd_context detector;
  fhd_context_init(&detector, 512, 424, 8, 8);
  detector.classifier = classifier;

#if WIN32
  auto source = std::unique_ptr<fhd_frame_source>(new fhd_kinect_source());
#else
  // TODO: Replace with a small DB
  auto source = std::unique_ptr<fhd_frame_source>(new fhd_debug_frame_source());
#endif

  for (int i = 0; i < 10; i++) {
    fhd_run_pass(&detector, source->get_frame());
    for (int j = 0; j < detector.candidates_len; j++) {
      fhd_candidate* candidate = &detector.candidates[j];
      int x = candidate->depth_position.x;
      int y = candidate->depth_position.y;
      printf("(%d %d), (%d, %d); weight: %f\n", x, y,
             x + candidate->depth_position.width,
             y + candidate->depth_position.height, candidate->weight);
    }
  }

  fhd_classifier_destroy(classifier);
  fhd_context_destroy(&detector);
  return 0;
}

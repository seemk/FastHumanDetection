#include "fhd_classifier.h"
#include "fhd_candidate.h"
#include <fann.h>

struct fhd_classifier {
  fann* nn;
};

fhd_classifier* fhd_classifier_create(const char* nn_file) {
  fann* nn = fann_create_from_file(nn_file);
  if (nn) {
    fhd_classifier* classifier = (fhd_classifier*)calloc(1, sizeof(fhd_classifier));
    classifier->nn = nn;
    return classifier;
  }

  return NULL;
}

float fhd_classify(const fhd_classifier* classifier, const fhd_candidate* candidate) {
  float* output = fann_run(classifier->nn, candidate->features);
  return output[0];
}

void fhd_classifier_destroy(fhd_classifier* classifier) {
  if (classifier) {
    fann_destroy(classifier->nn);
    free(classifier);
  }
}

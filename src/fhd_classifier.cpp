#include "fhd_classifier.h"
#include "fhd_candidate.h"
#include <fann.h>

void fhd_classifier_init(fhd_classifier* classifier, const char* nn_file) {
  classifier->nn = fann_create_from_file(nn_file);
}

float fhd_classify(fhd_classifier* classifier, const fhd_candidate* candidate) {
  float* output = fann_run(classifier->nn, candidate->features);
  return output[0];
}

void fhd_classifier_destroy(fhd_classifier* classifier) {
  if (classifier->nn) {
    fann_destroy(classifier->nn);
  }
}

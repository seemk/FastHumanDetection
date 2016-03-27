#pragma once

struct fhd_candidate;
struct fann;

struct fhd_classifier {
  fann* nn;
};

void fhd_classifier_init(fhd_classifier* classifier, const char* nn_file);
float fhd_classify(fhd_classifier* classifier, const fhd_candidate* candidate);
void fhd_classifier_destroy(fhd_classifier* classifier);

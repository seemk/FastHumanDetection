#pragma once

struct fhd_candidate;
struct fhd_classifier;

fhd_classifier* fhd_classifier_create(const char* nn_file);
float fhd_classify(fhd_classifier* classifier, const fhd_candidate* candidate);
void fhd_classifier_destroy(fhd_classifier* classifier);

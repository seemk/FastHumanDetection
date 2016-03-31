#include <fann.h>
#include <parallel_fann.h>
#include "../fhd_candidate_db.h"
#include "../fhd_candidate.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {

  if (argc < 3) {
    printf("usage: test_classifier testdata.db classifier.nn\n");
    return 1;
  }

  const char* db_file = argv[1];
  const char* nn_file = argv[2];

  fhd_candidate_db db;
  fhd_candidate_db_init(&db, db_file);

  const int features_length = 3780;
  int count = fhd_candidate_db_get_count(&db);

  float* features = (float*)calloc(count * features_length, sizeof(float));
  float* output = (float*)calloc(count, sizeof(float));

  fhd_result* training_data = (fhd_result*)calloc(count, sizeof(fhd_result));

  for (int i = 0; i < count; i++) {
    training_data[i].num_features = features_length;
    training_data[i].features = &features[features_length * i];
  }

  fhd_candidate_db_get_features(&db, training_data, count);
  fhd_candidate_db_close(&db);

  for (int i = 0; i < count; i++) {
    float v = float(training_data[i].human * 2 - 1);
    output[i] = v;
  }

  fann* nn = fann_create_from_file(nn_file);
  fann_train_data* fann_td = fann_create_train_array(count, features_length, features, 1, output);
  float r = fann_test_data(nn, fann_td);
  printf("MSE: %f\n", r);
  fann_destroy_train(fann_td);
  fann_destroy(nn);

  nn = fann_create_from_file(nn_file);

  int true_positives = 0;
  int true_negatives = 0;
  int false_positives = 0;
  int false_negatives = 0;
  const float weight_threshold = 1.f;

  for (int i = 0; i < count; i++) {
    bool expected = output[i] >= weight_threshold;
    bool real = fann_run(nn, training_data[i].features)[0] >= weight_threshold;

    if (expected && real) {
      true_positives++;
    }

    if (!expected && !real) {
      true_negatives++;
    }

    if (!expected && real) {
      false_positives++;
    }

    if (expected && !real) {
      false_negatives++;
    }
  }

  float tp = float(true_positives);
  float fp = float(false_positives);
  float fn = float(false_negatives);

  printf("Precision %.3f\n", (tp / (tp + fp)));
  printf("Recall: %.3f\n", (tp / (tp + fn)));

  free(training_data);
  fann_destroy(nn);

  return 0;
}

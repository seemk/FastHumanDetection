#include <fann.h>
#include <parallel_fann.h>
#include "../fhd_candidate_db.h"
#include "../fhd_candidate.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

volatile bool running = true;

void interrupt_handler(int) {
  printf("Quitting...\n");
  running = false;
}

int main(int argc, char** argv) {

  if (argc < 3) {
    printf("usage: train_classifier train_data.db result.nn\n");
    return 1;
  }

  struct sigaction action;
  action.sa_handler = interrupt_handler;
  sigaction(SIGINT, &action, NULL);

  const char* db_file = argv[1];
  const char* nn_file = argv[2];

  printf("%s %s\n", db_file, nn_file);

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

  free(training_data);

  fann* nn = fann_create_standard(2, features_length, 1);
  fann_set_activation_function_output(nn, FANN_SIGMOID_SYMMETRIC);

  fann_train_data* fann_td = fann_create_train_array(count, features_length, features, 1, output);
  
  for (int i = 1; i <= 40000; i++) {
    float err = fann_train_epoch_irpropm_parallel(nn, fann_td, 8);
    printf("Epoch: %d MSE %.10f\n", i, err);
    if (err < 0.001f || !running) break;
  }

  fann_save(nn, nn_file);

  fann_destroy_train(fann_td);
  fann_destroy(nn);

  return 0;
}

#include "fhd_training.h"
#include <algorithm>
#include <fann.h>
#include <parallel_fann.h>
#include <string.h>
#include "../fhd_candidate_db.h"
#include "../fhd_candidate.h"

fhd_training::fhd_training()
  : epoch(0) {
  output_classifier_buf.fill(0); 
}

fhd_training::~fhd_training() {
  running = false;
  if (training_thread.joinable()) {
    training_thread.join();
  }
}

void fhd_training_set_errortext(fhd_training* t, std::string err) {
  std::lock_guard<std::mutex>(t->access_lock);
  t->error = std::move(err);
}

void fhd_train(fhd_training* t) {
  t->epoch = 0;
  int epochs = t->max_epochs;
  int target_mse = t->target_mse;
  std::string out_file = t->output_classifier_buf.data();
  const int features_length = 3780;
  fhd_candidate_db db;
  if (!fhd_candidate_db_init(&db, t->candidate_database_name.c_str())) {
    fhd_training_set_errortext(t, "Invalid candidate database");
    return;
  }

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

  fann_train_data* fann_td =
      fann_create_train_array(count, features_length, features, 1, output);

  int threads = std::max(std::thread::hardware_concurrency(), 1U);

  t->running = true;
  for (int i = 1; i <= epochs; i++) {
    t->epoch = i;
    float err = fann_train_epoch_irpropm_parallel(nn, fann_td, threads);
    std::unique_lock<std::mutex> lock(t->access_lock);
    std::rotate(t->errors.begin(), t->errors.begin() + 1, t->errors.end());
    t->errors.back() = err;
    lock.unlock();
    if (err < target_mse || !t->running) break;

  }

  fann_save(nn, out_file.c_str());

  fann_destroy_train(fann_td);
  fann_destroy(nn);
}

bool fhd_training_start(fhd_training* t) {
  if (strlen(t->output_classifier_buf.data()) == 0) {
    fhd_training_set_errortext(t, "Output file name required");
    return false;
  }

  if (t->training_thread.joinable()) {
    t->training_thread.join();
  }

  t->error.clear();
  t->errors = std::vector<float>(256, 0.f);
  t->training_thread = std::thread(fhd_train, t);

  return true;
}

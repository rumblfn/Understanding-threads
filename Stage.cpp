#include "Stage.h"

Stage::Stage(std::string stage_name, int min_workers_count, int max_workers_count, int capacity)
    : _stage_name(std::move(stage_name)),
      _min_workers_count(min_workers_count),
      _max_workers_count(max_workers_count),
      _buffer_capacity(capacity),
      workers_count(min_workers_count) {
  pthread_mutex_init(&mutex, nullptr);
  pthread_cond_init(&cond_full, nullptr);
  pthread_cond_init(&cond_empty, nullptr);
}

Stage::~Stage() {
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond_full);
  pthread_cond_destroy(&cond_empty);
}

bool Stage::SetWorkersCount(int new_workers_count) {
  if (new_workers_count < _min_workers_count || new_workers_count > _max_workers_count) {
    std::cerr << "Invalid number of workers for Stage: " << _stage_name << ". Should be between "
              << _min_workers_count << " and " << _max_workers_count << "." << std::endl;
    return false;
  }

  workers_count = new_workers_count;
  return true;
}

void Stage::SetNextStage(Stage* stage) {
  next_stage = stage;
}

void Stage::AddToBuffer(int item) {
  pthread_mutex_lock(&mutex);
  while (buffer.size() >= _buffer_capacity) {
    pthread_cond_wait(&cond_full, &mutex);
  }
  buffer.push(item);
  pthread_cond_signal(&cond_empty);
  pthread_mutex_unlock(&mutex);
}

int Stage::GetFromBuffer() {
  int item;
  pthread_mutex_lock(&mutex);
  while (buffer.empty()) {
    pthread_cond_wait(&cond_empty, &mutex);
  }
  item = buffer.front();
  buffer.pop();
  pthread_cond_signal(&cond_full);
  pthread_mutex_unlock(&mutex);
  return item;
}

int Stage::GetBufferSize() {
  pthread_mutex_lock(&mutex);
  int size = static_cast<int>(buffer.size());
  pthread_mutex_unlock(&mutex);
  return size;
}

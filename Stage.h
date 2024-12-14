#ifndef IHW_4__STAGE_H_
#define IHW_4__STAGE_H_

#include <iostream>
#include <string>
#include <queue>

#include <pthread.h>

/**
 * Class for simulating the work of workshop.
 */
class Stage {
 public:
  Stage(std::string stage_name, int min_workers_count, int max_workers_count, int capacity);
  ~Stage();

  bool SetWorkersCount(int new_workers_count);
  void SetNextStage(Stage* stage);
  void AddToBuffer(int item);
  int GetFromBuffer();
  int GetBufferSize();

  int workers_count;
  Stage* next_stage{};
  std::queue<int> buffer;
 private:
  std::string _stage_name;
  pthread_mutex_t mutex{};
  pthread_cond_t  cond_full{}, cond_empty{};

  int _buffer_capacity;
  int _min_workers_count;
  int _max_workers_count;
};

#endif //IHW_4__STAGE_H_

#include <iostream>
#include <fstream>
#include <random>
#include <ctime>

#include <unistd.h>
#include "Stage.h"

// Configure for your needs.
int MAX_PINS_COUNT = 4;
int MAX_WORK_SECONDS = 40;

pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

std::string output_filename;

/**
 * Thread safe cout and writing to a file.
 * @param message – to print.
 */
void SafeCout(const std::string &message) {
  pthread_mutex_lock(&cout_mutex);
  std::cout << message << std::endl;
  pthread_mutex_unlock(&cout_mutex);

  if (!output_filename.empty()) {
    pthread_mutex_lock(&file_mutex);
    std::ofstream outfile;
    outfile.open(output_filename, std::ios_base::app);
    if (outfile.is_open()) {
      outfile << message << std::endl;
      outfile.close();
    } else {
      std::cerr << "Error opening file: " << output_filename << std::endl;
    }
    pthread_mutex_unlock(&file_mutex);
  }
}

/**
 * Thread safe random.
 * @param min
 * @param max
 * @return random value in range [min, max]
 */
int RandInRange(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);

  return dis(gen);
}

/**
 * Simulates the work of first workshop.
 * @param arg – pointer to stage.
 * @return
 */
void *stage1_worker(void *arg) {
  Stage *stage = static_cast<Stage *>(arg);
  while (true) {
    int pin = stage->GetFromBuffer();

    // Processing pin.
    int processing_time = RandInRange(1, 3);
    sleep(processing_time);

    // If pin is not curved it will be processed by second workshop.
    bool is_curved = RandInRange(0, 1);

    if (!is_curved) {
      if (stage->next_stage) {
        stage->next_stage->AddToBuffer(pin);
        SafeCout("[Stage 1 Worker] Passed a pin {" + std::to_string(pin) + "} to Stage 2. Buffer size: "
                     + std::to_string(stage->next_stage->GetBufferSize()));
      }
    } else {
      SafeCout("[Stage 1 Worker] Discarded a curved pin {" + std::to_string(pin) + "}.");
    }
  }
}

/**
 * Simulates the work of second workshop.
 * @param arg — pointer to stage.
 * @return
 */
void *stage2_worker(void *arg) {
  Stage *stage = static_cast<Stage *>(arg);
  while (true) {
    int pin = stage->GetFromBuffer();
    SafeCout("[Stage 2 Worker] Received a pin {" + std::to_string(pin) + "} from Stage 1. Buffer size: "
                 + std::to_string(stage->GetBufferSize()));

    int sharpening_time = RandInRange(1, 4);
    sleep(sharpening_time);

    // Any pin will be added to next stage.
    // There is no check for bad and good pin.
    if (stage->next_stage) {
      stage->next_stage->AddToBuffer(pin);
      SafeCout("[Stage 2 Worker] Passed a sharpened pin {" + std::to_string(pin) + "} to Stage 3. Buffer size: "
                   + std::to_string(stage->next_stage->GetBufferSize()));
    }
  }
}

/**
 * Simulates the work of third workshop.
 * @param arg – pointer to stage.
 * @return
 */
void *stage3_worker(void *arg) {
  Stage *stage = static_cast<Stage *>(arg);
  while (true) {
    int pin = stage->GetFromBuffer();
    SafeCout("[Stage 3 Worker] Received a sharpened pin {" + std::to_string(pin) + "} from Stage 2. Buffer size: "
                 + std::to_string(stage->GetBufferSize()));

    int quality_control_time = RandInRange(1, 2);
    sleep(quality_control_time);

    // Check for qulity control.
    bool is_quality_acceptable = RandInRange(0, 1);
    if (!is_quality_acceptable) {
      if (stage->next_stage) {
        stage->next_stage->AddToBuffer(pin);
        SafeCout("[Stage 3 Worker] Pin {" + std::to_string(pin)
                     + "} not enough quality. The pin returned to Stage 2. Buffer size: "
                     + std::to_string(stage->next_stage->GetBufferSize()));
      }
    } else {
      SafeCout("[Stage 3 Worker] quality control OK on a pin {" + std::to_string(pin) + "}.");
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " K L M output_filename(optional)" << std::endl;
    return -1;
  }

  // Configuring stages that simulate appropriate workshop.
  Stage stage1("1", 1, 3, MAX_PINS_COUNT);
  Stage stage2("2", 1, 5, 10);
  Stage stage3("3", 1, 2, 10);

  // Binding next stage for current.
  stage1.SetNextStage(&stage2);
  stage2.SetNextStage(&stage3);
  stage3.SetNextStage(&stage2);

  // Check for provided arguments.
  if (!stage1.SetWorkersCount(std::stoi(argv[1]))
      || !stage2.SetWorkersCount(std::stoi(argv[2]))
      || !stage3.SetWorkersCount(std::stoi(argv[3]))) {
    return -1;
  }

  if (argc >= 5) {
    output_filename = argv[4];
  }

  // Configuring pins for stages.
  // Buffer of head stage uses for initial pins that should be processed.
  for (int i = 0; i < MAX_PINS_COUNT; ++i) {
    stage1.AddToBuffer(i);
  }

  // Initializing workers threads.
  pthread_t stage1_workers[stage1.workers_count];
  pthread_t stage2_workers[stage2.workers_count];
  pthread_t stage3_workers[stage3.workers_count];

  // Creating threads and binding workshop for it.
  for (int i = 0; i < stage1.workers_count; ++i) {
    pthread_create(&stage1_workers[i], nullptr, stage1_worker, &stage1);
  }

  for (int i = 0; i < stage2.workers_count; ++i) {
    pthread_create(&stage2_workers[i], nullptr, stage2_worker, &stage2);
  }

  for (int i = 0; i < stage3.workers_count; ++i) {
    pthread_create(&stage3_workers[i], nullptr, stage3_worker, &stage3);
  }

  // It's like workers day off.
  sleep(MAX_WORK_SECONDS);

  return 0;
}
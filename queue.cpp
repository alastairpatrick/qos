#include "queue.h"
#include "queue.internal.h"

#include "core_migrator.h"
#include "semaphore.h"
#include "mutex.h"
#include "mutex.internal.h"
#include "task.h"
#include "time.h"

#include <algorithm>
#include <cstring>

qos_queue_t* qos_new_queue(int32_t capacity) {
  auto queue = new qos_queue_t;
  qos_init_queue(queue, new char[capacity], capacity);
  return queue;
}

void qos_init_queue(qos_queue_t* queue, void* buffer, int32_t capacity) {
  assert(capacity > 0);
  
  qos_init_semaphore(&queue->read_semaphore, 0);
  qos_init_semaphore(&queue->write_semaphore, capacity);
  qos_init_mutex(&queue->mutex);

  queue->capacity = capacity;
  queue->read_idx = 0;
  queue->write_idx = 0;
  queue->buffer = (char*) buffer;
}

bool STRIPED_RAM qos_write_queue(qos_queue_t* queue, const void* data, int32_t size, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  // This isn't actually needed but it reduces the number of task migrations.
  qos_core_migrator migrator(queue->mutex.core);

  if (!qos_acquire_semaphore(&queue->write_semaphore, size, timeout)) {
    return false;
  }

  if (!qos_acquire_mutex(&queue->mutex, timeout)) {
    qos_release_semaphore(&queue->write_semaphore, size);
    return false;
  }

  auto copy_bytes = std::min(size, queue->capacity - queue->write_idx);
  memcpy(&queue->buffer[queue->write_idx], data, copy_bytes);

  queue->write_idx += copy_bytes;
  if (queue->write_idx == queue->capacity) {
    queue->write_idx = size - copy_bytes;
    memcpy(queue->buffer, copy_bytes + (const char*) data, queue->write_idx);
  }

  qos_release_mutex(&queue->mutex);

  qos_release_semaphore(&queue->read_semaphore, size);

  return true;
}

bool STRIPED_RAM qos_read_queue(qos_queue_t* queue, void* data, int32_t size, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  // This isn't actually needed but it reduces the number of task migrations.
  qos_core_migrator migrator(queue->mutex.core);

  if (!qos_acquire_semaphore(&queue->read_semaphore, size, timeout)) {
    return false;
  }

  if (!qos_acquire_mutex(&queue->mutex, timeout)) {
    qos_release_semaphore(&queue->write_semaphore, size);
    return false;
  }

  auto copy_bytes = std::min(size, queue->capacity - queue->read_idx);
  memcpy(data, &queue->buffer[queue->read_idx], copy_bytes);

  queue->read_idx += copy_bytes;
  if (queue->read_idx == queue->capacity) {
    queue->read_idx = size - copy_bytes;
    memcpy(copy_bytes + (char*) data, queue->buffer, queue->read_idx);
  }

  qos_release_mutex(&queue->mutex);

  qos_release_semaphore(&queue->write_semaphore, size);

  return true;
}

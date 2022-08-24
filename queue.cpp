#include "queue.h"
#include "queue.internal.h"

#include "scheduler.h"
#include "semaphore.h"
#include "mutex.h"

#include <algorithm>
#include <cstring>

Queue* new_queue(int32_t capacity) {
  auto queue = new Queue;
  init_queue(queue, new char[capacity], capacity);
  return queue;
}

void init_queue(Queue* queue, void* buffer, int32_t capacity) {
  assert(capacity > 0);
  
  init_semaphore(&queue->read_semaphore, 0);
  init_semaphore(&queue->write_semaphore, capacity);
  init_mutex(&queue->mutex);

  queue->capacity = capacity;
  queue->read_idx = 0;
  queue->write_idx = 0;
  queue->buffer = (char*) buffer;
}

bool STRIPED_RAM write_queue(Queue* queue, const void* data, int32_t size, tick_t timeout) {
  if (!acquire_semaphore(&queue->write_semaphore, size, timeout)) {
    return false;
  }

  if (!acquire_mutex(&queue->mutex, timeout)) {
    release_semaphore(&queue->write_semaphore, size);
    return false;
  }

  auto copy_bytes = std::min(size, queue->capacity - queue->write_idx);
  memcpy(&queue->buffer[queue->write_idx], data, copy_bytes);

  queue->write_idx += copy_bytes;
  if (queue->write_idx == queue->capacity) {
    queue->write_idx = size - copy_bytes;
    memcpy(queue->buffer, copy_bytes + (const char*) data, queue->write_idx);
  }

  release_mutex(&queue->mutex);

  release_semaphore(&queue->read_semaphore, size);

  return true;
}

bool STRIPED_RAM read_queue(Queue* queue, void* data, int32_t size, tick_t timeout) {
  if (!acquire_semaphore(&queue->read_semaphore, size, timeout)) {
    return false;
  }

  if (!acquire_mutex(&queue->mutex, timeout)) {
    release_semaphore(&queue->write_semaphore, size);
    return false;
  }

  auto copy_bytes = std::min(size, queue->capacity - queue->read_idx);
  memcpy(data, &queue->buffer[queue->read_idx], copy_bytes);

  queue->read_idx += copy_bytes;
  if (queue->read_idx == queue->capacity) {
    queue->read_idx = size - copy_bytes;
    memcpy(copy_bytes + (char*) data, queue->buffer, queue->read_idx);
  }

  release_mutex(&queue->mutex);

  release_semaphore(&queue->write_semaphore, size);

  return true;
}

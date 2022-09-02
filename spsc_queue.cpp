#include "spsc_queue.internal.h"

#include "time.h"

#include <algorithm>
#include <cstring>

qos_spsc_queue_t* qos_new_spsc_queue(int32_t capacity, int32_t producer_core, int32_t consumer_core) {
  auto queue = new qos_spsc_queue_t;
  qos_init_spsc_queue(queue, new char[capacity], capacity, producer_core, consumer_core);
  return queue;
}

void qos_init_spsc_queue(qos_spsc_queue_t* queue, void* buffer, int32_t capacity, int32_t producer_core, int32_t consumer_core) {
  assert(capacity > 0);

  queue->buffer = (char*) buffer;
  queue->capacity = capacity;

  if (producer_core < 0) {
    producer_core = get_core_num();
  }
  qos_init_event(&queue->producer_event, producer_core);
  queue->producer_head = queue->producer_tail = 0;

  if (consumer_core < 0) {
    consumer_core = get_core_num();
  }
  qos_init_event(&queue->consumer_event, consumer_core);
  queue->consumer_head = queue->consumer_tail = 0;
}

static int32_t STRIPED_RAM max_producer_avail(qos_spsc_queue_t* queue) {
  auto avail = queue->consumer_tail - queue->producer_head;
  if (avail <= 0) {
    avail += queue->capacity;
  }
  assert(avail <= queue->capacity);
  return avail;
}

void STRIPED_RAM internal_write_spsc_queue(qos_spsc_queue_t* queue, const void* data, int32_t size) {
  queue->producer_head += size;
  if (queue->producer_head >= queue->capacity) {
    queue->producer_head -= queue->capacity;
  }

  auto copy_size = std::min(size, queue->capacity - queue->producer_tail);
  memcpy(&queue->buffer[queue->producer_tail], data, copy_size);
  memcpy(queue->buffer, copy_size + (const char*) data, size - copy_size);

  queue->producer_tail = queue->producer_head;

  qos_signal_event(&queue->consumer_event);
}

bool STRIPED_RAM qos_write_spsc_queue(qos_spsc_queue_t* queue, const void* data, int32_t size, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  while (max_producer_avail(queue) < size) {
    if (!qos_await_event(&queue->producer_event, timeout)) {
      return false;
    }
  }

  internal_write_spsc_queue(queue, data, size);
  return true;
}

bool STRIPED_RAM qos_write_spsc_queue_from_isr(qos_spsc_queue_t* queue, const void* data, int32_t size) {
  if (max_producer_avail(queue) < size) {
    return false;
  }

  internal_write_spsc_queue(queue, data, size);
  return true;
}


static int32_t STRIPED_RAM max_consumer_avail(qos_spsc_queue_t* queue) {
  auto avail = queue->producer_tail - queue->consumer_head;
  if (avail < 0) {
    avail += queue->capacity;
  }
  assert(avail < queue->capacity);
  return avail;
}

void STRIPED_RAM internal_read_spsc_queue(qos_spsc_queue_t* queue, void* data, int32_t size) {
  queue->consumer_head += size;
  if (queue->consumer_head >= queue->capacity) {
    queue->consumer_head -= queue->capacity;
  }

  auto copy_size = std::min(size, queue->capacity - queue->consumer_tail);
  memcpy(data, &queue->buffer[queue->consumer_tail], copy_size);
  memcpy(copy_size + (char*) data, queue->buffer, size - copy_size);

  queue->consumer_tail = queue->consumer_head;

  qos_signal_event(&queue->producer_event);
}

bool STRIPED_RAM qos_read_spsc_queue(qos_spsc_queue_t* queue, void* data, int32_t size, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  while (max_consumer_avail(queue) < size) {
    if (!qos_await_event(&queue->consumer_event, timeout)) {
      return false;
    }
  }

  internal_read_spsc_queue(queue, data, size);
  return true;
}

bool STRIPED_RAM qos_read_spsc_queue_from_isr(qos_spsc_queue_t* queue, void* data, int32_t size) {
  if (max_consumer_avail(queue) < size) {
    return false;
  }

  internal_read_spsc_queue(queue, data, size);
  return true;
}

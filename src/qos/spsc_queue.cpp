#include "spsc_queue.internal.h"

#include "time.h"

#include <algorithm>
#include <cstring>

qos_spsc_queue_t* QOS_INITIALIZATION qos_new_spsc_queue(int32_t capacity, int32_t write_core, int32_t read_core) {
  auto queue = new qos_spsc_queue_t;
  qos_init_spsc_queue(queue, new char[capacity], capacity, write_core, read_core);
  return queue;
}

void QOS_INITIALIZATION qos_init_spsc_queue(qos_spsc_queue_t* queue, void* buffer, int32_t capacity, int32_t write_core, int32_t read_core) {
  assert(capacity > 0);

  queue->buffer = (char*) buffer;
  queue->capacity = capacity;

  if (write_core < 0) {
    write_core = get_core_num();
  }
  qos_init_event(&queue->write_event, write_core);
  queue->write_head = queue->write_tail = 0;

  if (read_core < 0) {
    read_core = get_core_num();
  }
  qos_init_event(&queue->read_event, read_core);
  queue->read_head = queue->read_tail = 0;
}

static int32_t QOS_HANDLER_MODE max_producer_avail(qos_spsc_queue_t* queue) {
  auto avail = queue->read_tail - queue->write_head;
  if (avail <= 0) {
    avail += queue->capacity;
  }
  assert(avail <= queue->capacity);
  return avail;
}

void QOS_HANDLER_MODE internal_write_spsc_queue(qos_spsc_queue_t* queue, const void* data, int32_t size) {
  queue->write_head += size;
  if (queue->write_head >= queue->capacity) {
    queue->write_head -= queue->capacity;
  }

  auto copy_size = std::min(size, queue->capacity - queue->write_tail);
  memcpy(&queue->buffer[queue->write_tail], data, copy_size);
  memcpy(queue->buffer, copy_size + (const char*) data, size - copy_size);

  queue->write_tail = queue->write_head;
}

int32_t qos_write_spsc_queue(qos_spsc_queue_t* queue, const void* data, int32_t min_size, int32_t max_size, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  int32_t avail;
  for (;;) {
    avail = max_producer_avail(queue);
    if (avail >= min_size) {
      break;
    }
    if (!qos_await_event(&queue->write_event, timeout)) {
      return -1;
    }
  }

  auto size = std::min(max_size, avail);
  internal_write_spsc_queue(queue, data, size);
  qos_signal_event(&queue->read_event);
  return size;
}

int32_t QOS_HANDLER_MODE qos_write_spsc_queue_from_isr(qos_spsc_queue_t* queue, const void* data, int32_t min_size, int32_t max_size) {
  auto avail = max_producer_avail(queue);
  if (avail < min_size) {
    return -1;
  }

  auto size = std::min(max_size, avail);
  internal_write_spsc_queue(queue, data, size);
  qos_signal_event_from_isr(&queue->read_event);
  return size;
}


static int32_t QOS_HANDLER_MODE max_consumer_avail(qos_spsc_queue_t* queue) {
  auto avail = queue->write_tail - queue->read_head;
  if (avail < 0) {
    avail += queue->capacity;
  }
  assert(avail < queue->capacity);
  return avail;
}

void QOS_HANDLER_MODE internal_read_spsc_queue(qos_spsc_queue_t* queue, void* data, int32_t size) {
  queue->read_head += size;
  if (queue->read_head >= queue->capacity) {
    queue->read_head -= queue->capacity;
  }

  auto copy_size = std::min(size, queue->capacity - queue->read_tail);
  memcpy(data, &queue->buffer[queue->read_tail], copy_size);
  memcpy(copy_size + (char*) data, queue->buffer, size - copy_size);

  queue->read_tail = queue->read_head;
}

int32_t qos_read_spsc_queue(qos_spsc_queue_t* queue, void* data, int32_t min_size, int32_t max_size, qos_time_t timeout) {
  qos_normalize_time(&timeout);

  int32_t avail;
  for (;;) {
    avail = max_consumer_avail(queue);
    if (avail >= min_size) {
      break;
    }
    if (!qos_await_event(&queue->read_event, timeout)) {
      return -1;
    }
  }

  auto size = std::min(max_size, avail);
  internal_read_spsc_queue(queue, data, size);
  qos_signal_event(&queue->write_event);
  return size;
}

int32_t QOS_HANDLER_MODE qos_read_spsc_queue_from_isr(qos_spsc_queue_t* queue, void* data, int32_t min_size, int32_t max_size) {
  auto avail = max_consumer_avail(queue);
  if (avail < min_size) {
    return -1;
  }

  auto size = std::min(max_size, avail);
  internal_read_spsc_queue(queue, data, size);
  qos_signal_event_from_isr(&queue->write_event);
  return size;
}

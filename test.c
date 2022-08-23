#include "scheduler.inl.c"
#include "queue.h"

#include <assert.h>
#include <string.h>

struct Queue* g_queue;
struct Task* g_delay_task;
struct Task* g_producer_task1;
struct Task* g_producer_task2;
struct Task* g_consumer_task1;
struct Task* g_consumer_task2;

void do_delay_task() {
  for(;;) {
    sleep(1000);
  }
}

void do_producer_task1() {
  for(;;) {
    write_queue(g_queue, "hello", 6);
  }
}

void do_producer_task2() {
  for(;;) {
    write_queue(g_queue, "world", 6);
  }
}

void do_consumer_task1() {
  for(;;) {
    char buffer[10];
    memset(buffer, 0, sizeof(buffer));
    read_queue(g_queue, buffer, 6);
    assert(strcmp(buffer, "hello") == 0 || strcmp(buffer, "world") == 0);
  }
}

void do_consumer_task2() {
  for(;;) {
    char buffer[10];
    memset(buffer, 0, sizeof(buffer));
    read_queue(g_queue, buffer, 6);
    assert(strcmp(buffer, "hello") == 0 || strcmp(buffer, "world") == 0);
  }
}

int main() {
  g_queue = new_queue(100);

  g_delay_task = new_task(2, do_delay_task, 1024);
  g_producer_task1 = new_task(1, do_producer_task1, 1024);
  g_consumer_task1 = new_task(1, do_consumer_task1, 1024);
  g_producer_task2 = new_task(1, do_producer_task2, 1024);
  g_consumer_task2 = new_task(1, do_consumer_task2, 1024);

  start_scheduler();

  // Not reached.
  assert(false);

  return 0;
}

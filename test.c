#include "scheduler.h"
#include "queue.h"

#include <assert.h>
#include <string.h>

struct Queue* g_queue;

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

  new_task(1, do_producer_task1, 1024);
  new_task(1, do_producer_task2, 1024);
  new_task(1, do_consumer_task1, 1024);
  new_task(1, do_consumer_task2, 1024);

  start_scheduler();

  // Not reached.
  assert(false);

  return 0;
}

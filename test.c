#include "atomic.h"
#include "scheduler.h"
#include "mutex.h"
#include <assert.h>

struct Mutex* g_mutex;
atomic32_t g_count;

void do_task1() {
  for(;;) {
    acquire_mutex(g_mutex);
    g_count += 1;
    g_count -= 1;
    int count = g_count;
    release_mutex(g_mutex);
    assert(count == 0);
  }
}

void do_task2() {
  for(;;) {
    acquire_mutex(g_mutex);
    g_count += 1;
    g_count -= 1;
    int count = g_count;
    release_mutex(g_mutex);
    assert(count == 0);
  }
}

int main() {
  g_mutex = new_mutex();
  struct Task* task2 = new_task(1, do_task2, 1024);
  struct Task* task1 = new_task(1, do_task1, 1024);
  start_scheduler();

  // Not reached.
  assert(false);

  return 0;
}

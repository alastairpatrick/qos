#include "atomic.h"
#include "scheduler.h"
#include "semaphore.h"

#include <assert.h>

struct Semaphore* g_semaphore;
atomic32_t g_count;

void do_task1() {
  for(;;) {
    acquire_semaphore(g_semaphore, 1);
    g_count += 1;
    g_count -= 1;
    int count = g_count;
    release_semaphore(g_semaphore, 1);
    assert(count == 0);
  }
}

void do_task2() {
  for(;;) {
    acquire_semaphore(g_semaphore, 1);
    g_count += 1;
    g_count -= 1;
    int count = g_count;
    release_semaphore(g_semaphore, 1);
    assert(count == 0);
  }
}

int main() {
  g_semaphore = new_semaphore(1);
  struct Task* task2 = new_task(1, do_task2, 1024);
  struct Task* task1 = new_task(1, do_task1, 1024);
  start_scheduler();

  // Not reached.
  assert(false);

  return 0;
}

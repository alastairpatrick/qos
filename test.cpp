#include "atomic.h"
#include "scheduler.h"

#include <cassert>

int g_count;

void do_task1() {
  for(;;) {
    atomic_add(&g_count, 1);
  }
}

void do_task2() {
  for(;;) {
    atomic_add(&g_count, 1);
  }
}

int main() {
  auto task1 = new_task(1, do_task1, 1024);
  auto task2 = new_task(1, do_task2, 1024);
  start_scheduler();

  // Not reached.
  assert(false);

  return 0;
}

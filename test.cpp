#include "atomic.h"
#include "scheduler.h"

#include <cassert>

atomic32_t g_count;

void do_task1() {
  for(;;) {
    for (int i = 0; i < 1000; ++i) {
      atomic_add(&g_count, 1);
      atomic_add(&g_count, -1);
    }
    assert(g_count >= -2 && g_count <= 2);
    yield();
  }
}

void do_task2() {
  for(;;) {
    for (int i = 0; i < 1000; ++i) {
      atomic_add(&g_count, 1);
      atomic_add(&g_count, -1);
    }
    assert(g_count >= -2 && g_count <= 2);
    yield();
  }
}

int main() {
  auto task2 = new_task(1, do_task2, 1024);
  auto task1 = new_task(1, do_task1, 1024);
  start_scheduler();

  // Not reached.
  assert(false);

  return 0;
}

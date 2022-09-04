#ifndef QOS_CORE_MIGRATOR_H
#define QOS_CORE_MIGRATOR_H

#include "task.h"

struct qos_core_migrator {
  explicit qos_core_migrator(int32_t dest_core) {
    original_core = qos_migrate_core(dest_core);
  }

  ~qos_core_migrator() {
    qos_migrate_core(original_core);
  }

  qos_core_migrator(const qos_core_migrator&) = delete;
  qos_core_migrator& operator=(const qos_core_migrator&) = delete;

  int32_t original_core;
};

#endif  // QOS_CORE_MIGRATOR_H

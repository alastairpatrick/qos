#ifndef QOS_DIVIDE_H
#define QOS_DIVIDE_H

typedef struct qos_divmod_t {
  int32_t quotient;
  int32_t remainder;
} qos_divmod_t;

typedef struct qos_udivmod_t {
  uint32_t quotient;
  uint32_t remainder;
} qos_udivmod_t;

// These avoid the overhead of saving and restoring divider state in ISRs.
int32_t qos_div(int32_t dividend, int32_t divisor);
qos_divmod_t qos_divmod(int32_t dividend, int32_t divisor);
uint32_t qos_udiv(uint32_t dividend, uint32_t divisor);
qos_udivmod_t qos_udivmod(uint32_t dividend, uint32_t divisor);

#endif  // QOS_DIVIDE_H

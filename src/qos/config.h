#ifndef QOS_CONFIG_H
#define QOS_CONFIG_H

#ifndef QOS_TICK_1MHZ_SOURCE
#define QOS_TICK_1MHZ_SOURCE 1      // nominal 1MHz clock source
#endif

#ifndef QOS_TICK_MS
#define QOS_TICK_MS 10
#endif

#ifndef QOS_TICK_CYCLES
#if QOS_TICK_1MHZ_SOURCE
#define QOS_TICK_CYCLES (QOS_TICK_MS * 1000)
#else
#define QOS_TICK_CYCLES (QOS_TICK_MS * 125 * 1000)
#endif
#endif

#ifndef QOS_MAX_EVENTS_PER_CORE
#define QOS_MAX_EVENTS_PER_CORE 8
#endif

#endif  // QOS_CONFIG_H

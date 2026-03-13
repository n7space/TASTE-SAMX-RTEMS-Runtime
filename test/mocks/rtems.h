#ifndef RTEMS_H
#define RTEMS_H

#include <stdint.h>

/* Mock providing common RTEMS types to enable compilation */

#define RTEMS_SUCCESSFUL (1)
#define RTEMS_WAIT (2)
#define RTEMS_NO_TIMEOUT (3)
#define RTEMS_BINARY_SEMAPHORE (4)
#define RTEMS_INTERRUPT_UNIQUE (5)

typedef uint32_t rtems_interval;
typedef uint32_t rtems_vector_number;
typedef uint32_t rtems_id;
typedef uint32_t rtems_name;
typedef uint32_t rtems_status_code;
typedef uint32_t rtems_option;
typedef void (*rtems_interrupt_handler)(void*);

/* Build a 32-bit RTEMS name at compile time. Use as a constant initializer. */
#define rtems_build_name(a,b,c,d) \
	(((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))

/* Mocked API declarations */
const rtems_status_code rtems_semaphore_release(const rtems_id id);
const rtems_status_code rtems_semaphore_obtain(const rtems_id id, const uint32_t mode, const uint32_t timeout);

const rtems_status_code rtems_interrupt_handler_install(
  rtems_vector_number vector,
  const char *info,
  rtems_option options,
  rtems_interrupt_handler handler,
  void *arg
);

const rtems_status_code rtems_interrupt_vector_enable(const rtems_vector_number number);

const rtems_status_code rtems_semaphore_create(const rtems_name name,
    const uint32_t initial_value,
    const uint32_t type,
    const uint32_t ceiling,
    const rtems_id* ids);

const rtems_status_code rtems_task_wake_after(const rtems_interval interval);

#endif
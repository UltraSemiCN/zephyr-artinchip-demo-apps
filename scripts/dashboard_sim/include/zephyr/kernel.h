/* SPDX-License-Identifier: Apache-2.0 */
#ifndef DASHBOARD_HOST_KERNEL_H
#define DASHBOARD_HOST_KERNEL_H
/* Only utility macros are used by the shared UI source. No RTOS emulation. */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ARG_UNUSED(a) ((void)(a))
#endif

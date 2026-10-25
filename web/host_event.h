#ifndef WEB_HOST_EVENT_H
#define WEB_HOST_EVENT_H
#include <stdint.h>
/* Host notifications carry event keys, without reading guest memory. Missing
 * or already destroyed events are ignored; no event is implicitly created. */
int web_os_try_signal_event(uint64_t key);
#endif

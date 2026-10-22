#ifndef WEB_HOST_MUTEX_H
#define WEB_HOST_MUTEX_H
int web_mutex_import(Ctx *,const char *);
unsigned web_mutex_live_count(void);
unsigned web_mutex_peak_count(void);
#endif

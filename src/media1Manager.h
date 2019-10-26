#ifndef MEDIA1_MANAGER_H_
#define MEDIA1_MANAGER_H_

#include <gio/gio.h>

typedef void (*Media1RegisterObjectsCb)(GDBusConnection *connection, void *userData);
typedef GVariant* (*Media1GetPlayerProperyCb)(const char *propertyName, void *userData);

void media1Start(Media1RegisterObjectsCb regObjectsCb, Media1GetPlayerProperyCb getPropCb, void *userData);
void media1Shutdown(void);

#endif

#include "media1Manager.h"

#include "logging.h"

#include <assert.h>

static Media1RegisterObjectsCb registerObjectsCb;
static Media1GetPlayerProperyCb getPlayerPropertyCb;
static void *cbUserData;

static GDBusObjectManager *bluezObjectManager;
static GDBusProxy *media1Proxy;

static void onRegisterPlayerDone(GObject *sourceObject, GAsyncResult *res, gpointer userData) {
	GDBusProxy *proxy = G_DBUS_PROXY(sourceObject);

	GError *error = NULL;
	GVariant *ret = g_dbus_proxy_call_finish(proxy, res, &error);

	if (ret != NULL) {
		debug("Player is registered on a system bus");
		g_variant_unref(ret);
	} else {
		error("Failed to register player on a system bus: %s", error->message);
		g_error_free(error);
	}
}

static GVariant* getPlayerProperties(void) {
	assert(getPlayerPropertyCb);

	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	const char *playerProperties[] = {
		"PlaybackStatus",
		"LoopStatus",
		"Rate",
		"Shuffle",
		"Metadata",
		"Volume",
		"Position",
		"MinimumRate",
		"MaximumRate",
		"CanGoNext",
		"CanGoPrevious",
		"CanPlay",
		"CanPause",
		"CanSeek",
		"CanControl",
		NULL
	};

	for (const char **propertyNamePtr = playerProperties; *propertyNamePtr; propertyNamePtr++) {
		GVariant *value = getPlayerPropertyCb(*propertyNamePtr, cbUserData);

		if (!value) {
			continue;
		}

		g_variant_builder_add(&builder, "{sv}", *propertyNamePtr, value);
	}

	return g_variant_builder_end(&builder);
}

static void media1ProxyCallRegisterPlayer(GDBusProxy *proxy, GAsyncReadyCallback onRegisterPlayerDone) {
	g_dbus_proxy_call(proxy,
					  "RegisterPlayer",
					  g_variant_new ("(o@a{sv})",
									 "/org/mpris/MediaPlayer2",
									 getPlayerProperties()),
					  G_DBUS_CALL_FLAGS_NO_AUTO_START,
					  -1,
					  NULL,
					  onRegisterPlayerDone,
					  NULL);
}

static void onMedia1ProxyAppeared() {
	debug("Starting player registration");
	media1ProxyCallRegisterPlayer(media1Proxy, onRegisterPlayerDone);
}

static void onInterfaceAdded(GDBusObjectManager *manager,
							 GDBusObject *object,
							 GDBusInterface *interface,
							 gpointer userData) {
	debug("Added interface %s on object %s",
		  g_dbus_proxy_get_interface_name(G_DBUS_PROXY(interface)),
		  g_dbus_object_get_object_path(object));

	if (g_str_equal(g_dbus_proxy_get_interface_name(G_DBUS_PROXY(interface)), "org.bluez.Media1") &&
		!media1Proxy) {
		debug("Saved reference to org.bluez.Media1 proxy");
		media1Proxy = g_object_ref(G_DBUS_PROXY(interface));
		onMedia1ProxyAppeared();
	}
}

static void onInterfaceRemoved(GDBusObjectManager *manager,
							   GDBusObject *object,
							   GDBusInterface *interface,
							   gpointer userData) {
	debug("Removed interface %s on object %s",
		  g_dbus_proxy_get_interface_name(G_DBUS_PROXY(interface)),
		  g_dbus_object_get_object_path(object));

	if (g_str_equal(g_dbus_proxy_get_interface_name(G_DBUS_PROXY(interface)), "org.bluez.Media1") &&
		G_DBUS_PROXY(interface) == media1Proxy) {
		debug("Dropped org.bluez.Media1 proxy reference");
		g_object_unref(media1Proxy);
		media1Proxy = NULL;

		// When interface disappears, it automatically
		// unregisters previously registered players
		// No need to de-register player explicitly
	}
}

static void onEachInterface(gpointer data, gpointer userData) {
	GDBusObject *obj = G_DBUS_OBJECT(userData);
	GDBusInterface *interface = G_DBUS_INTERFACE(data);

	onInterfaceAdded(bluezObjectManager, obj, interface, NULL);

	g_object_unref(interface);
}

static void onEachObject(gpointer data, gpointer userData) {
	GDBusObject *obj = G_DBUS_OBJECT(data);

	GList *interfaces = g_dbus_object_get_interfaces(obj);
	g_list_foreach(interfaces, onEachInterface, obj);
	g_list_free(interfaces);

	g_object_unref(obj);
}

static void enumerateInterfaces(void) {
	GList *objects = g_dbus_object_manager_get_objects(bluezObjectManager);
	g_list_foreach(objects, onEachObject, NULL);
	g_list_free(objects);
}

static void onBluezObjectManagerCreated(GObject *sourceObject, GAsyncResult *res, gpointer userData) {
	GError *error = NULL;
	bluezObjectManager = g_dbus_object_manager_client_new_finish(res, &error);

	if (!bluezObjectManager) {
		error("Failed to create object manager for org.bluez: %s", error->message);
		g_error_free(error);
		return;
	}

	debug("Successfully created object manager for org.bluez");

	assert(registerObjectsCb);
	registerObjectsCb(g_dbus_object_manager_client_get_connection(G_DBUS_OBJECT_MANAGER_CLIENT(bluezObjectManager)),
					  cbUserData);

	g_signal_connect(G_OBJECT(bluezObjectManager), "interface-added", G_CALLBACK(onInterfaceAdded), NULL);
	g_signal_connect(G_OBJECT(bluezObjectManager), "interface-removed", G_CALLBACK(onInterfaceRemoved), NULL);

	enumerateInterfaces();
}

static void createBluezObjectManager(void) {
	g_dbus_object_manager_client_new_for_bus(G_BUS_TYPE_SYSTEM,
											 G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START,
											 "org.bluez",
											 "/",
											 NULL,
											 NULL,
											 NULL,
											 NULL,
											 onBluezObjectManagerCreated,
											 NULL);
}

void media1Start(Media1RegisterObjectsCb regObjectsCb, Media1GetPlayerProperyCb getPropCb, void *userData) {
	registerObjectsCb = regObjectsCb;
	getPlayerPropertyCb = getPropCb;
	cbUserData = userData;

	createBluezObjectManager();
}

void media1Shutdown(void)
{
	g_object_unref(media1Proxy);
	media1Proxy = NULL;
	g_object_unref(bluezObjectManager);
}

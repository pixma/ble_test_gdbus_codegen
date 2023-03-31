/**
 * @file main.cpp
 * @author Annim banerjee (annim.banerjee@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-31
 * 
 * @copyright Copyright (c) 2023
 * 
 */
//////////////////////////////////////////
#include "main.h"
//////////////////////////////////////////
static GVariant *acquireWriteOptions = g_variant_new("a{sv}", NULL); // create empty options
static GVariant *out_fd = NULL;
static int fDescriptor = -1;
static guint16 *out_mtu = NULL;
GUnixFDList *fd_list;
//////////////////////////////////////////
GMainLoop *loop;
GError *error = NULL;
GDBusConnection *connection;
GDBusProxy *proxy;
guint signal_handler_id_dbus_properties;
guint signal_handler_id_dbus_object_manager_interfaces_added;
guint signal_handler_id_dbus_object_manager_interfaces_removed;
guint signal_handler_id_dbus_device_properties_changed;
guint signal_handler_id_dbus_service_to_write_properties_change;
guint signal_handler_id_dbus_service_to_read_properties_change;
//////////////////////////////////////////
static OrgBluezAdapter1 *bleAdapter;
static OrgBluezDevice1 *bleDeviceInstance;
static OrgBluezGattCharacteristic1 *bleGattCharacteristicsServiceToWrite;
static OrgFreedesktopDBusProperties *bleOrgFreedesktopDBusProperties_of_GattCharacteristic_ServiceToWrite;
static OrgFreedesktopDBusProperties *bleOrgFreedesktopDBusProperties_of_GattCharacteristic_ServiceToRead;
static OrgBluezGattCharacteristic1 *bleGattCharacteristicsServiceToRead;
static OrgFreedesktopDBusProperties *bleOrgFreedesktopDBusProperties;
static OrgFreedesktopDBusProperties *bleOrgFreedesktop_DiscoveredDevice_DBusProperties;
static OrgFreedesktopDBusObjectManager *bleOrgFreedesktopDBusObjectManager;
//////////////////////////////////////////
bluetoothDevice btDevice;
static char bFalgDeviceIdentified = FALSE;
static char bFlagDeviceTrusted = FALSE;
static char bFlagDeviceConnected = FALSE;
static char bFlagIsDiscovering = FALSE;
static char bFlagServicesResolved = FALSE;
static char bFlagAcquireWrite = FALSE;
static char bFlagNotifyOn = FALSE;
//////////////////////////////////////////
void bluez_property_value(
    const gchar *key,
    GVariant *value)
{
    const gchar *type = g_variant_get_type_string(value);

    g_print("\t[Type:%s]:", type);
    g_print("%s : ", key);
    switch (*type)
    {
    case 'o':
    case 's':
        g_print("%s\n", g_variant_get_string(value, NULL));
        break;
    case 'b':
        g_print("%d\n", g_variant_get_boolean(value));
        break;
    case 'u':
        g_print("%d\n", g_variant_get_uint32(value));
        break;
    case 'h':
        g_print("%d\n", g_variant_get_int32(value));
        break;
    case 'a':
        /* TODO Handling only 'as', but not array of dicts */
        if (g_strcmp0(type, "as") == 0)
        {
            const gchar *uuid;
            GVariantIter i;

            g_print("\n");
            g_variant_iter_init(&i, value);
            while (g_variant_iter_next(&i, "s", &uuid))
                g_print("%s\n", uuid);
        }
        break;
    default:
        g_print("Other\n");
        break;
    }
}
//////////////////////////////////////////
static void cleanup_handler(int signo)
{
    if (signo == SIGINT)
    {
        g_print("received SIGINT\n");
        g_main_loop_quit(loop);
    }
}
//////////////////////////////////////////
void on_gatt_characteristic_write_to_properties_change(OrgFreedesktopDBusProperties *object,
                                                       const gchar *arg_interface,
                                                       GVariant *arg_changed_properties,
                                                       const gchar *const *arg_invalidated_properties)
{

    LOG_F(INFO, "on_gatt_characteristic_write_to_properties_change for interface: %s\n", arg_interface);
    LOG_F(INFO, "\n%s | Signature: %s", g_variant_print(arg_changed_properties, TRUE), g_variant_get_type_string(arg_changed_properties));

    if (g_variant_is_of_type(arg_changed_properties, G_VARIANT_TYPE("a{sv}")))
    {
        GVariantIter iter;
        gchar *key;
        GVariant *value;

        g_variant_iter_init(&iter, arg_changed_properties);

        while (g_variant_iter_next(&iter, "{sv}", &key, &value))
        {
            gchar *value_str = g_variant_print(value, FALSE);
            printf("%s: %s\n", key, value_str);

            bluez_property_value(key, value); //
            if (!g_strcmp0(key, "WriteAcquired"))
            {
                LOG_F(INFO, "Gatt WriteAcquired: %s", g_variant_get_boolean(value) ? "on" : "off");
                fd_list = g_unix_fd_list_new();
                g_unix_fd_list_append(fd_list, fDescriptor, NULL);                
            }

            // g_free(value_str);
            // g_free(key);
            // g_variant_unref(value);
        }
    }
}
//////////////////////////////////////////
void on_gatt_characteristic_read_from_properties_change(OrgFreedesktopDBusProperties *object,
                                                        const gchar *arg_interface,
                                                        GVariant *arg_changed_properties,
                                                        const gchar *const *arg_invalidated_properties)
{

    LOG_F(INFO, "on_gatt_characteristic_read_from_properties_change for interface: %s\n", arg_interface);
    LOG_F(INFO, "\n%s | Signature: %s", g_variant_print(arg_changed_properties, TRUE), g_variant_get_type_string(arg_changed_properties));

    if (g_variant_is_of_type(arg_changed_properties, G_VARIANT_TYPE("a{sv}")))
    {
        GVariantIter iter;
        gchar *key;
        GVariant *value;

        g_variant_iter_init(&iter, arg_changed_properties);

        while (g_variant_iter_next(&iter, "{sv}", &key, &value))
        {
            gchar *value_str = g_variant_print(value, FALSE);
            printf("%s: %s\n", key, value_str);

            bluez_property_value(key, value);
            if (!g_strcmp0(key, "Notifying"))
            {
                LOG_F(INFO, "Gatt is Notifying \"%s\"\n", g_variant_get_boolean(value) ? "on" : "off");
                bFlagNotifyOn = g_variant_get_boolean(value) ? TRUE : FALSE;
            }
            // g_free(value_str);
            // g_free(key);
            // g_variant_unref(value);
        }
    }

    if (bFlagNotifyOn)
    {
        bFlagNotifyOn = FALSE;
        // now try to AcquireWrite fd to Write Data to BLE node from this application.
        LOG_F(INFO, "Now trying to AcquireWrite fd to Write Data to BLE node from this application.");
        if (!org_bluez_gatt_characteristic1_call_acquire_write_sync(
                bleGattCharacteristicsServiceToWrite,
                acquireWriteOptions,
                &out_fd,
                out_mtu,
                NULL,
                &error))
        {
            LOG_F(ERROR, "Unable to AcquireWrite: Error: %s", error->message);
        }
        else
        {
            fDescriptor = g_variant_get_handle(out_fd);
            LOG_F(INFO, "Write handle Locked...Hopefully.");
            // perform write operation using the stored file descriptor
            char writeData[24] = "Hello, BLE!";
            size_t writeLen = strlen(writeData);
            ssize_t bytesWritten = write(fDescriptor, writeData, writeLen);
            if (bytesWritten == -1)
            {
                LOG_F(ERROR, "Error writing to GATT characteristic");
                return;
            }

            LOG_F(INFO, "Wrote %zu bytes to GATT characteristic\n", bytesWritten);
        }
    }
}
//////////////////////////////////////////
void On_discovered_device_properties_change(OrgFreedesktopDBusProperties *object,
                                            const gchar *arg_interface,
                                            GVariant *arg_changed_properties,
                                            const gchar *const *arg_invalidated_properties)
{

    LOG_F(INFO, "Discovered Device PropertiesChanged signal received for interface: %s\n", arg_interface);
    LOG_F(INFO, "\n%s | Signature: %s", g_variant_print(arg_changed_properties, TRUE), g_variant_get_type_string(arg_changed_properties));

    if (g_variant_is_of_type(arg_changed_properties, G_VARIANT_TYPE("a{sv}")))
    {
        GVariantIter iter;
        gchar *key;
        GVariant *value;

        g_variant_iter_init(&iter, arg_changed_properties);

        while (g_variant_iter_next(&iter, "{sv}", &key, &value))
        {
            gchar *value_str = g_variant_print(value, FALSE);
            printf("%s: %s\n", key, value_str);

            bluez_property_value(key, value);
            if (!g_strcmp0(key, "Trusted"))
            {

                bFlagDeviceTrusted = g_variant_get_boolean(value) ? TRUE : FALSE;
                btDevice.nTrusted = (int)bFlagDeviceTrusted;
                g_print("Trusted \"%s\"\n", g_variant_get_boolean(value) ? "True" : "False");
            }
            else if (!g_strcmp0(key, "Connected"))
            {

                bFlagDeviceConnected = g_variant_get_boolean(value) ? TRUE : FALSE;
                btDevice.nConnected = (int)bFlagDeviceConnected;
                g_print("Connected : \"%s\"\n", g_variant_get_boolean(value) ? "True" : "False");
            }
            else if (!g_strcmp0(key, "ServicesResolved"))
            {
                bFlagServicesResolved = g_variant_get_boolean(value) ? TRUE : FALSE;
                btDevice.nServicesResolved = (int)bFlagServicesResolved;
                g_print("Services Resolved : \"%s\"\n", g_variant_get_boolean(value) ? "True" : "False");
            }
        }
    }

    if ((bFlagDeviceTrusted == TRUE) && (bFlagDeviceConnected == FALSE))
    {
        bFlagDeviceTrusted = FALSE;
        LOG_F(INFO, "[ %s ] is now trusted.\n", btDevice.sAddress);
        LOG_F(INFO, "Trying to connect to [%s:%s]...\n", btDevice.sName, btDevice.sAddress);

        // Get Device  Interface  Proxy quickly here.
        bleDeviceInstance = org_bluez_device1_proxy_new_sync(
            connection,
            G_DBUS_PROXY_FLAGS_NONE,
            BLUEZ_ORG_PATH,
            btDevice.sDBusPath,
            NULL,
            &error);

        if (bleDeviceInstance == NULL)
        {
            LOG_F(ERROR, "Failed to create/get bleDeviceInstance proxy object: %s", error->message);
        }
        if (!org_bluez_device1_call_connect_sync(bleDeviceInstance, NULL, &error))
        {
            LOG_F(ERROR, "Device Call method failed: %s", error->message);
        }
        else
        {
            LOG_F(INFO, "Device Call method : OK.\n");
        }
    }

    if ((bFlagDeviceConnected == TRUE) && (bFlagServicesResolved == TRUE))
    {
        // Fow now, making this section to run only for once.
        bFlagDeviceConnected = bFlagServicesResolved = FALSE;

        LOG_F(INFO, "Printing again Full Details:\n");
        LOG_F(INFO, "btDevice.sName: Device Name:\t\t%s", btDevice.sName);
        LOG_F(INFO, "btDevice.sAddress: Device Address:\t\t%s", btDevice.sAddress);
        LOG_F(INFO, "btDevice.sAddressType: Device AddressType:\t\t%s", btDevice.sAddressType);
        LOG_F(INFO, "btDevice.sDBusPath: Device DBUS path:\t\t%s", btDevice.sDBusPath);
        LOG_F(INFO, "btDevice.sUUID_WriteData: Device Write Data UUID:\t\t%s", btDevice.sUUID_WriteData);
        LOG_F(INFO, "btDevice.sObjectPath_WriteData: Device Write Data ObjectPath:\t\t%s", btDevice.sObjectPath_WriteData);
        LOG_F(INFO, "btDevice.sUUID_ReceiveData: Device Receive Data UUID:\t\t%s", btDevice.sUUID_ReceiveData);
        LOG_F(INFO, "btDevice.sObjectPath_ReceiveData: Device Receive Data ObjectPath:\t\t%s", btDevice.sObjectPath_ReceiveData);
        LOG_F(INFO, "btDevice.sUUID_ReceiveData_Descriptor: Device Receive Data Descriptor UUID:\t\t%s", btDevice.sUUID_ReceiveData_Descriptor);
        LOG_F(INFO, "btDevice.sObjectPath_ReceiveData_Descriptor: Device Receive Data Descriptor ObjectPath:\t\t%s", btDevice.sObjectPath_ReceiveData_Descriptor);

        LOG_F(INFO, "Signals to be subscribed for:-");
        LOG_F(INFO, "btDevice.sObjectPath_WriteData: %s", btDevice.sObjectPath_WriteData);
        LOG_F(INFO, "btDevice.sObjectPath_ReceiveData: %s", btDevice.sObjectPath_ReceiveData);

        // Subscribe to Signals for Write GATT Characteristics Properties Change
        // Signal of To Write Service.

        // Before that, get the proxy for it.
        bleGattCharacteristicsServiceToWrite = org_bluez_gatt_characteristic1_proxy_new_sync(
            connection,
            G_DBUS_PROXY_FLAGS_NONE,
            BLUEZ_ORG_PATH,
            btDevice.sObjectPath_WriteData,
            NULL,
            &error);

        // Before that, get the proxy for it.
        bleGattCharacteristicsServiceToRead = org_bluez_gatt_characteristic1_proxy_new_sync(
            connection,
            G_DBUS_PROXY_FLAGS_NONE,
            BLUEZ_ORG_PATH,
            btDevice.sObjectPath_ReceiveData,
            NULL,
            &error);

        // Before that, get the proxy for DBusProperties.
        bleOrgFreedesktopDBusProperties_of_GattCharacteristic_ServiceToWrite = org_freedesktop_dbus_properties_proxy_new_sync(
            connection,
            G_DBUS_PROXY_FLAGS_NONE,
            BLUEZ_ORG_PATH,
            btDevice.sObjectPath_WriteData,
            NULL,
            &error);

        // Before that, get the proxy for DBusProperties.
        bleOrgFreedesktopDBusProperties_of_GattCharacteristic_ServiceToRead = org_freedesktop_dbus_properties_proxy_new_sync(
            connection,
            G_DBUS_PROXY_FLAGS_NONE,
            BLUEZ_ORG_PATH,
            btDevice.sObjectPath_ReceiveData,
            NULL,
            &error);

        signal_handler_id_dbus_service_to_write_properties_change = g_signal_connect(
            bleOrgFreedesktopDBusProperties_of_GattCharacteristic_ServiceToWrite,
            "properties-changed",
            G_CALLBACK(on_gatt_characteristic_write_to_properties_change),
            NULL);

        signal_handler_id_dbus_service_to_read_properties_change = g_signal_connect(
            bleOrgFreedesktopDBusProperties_of_GattCharacteristic_ServiceToRead,
            "properties-changed",
            G_CALLBACK(on_gatt_characteristic_read_from_properties_change),
            NULL);

        // Enable notify to that GATT Characteristic from Which This Application will read or StreamIN the Data over Bluetooth.
        if (!org_bluez_gatt_characteristic1_call_start_notify_sync(
                bleGattCharacteristicsServiceToRead,
                NULL,
                &error))
        {
            LOG_F(ERROR, "org_bluez_gatt_characteristic1_call_start_notify_sync Notify Failed. Error: %s", error->message);
        }
    }
}
//////////////////////////////////////////
void on_properties_changed(OrgFreedesktopDBusProperties *object,
                           const gchar *arg_interface,
                           GVariant *arg_changed_properties,
                           const gchar *const *arg_invalidated_properties)
{
    /* Handle the signal here */
    // Print the interface name
    LOG_F(INFO, "PropertiesChanged signal received for interface: %s\n", arg_interface);
    LOG_F(INFO, "\n%s | Signature: %s", g_variant_print(arg_changed_properties, TRUE), g_variant_get_type_string(arg_changed_properties));

    if (g_variant_is_of_type(arg_changed_properties, G_VARIANT_TYPE("a{sv}")))
    {
        GVariantIter iter;
        gchar *key;
        GVariant *value;

        g_variant_iter_init(&iter, arg_changed_properties);

        while (g_variant_iter_next(&iter, "{sv}", &key, &value))
        {
            gchar *value_str = g_variant_print(value, FALSE);
            printf("%s: %s\n", key, value_str);

            if (!g_strcmp0(key, "Powered"))
            {
                LOG_F(INFO, "Adapter is Powered \"%s\"\n", g_variant_get_boolean(value) ? "on" : "off");
            }
            if (!g_strcmp0(key, "Discovering"))
            {
                LOG_F(INFO, "Adapter scan \"%s\"\n", g_variant_get_boolean(value) ? "on" : "off");
                bFlagIsDiscovering = g_variant_get_boolean(value) ? TRUE : FALSE;
                btDevice.nDeviceDiscovered = (int)bFlagIsDiscovering; // Although it is int type but writing it as boolean for friendly reading.
            }
            // g_free(value_str);
            // g_free(key);
            // g_variant_unref(value);
        }
    }

    // Print the invalidated properties
    if (arg_invalidated_properties)
    {
        g_print("Invalidated properties:");
        for (int i = 0; arg_invalidated_properties[i]; i++)
        {
            LOG_F(INFO, " %s", arg_invalidated_properties[i]);
        }
        g_print("\n");
    }

    LOG_F(INFO, "bFalgDeviceIdentified: %d", bFalgDeviceIdentified);

    if (bFalgDeviceIdentified)
    {
        // Let this portion runs only for once for now.
        bFalgDeviceIdentified = FALSE;
        // Get DBus Properties Interface SubPath Proxy.
        bleOrgFreedesktop_DiscoveredDevice_DBusProperties = org_freedesktop_dbus_properties_proxy_new_sync(
            connection,
            G_DBUS_PROXY_FLAGS_NONE,
            BLUEZ_ORG_PATH,
            btDevice.sDBusPath,
            NULL,
            &error);

        if (bleOrgFreedesktop_DiscoveredDevice_DBusProperties == NULL)
        {
            LOG_F(ERROR, "Failed to create/get bleOrgFreedesktop_DiscoveredDevice_DBusProperties proxy object: %s", error->message);
            g_clear_error(&error);
        }

        // Subscribe to Device property Changes.
        signal_handler_id_dbus_device_properties_changed = g_signal_connect(
            bleOrgFreedesktop_DiscoveredDevice_DBusProperties,
            "properties-changed",
            G_CALLBACK(On_discovered_device_properties_change),
            NULL);
        g_print("Signal subscribed to Device's DBus Properties changes: [ %s ]\n", btDevice.sDBusPath);
        // Set Trusted Field of Device : True in DBus Properties.
        if (!org_freedesktop_dbus_properties_call_set_sync(
                bleOrgFreedesktop_DiscoveredDevice_DBusProperties,
                INTERFACE_DEVICES,
                "Trusted",
                g_variant_new_variant(g_variant_new_boolean(TRUE)),
                NULL,
                &error))
        {
            LOG_F(ERROR, "Not able to update Trusted Field YES to the device.");
        }
        else
        {
            LOG_F(INFO, "Device Trusted : YES.");
            btDevice.nTrusted = TRUE;
            bFlagDeviceTrusted = TRUE;
        }
    }
}

//////////////////////////////////////////
void on_interfaces_added(
    OrgFreedesktopDBusObjectManager *object,
    const gchar *arg_object,
    GVariant *arg_interfaces)
{

    const gchar *interface_name = g_variant_get_type_string(arg_interfaces);

    LOG_F(INFO, "org.freedesktop.DBus.ObjectManager Related signals Captured.");
    LOG_F(INFO, "Interface added: %s .", arg_object);
    strcpy(btDevice.sDBusPath, arg_object);

    if (strcmp(interface_name, "a{sa{sv}}") != 0)
    {
        LOG_F(INFO, "arg_interfaces is not of type: a{sa{sv}}.");
        LOG_F(INFO, "\n%s", g_variant_print(arg_interfaces, TRUE));
        return;
    }

    GVariantIter *interfaces_iter;
    gchar *interface_name_str;
    GVariant *interface_properties;
    const gchar *property_name;
    GVariant *prop_val;
    GVariantIter i;

    LOG_F(INFO, "\n%s", g_variant_print(arg_interfaces, TRUE));

    g_variant_get(arg_interfaces, "a{sa{sv}}", &interfaces_iter);
    while (g_variant_iter_next(interfaces_iter, "{&s@a{sv}}", &interface_name_str, &interface_properties))
    {
        if (strcmp(interface_name_str, "org.bluez.Device1") == 0)
        {
            LOG_F(INFO, "interface_properties\n%s", g_variant_print(interface_properties, TRUE));

            g_variant_iter_init(&i, interface_properties);
            while (g_variant_iter_next(&i, "{&sv}", &property_name, &prop_val))
            {
                if (strcmp(property_name, "Address") == 0)
                {
                    strcpy(btDevice.sAddress, g_variant_get_string(prop_val, NULL));
                }
                if (strcmp(property_name, "Name") == 0)
                {
                    strcpy(btDevice.sName, g_variant_get_string(prop_val, NULL));
                }
                if (strcmp(property_name, "AddressType") == 0)
                {
                    strcpy(btDevice.sAddressType, g_variant_get_string(prop_val, NULL));
                }
                bluez_property_value(property_name, prop_val);
            }
            g_variant_unref(prop_val);
        }
        else
        {
            g_print("Interface\t[ %s ]\n", interface_name);
            g_print("Object Path\t[ %s ]\n", arg_object);

            g_variant_iter_init(&i, interface_properties);
            while (g_variant_iter_next(&i, "{&sv}", &property_name, &prop_val))
            {
                if (
                    (strcmp(property_name, "UUID") == 0) &&
                    (strcmp(NODE_CENTRAL_WRITE_DATA_CHANNEL_UUID, g_variant_get_string(prop_val, NULL)) == 0))
                {
                    g_print("Tracked the Write Data UUID: [%s]\n", g_variant_get_string(prop_val, NULL));
                    strcpy(btDevice.sUUID_WriteData, g_variant_get_string(prop_val, NULL));
                    strcpy(btDevice.sObjectPath_WriteData, arg_object);
                }
                else if (
                    (strcmp(property_name, "UUID") == 0) &&
                    (strcmp(NODE_CENTRAL_RECEIVE_DATA_CHANNEL_UUID, g_variant_get_string(prop_val, NULL)) == 0))
                {
                    g_print("Tracked the Receive Data UUID: [%s]\n", g_variant_get_string(prop_val, NULL));
                    strcpy(btDevice.sUUID_ReceiveData, g_variant_get_string(prop_val, NULL));
                    strcpy(btDevice.sObjectPath_ReceiveData, arg_object);
                }
                else if (
                    (strcmp(interface_name, INTERFACE_GATT_DESCRIPTOR) == 0) &&
                    (strcmp(property_name, "UUID") == 0) &&
                    (strcmp(NODE_CENTRAL_RECEIVE_DATA_CHANNEL_FILE_DESCRIPTOR, g_variant_get_string(prop_val, NULL)) == 0))
                {
                    g_print("Tracked the Receive Data GATT Descriptor: [%s]\n\rObject Path: [ %s ]",
                            g_variant_get_string(prop_val, NULL),
                            arg_object);

                    if (!(strlen(btDevice.sUUID_ReceiveData_Descriptor) > 0))
                        strcpy(btDevice.sUUID_ReceiveData_Descriptor, g_variant_get_string(prop_val, NULL));

                    if (!(strlen(btDevice.sObjectPath_ReceiveData_Descriptor) > 0))
                        strcpy(btDevice.sObjectPath_ReceiveData_Descriptor, arg_object);
                }
                bluez_property_value(property_name, prop_val);
            }

            g_print("\n\r---------------------------\n\r");
        }
    }

    // g_free(interface_name_str);
    // g_variant_unref(interface_properties);
    // g_variant_iter_free(interfaces_iter);

    if (bFlagIsDiscovering)
    {
        LOG_F(INFO, "Found The Device:[ %s ]", btDevice.sName);
        LOG_F(INFO, "Device SSID[ %s ]", btDevice.sName);
        LOG_F(INFO, "Device MAC Address[ %s ]", btDevice.sAddress);
        LOG_F(INFO, "Device AddressType[ %s ]", btDevice.sAddressType);
        LOG_F(INFO, "Device Object DBUS Path[ %s ]", btDevice.sDBusPath);
        bFalgDeviceIdentified = TRUE;

        LOG_F(INFO, "Stopping Discovery...");

        if (!org_bluez_adapter1_call_stop_discovery_sync(bleAdapter, NULL, &error))
        {
            LOG_F(ERROR, "StopDiscovery API Call Failed:\nError: %s", error->message);
        }
        else
        {
            LOG_F(INFO, "StopDiscovery API Called : OK.");
        }
    }
}
//////////////////////////////////////////
void on_interfaces_removed(
    OrgFreedesktopDBusObjectManager *object,
    const gchar *arg_object,
    GVariant *arg_interfaces)
{
    LOG_F(INFO, "Interfaces removed: %s .", arg_object);
    // LOG_F(INFO, "\n%s",g_variant_print(arg_interfaces, TRUE));
}
//////////////////////////////////////////
int bluez_set_discovery_filter()
{

    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(b, "{sv}", "Transport", g_variant_new_string("auto"));
    g_variant_builder_add(b, "{sv}", "RSSI", g_variant_new_int16(-g_ascii_strtod("100", NULL)));
    g_variant_builder_add(b, "{sv}", "DuplicateData", g_variant_new_boolean(FALSE));
    GVariantBuilder *u = g_variant_builder_new(G_VARIANT_TYPE_STRING_ARRAY);
    g_variant_builder_add(u, "s", NODE_BLE_SENSOR_UUID);
    g_variant_builder_add(b, "{sv}", "UUIDs", g_variant_builder_end(u));
    GVariant *device_dict = g_variant_builder_end(b);

    LOG_F(INFO, "Setting Discovery Filter...");

    g_clear_error(&error);

    if (!org_bluez_adapter1_call_set_discovery_filter_sync(
            bleAdapter,
            device_dict,
            NULL,
            &error))
    {
        LOG_F(ERROR, "Not able to set discovery filter: \n%s", error->message);
        g_clear_error(&error);
        g_variant_builder_unref(u);
        g_variant_builder_unref(b);
        return 0;
    }
    else
    {
        LOG_F(INFO, "Discovery Filters Set: OK");
    }

    // g_clear_error(&error);
    g_variant_builder_unref(u);
    g_variant_builder_unref(b);
    return 1;
}
//////////////////////////////////////////
int main(int argc, char **argv)
{

    if (signal(SIGINT, cleanup_handler) == SIG_ERR)
    {
        g_print("can't catch SIGINT\n");
    }

    loguru::init(argc, argv); // Construct a new loguru::init object

    // Throw exceptions instead of aborting on CHECK fails:
    loguru::set_fatal_handler([](const loguru::Message &message)
                              { throw std::runtime_error(std::string(message.prefix) + message.message); });

    LOG_F(INFO, "BLE Test with Codegen Program: Init");

    memset(btDevice.sDBusPath, 0, DBUS_PATH_BUF);
    memset(btDevice.sAddress, 0, BLE_CHAR_BUFF);
    memset(btDevice.sAddressType, 0, BLE_CHAR_BUFF);
    memset(btDevice.sName, 0, BLE_CHAR_BUFF);

    memset(btDevice.sUUID_ReceiveData, 0, BLE_UUID_BUFF);
    memset(btDevice.sObjectPath_ReceiveData, 0, DBUS_PATH_BUF);
    memset(btDevice.sUUID_ReceiveData_Descriptor, 0, BLE_UUID_BUFF);
    memset(btDevice.sObjectPath_ReceiveData_Descriptor, 0, DBUS_PATH_BUF);
    memset(btDevice.sUUID_WriteData, 0, BLE_UUID_BUFF);
    memset(btDevice.sObjectPath_WriteData, 0, DBUS_PATH_BUF);

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL)
    {
        printf("Error connecting to the system bus: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Get new Loop context.
    loop = g_main_loop_new(NULL, FALSE);

    // Get adapter Interface SubPath Proxy.
    bleAdapter = org_bluez_adapter1_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        BLUEZ_ORG_PATH,
        ADAPTER_OBJECT_PATH,
        NULL,
        &error);

    if (bleAdapter == NULL)
    {
        g_error("Failed to create/get bleAdapter proxy object: %s", error->message);
        g_clear_error(&error);
        return 1;
    }

    // Get DBus Properties Interface SubPath Proxy.
    bleOrgFreedesktopDBusProperties = org_freedesktop_dbus_properties_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        BLUEZ_ORG_PATH,
        ADAPTER_OBJECT_PATH,
        NULL,
        &error);

    if (bleOrgFreedesktopDBusProperties == NULL)
    {
        g_error("Failed to create/get bleOrgFreedesktopDBusProperties proxy object: %s", error->message);
        g_clear_error(&error);
        return 1;
    }

    // Get DBus ObjectManager Interface Proxy.
    bleOrgFreedesktopDBusObjectManager = org_freedesktop_dbus_object_manager_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        BLUEZ_ORG_PATH,
        BLUEZ_ORG_OBJECT_ROOT_PATH,
        NULL,
        &error);
    if (bleOrgFreedesktopDBusObjectManager == NULL)
    {
        g_error("Failed to create/get bleOrgFreedesktopDBusObjectManager proxy object: %s", error->message);
        g_clear_error(&error);
        return 1;
    }

    signal_handler_id_dbus_object_manager_interfaces_added = g_signal_connect(
        bleOrgFreedesktopDBusObjectManager,
        "interfaces-added",
        G_CALLBACK(on_interfaces_added),
        NULL);

    signal_handler_id_dbus_object_manager_interfaces_removed = g_signal_connect(
        bleOrgFreedesktopDBusObjectManager,
        "interfaces-removed",
        G_CALLBACK(on_interfaces_removed),
        NULL);

    signal_handler_id_dbus_properties = g_signal_connect(
        bleOrgFreedesktopDBusProperties,
        "properties-changed",
        G_CALLBACK(on_properties_changed),
        NULL);

    LOG_F(INFO, "Signals: InterfacesAdded, InterfacesRemoved, DBus Properties Changed: Connected");

    LOG_F(INFO, "Adapter Power: Setting to ON...");
    // Set Power On : Adapter Properties.
    if (!org_freedesktop_dbus_properties_call_set_sync(
            bleOrgFreedesktopDBusProperties,
            ADAPTER_INTERFACE,
            "Powered",
            g_variant_new_variant(g_variant_new_boolean(TRUE)),
            NULL,
            &error))
    {
        LOG_F(ERROR, "Not able to enable/power on the adapter");
    }
    else
    {
        LOG_F(INFO, "Bluetooth Adapter enable/power : ON.");
    }

    g_clear_error(&error);
    // Get All Paired or Known Device Before going ahead.

    // Set Discovery Filter.
    bluez_set_discovery_filter();

    // Start Discovery Now
    if (!org_bluez_adapter1_call_start_discovery_sync(bleAdapter, NULL, &error))
    {
        LOG_F(ERROR, "StartDiscovery API Call Failed:\nError: %s", error->message);
    }
    else
    {
        LOG_F(INFO, "StartDiscovery API Called : OK.");
    }

    // Now run the loop - context.
    g_main_loop_run(loop);

    g_usleep(100);

    g_main_loop_quit(loop);

    // Free resources and exit
    if (proxy != NULL)
        g_object_unref(proxy);

    if (connection != NULL)
        g_object_unref(connection);

    return 0;
}
//////////////////////////////////////////
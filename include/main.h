#ifndef MAIN_H_
#define MAIN_H_

#include <string>
#include <fstream>
#include <memory>
#include <cstdio>
#include <cstdarg>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "loguru.hpp"
#include <unistd.h>
#include <gio/gunixfdlist.h>
#include <cstdlib>
#include <string.h>
#include <iostream>
#include <libconfig.h++>
#include <cassert>
#include <glib.h>
#include <gio/gio.h>

#include "ble_app_client.h"

#define BLUEZ_ORG_PATH  "org.bluez"
#define BLUEZ_ORG_OBJECT_ROOT_PATH  "/"
#define ADAPTER_OBJECT_PATH "/org/bluez/hci0"
#define ADAPTER_INTERFACE   "org.bluez.Adapter1"
#define INTERFACE_DEVICES "org.bluez.Device1"
#define INTERFACE_GATT_CHARACTERISTICS "org.bluez.GattCharacteristic1"
#define INTERFACE_GATT_DESCRIPTOR "org.bluez.GattDescriptor1"

#define BT_ADDRESS_STRING_SIZE 18
#define BLE_CHAR_BUFF 24
#define BLE_UUID_BUFF 48
#define DBUS_PATH_BUF 128
#define NODE_BLE_SENSOR_UUID "0000abf0-0000-1000-8000-00805f9b34fb"
#define NODE_BLE_SENSOR_UPSTREAM_CHANNEL_UUID "0000abf2-0000-1000-8000-00805f9b34fb" // "Receive Data"
#define NODE_BLE_SENSOR_UPSTREAM_CHANNEL_DESCRIPTOR_UUID "00002902-0000-1000-8000-00805f9b34fb" // "Write File Descriptor"
#define NODE_BLE_SENSOR_DOWNSTREAM_CHANNEL_UUID "0000abf1-0000-1000-8000-00805f9b34fb" // "Write Data"
#define NODE_CENTRAL_WRITE_DATA_CHANNEL_UUID NODE_BLE_SENSOR_DOWNSTREAM_CHANNEL_UUID
#define NODE_CENTRAL_RECEIVE_DATA_CHANNEL_UUID NODE_BLE_SENSOR_UPSTREAM_CHANNEL_UUID
#define NODE_CENTRAL_RECEIVE_DATA_CHANNEL_FILE_DESCRIPTOR NODE_BLE_SENSOR_UPSTREAM_CHANNEL_DESCRIPTOR_UUID

typedef struct ble_device
{
    int  nDeviceDiscovered = 0;
    char sDBusPath[DBUS_PATH_BUF] = {0};
    char sAddress[BLE_CHAR_BUFF] = {0};
    char sAddressType[BLE_CHAR_BUFF] = {0};
    char sName[BLE_CHAR_BUFF] = {0};
    char sAlias[BLE_CHAR_BUFF] = {0};
    int  nPaired = 0;
    int  nTrusted = 0;
    int  nBlocked = 0;
    int  nLegacyPairing = 0;
    int  nRssi = 0;
    int  nConnected = 0;
    char sAdpater[DBUS_PATH_BUF] = {0};
    int  nServicesResolved = 0;
    char sUUID_ReceiveData[BLE_UUID_BUFF] = {0};
    char sObjectPath_ReceiveData[DBUS_PATH_BUF] = {0};
    char sUUID_ReceiveData_Descriptor[BLE_UUID_BUFF] = {0};
    char sObjectPath_ReceiveData_Descriptor[DBUS_PATH_BUF] = {0};
    char sUUID_WriteData[BLE_UUID_BUFF] = {0};
    char sObjectPath_WriteData[DBUS_PATH_BUF] = {0};
} bluetoothDevice;

void bluez_property_value(const gchar *key, GVariant *value);
int bluez_set_discovery_filter();
static void cleanup_handler(int signo);


void on_interfaces_added (
    OrgFreedesktopDBusObjectManager *object,
    const gchar *arg_object,
    GVariant *arg_interfaces);

void on_interfaces_removed (
    OrgFreedesktopDBusObjectManager *object,
    const gchar *arg_object,
    GVariant *arg_interfaces);

void on_properties_changed( OrgFreedesktopDBusProperties *object,
    const gchar *arg_interface,
    GVariant *arg_changed_properties,
    const gchar *const *arg_invalidated_properties );

void On_discovered_device_properties_change( OrgFreedesktopDBusProperties *object,
    const gchar *arg_interface,
    GVariant *arg_changed_properties,
    const gchar *const *arg_invalidated_properties );

void on_gatt_characteristic_write_to_properties_change( OrgFreedesktopDBusProperties *object,
    const gchar *arg_interface,
    GVariant *arg_changed_properties,
    const gchar *const *arg_invalidated_properties );

void on_gatt_characteristic_read_from_properties_change( OrgFreedesktopDBusProperties *object,
    const gchar *arg_interface,
    GVariant *arg_changed_properties,
    const gchar *const *arg_invalidated_properties );

#endif
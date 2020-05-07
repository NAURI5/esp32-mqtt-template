#define HOSTNAME "esp32test" //WIFI_HOSTNAME,OTA_HOSTNAME, MQTT_ID, mDNS (platformio.ini)
const char *prefixtopic = "D49";
const char *subscribetopic[] = {"alarm", "heartbeat", "settings", "load", "readI", "readH", "cmd", "time", "setDeviceId"};

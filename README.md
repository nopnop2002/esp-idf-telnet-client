# esp-idf-telnet-client
telnet client example for esp-idf.   
You can execute remote command via telnet.   

# Installation for ESP32
```
git clone https://github.com/nopnop2002/esp-idf-telnet-client
cd esp-idf-telnet-client/
idf.py set-target esp32
idf.py menuconfig
idf.py flash monitor
```

# Installation for ESP32-S2
```
git clone https://github.com/nopnop2002/esp-idf-telnet-client
cd esp-idf-telnet-client/
idf.py set-target esp32s2
idf.py menuconfig
idf.py flash monitor
```

# Configure
You have to set this config value with menuconfig.   
- CONFIG_ESP_WIFI_SSID   
SSID of your wifi.   
- CONFIG_ESP_WIFI_PASSWORD   
PASSWORD of your wifi.   
- CONFIG_ESP_MAXIMUM_RETRY   
Maximum number of retries when connecting to wifi.   
- CONFIG_TELNET_SERVER   
Telnet Server host name or ip address.   
- CONFIG_TELNET_PORT   
Telnet Server ports.   
- CONFIG_TELNET_USER   
Telnet Login User name.
- CONFIG_TELNET_PASSWORD   
Telnet Login Password.   

# Command list
```
$ cat data/command.txt
#this is example list
>LANG=C
<2
>ls /tmp
<2
>uname -a
<2
>sleep 20
<25
>hogehoge
<2
#Execute exit without exit
```

Syntax:   
- #hoge   
 Comment line   
- &gthoge   
 Execute command   
- &ltsec   
 Waiting time for the command to finish (seconds)   
 Wait this time and execute the next command   


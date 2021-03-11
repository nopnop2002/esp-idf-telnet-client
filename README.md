# esp-idf-telnet-client
telnet client example for esp-idf.   
You can execute remote command via telnet.   
I referred to [this](https://github.com/hasan-kamal/Telnet-Client).

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

![config-main](https://user-images.githubusercontent.com/6020549/110864019-e03af480-8304-11eb-9fae-bf4da9318b8d.jpg)
![config-app](https://user-images.githubusercontent.com/6020549/110864029-e29d4e80-8304-11eb-9808-acc304675e14.jpg)

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
```
#hoge   
 Comment line   
>hoge   
 Remote command   
<sec   
 Waiting time for the command to finish (seconds)   
 Wait this time and execute the next command   
```

# Screen Shot
![ScreenShot-1](https://user-images.githubusercontent.com/6020549/110864068-f47ef180-8304-11eb-9d37-854980b0ec3a.jpg)
![ScreenShot-2](https://user-images.githubusercontent.com/6020549/110864078-f6e14b80-8304-11eb-87df-89da6935097e.jpg)


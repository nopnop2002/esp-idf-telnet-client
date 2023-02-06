# esp-idf-telnet-client
telnet client example for esp-idf.   
You can execute remote command via telnet.   
I referred to [this](https://github.com/hasan-kamal/Telnet-Client).

# Software requirement
ESP-IDF V4.4/V5.0.   
ESP-IDF V5 is required when using ESP32-C2.   

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-telnet-client
cd esp-idf-telnet-client/
idf.py set-target {esp32/esp32s2/esp32s3/esp32c2/esp32c3}
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

# STDOUT
You can see the standard output using a browser.   
Unfortunately, Japanese characters are garbled.   
![stdout](https://user-images.githubusercontent.com/6020549/111010246-47c27400-83d9-11eb-89e7-2e7b1a3aac51.jpg)

# Screen Shot
![ScreenShot-1](https://user-images.githubusercontent.com/6020549/110864068-f47ef180-8304-11eb-9d37-854980b0ec3a.jpg)
![ScreenShot-2](https://user-images.githubusercontent.com/6020549/111010620-68d79480-83da-11eb-8814-a18316ea2cee.jpg)

# Task diagram
![Telnet-Client](https://user-images.githubusercontent.com/6020549/111012919-ffa74f80-83e0-11eb-99a6-611d66e1dd24.jpg)


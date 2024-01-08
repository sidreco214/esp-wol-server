# ESP WOL Server
WOL HTTPS Server on ESP32

* When Wifi is connected, built-in LED (GIPO_NUM_2) turn on.
* When the server sends magic packet, built-in LED blink three times.
* Multiple registering of user is supported.

## Menuconfig
* Compiler Option -> Optimization Level (Optimize for performance (-O2)) #Release build
* Component Config -> HTTP Server -> Max HTTP Request Header Length 1024
* Component Config -> ESP HTTPS Server -> Enable ESP_HTTPS_SERVER 
* Component Config -> ESP System Setting -> CPU frequency(240 MHz)
* Partition Table -> Partition Table (Custom partition table CSV)

## Build & and upload
### Using idf.py
```console
git clone https://github.com/sidreco214/esp-wol-server
cd esp-wol-server
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p <Port> flash monitor
```
If you want to debug build, enter this command instead of idf.py set-target esp32
```cmake -G Ninja -B build -D IDF_TARGET=esp32 -D CMAKE_BUILD_TYPE=DEBUG```

### Using cmake
```console
git clone https://github.com/sidreco214/esp-wol-server
cd esp-wol-server
cmake -G Ninja -B build-release -D IDF_TARGET=esp32 -D SDKCONFIG=sdkconfig.release -D CMAKE_BUILD_TYPE=Release
cd build-release
ninja menuconfig
ninja all
python -m esptool -p <Port> --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 4MB --flash_freq 40m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/esp-wol-server.bin 0x187000 build/storage.bin
```

## Configuration
### SSL Certification
#### Open SSL
```console
openssl req -newkey rsa:2048 -nodes -keyout certs/prvtkey.pem -x509 -days 3650 -out certs/servercert.pem -subj "/C=KR/CN=ESP32 WOL Server"
```
other subjs configuration
* CN      Common Name
* L       Locality Name
* ST      State or ProvinceName
* O       Organization Name
* OU      Organizational Unit Name
* C       Country Name
* STREET  Street Address
* DC      Domain Component
* UID     Userid

#### Update SSL Certification
```console
python ~/esp/esp-idf/components/spiffs/spiffsgen.py 0x5000 certs certs.bin
python ~/esp/esp-idf/components/partition_table/parttool.py -p /dev/ttyUSB0 write_partition --partition-name=storage --input certs.bin
```
or regenerate openssl and reflash it.

### Wifi and User Data
#### Method 1 - Serial Monitor
Baud Rate is 115200
```console
/nvs_set WIFI_SSID <WIFI SSID>
/nvs_set WIFI_PW <WIFI Password>
/nvs_set BROADCAST_IP xxx.xxx.xxx.xxx
/nvs_register_user <User Name> <Password> <MAC Address>
/reboot
```

Example
```console
/nvs_set WIFI_SSID MY_WIFI
/nvs_set WIFI_PW MY_PASSWORD
/nvs_set BROADCAST_IP 123.456.789.255
/nvs_register_user My_PC 123456abc 72-47-28-d7-a1-e8
/nvs_register_user My_PC 123456abc 72:47:28:d7:a1:e8 #it's also ok
```

#### Method 2 - nvs_partition_generator
```console
touch nvs-partition.csv #linux or git bash
echo . > nvs-partition.csv #window
```

nvs-partition.csv
```console
key,type,encoding,value
Storage,namespace,,
WIFI_SSID,data,string,YOUR_WIFI SSID
WIFI_PW,data,string,YOUR_WIFI_PASSWORD
BROADCAST_IP,data,string,YOUR_BRAOADCAST_IP
USERNAME,namespace,,
pw,data,string,YOUR_PASSWORD
mac,data,i64,YOUR_MAC_DATA
USERNAME2,namespace,,
pw,data,string,YOUR_PASSWORD2
mac,data,i64,YOUR_MAC_DATA2
...
```

```console
python ~/esp/esp-idf/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate nvs-partition.csv nvs-partition.bin 0x6000
python ~/esp/esp-idf/components/partition_table/parttool.py -p /dev/ttyUSB0 write_partition --partition-name=nvs --input nvs-partition.bin
```

##### Note
If your MAC Address: 72-47-28-d7-a1-e8, MAC data is 255781797119858(0xe8a1d7284772).

##### How to convert MAC Address to MAC data
1. Reverse the order of Mac Address. -> e8-a1-d7-28-42-72
2. Delete hypon(-) or colon(:) and add '0x' in front of them. ->   0xe8a1d7284272
3. Convert decimal to binary number. -> 255781797119858
Windows stadard caculator in programmer mode is helpful

## Serial Commands
```console
/help #Show a list of serial command
/reboot #Rebooting device
/nvs_erase #Erase all NVS data
/nvs_set <key> <value> #Set NVS data
/nvs_register_user <UserName> <password> <MAC Address> #Register user data
/nvs_show_user <UserName> #show nvs data
```

## Request Sending Magic Packet
HTTPS POST Form
```console
POST /wol HTTP/1.1
Host: hostname
Content-Type: application/x-www-form-urlencoded
Authorization: Basic Base64_Encoded_Credential

name=USER_NAME
```

Using curl
```console
curl -u "<UserName>:<password>" -d "name=<UserName>" -H "Content-Type: application/x-www-form-urlencoded" -k "https://ESP32_IP_ADDRESS/wol"
```
If you have successful installation, Built-in led blink three times
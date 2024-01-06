# ESP WOL Server
WOL HTTPS Server on ESP32

* binary size is 0x11bdd0 bytes
* When Wifi is connected, Built-in led(GIPO_NUM_2) turn on.
* When Server sends magic packet, Built-in led(GIPO_NUM_2) blink three times.

## Menuconfig
* Compiler Option -> Enable C++ Exception
* Compiler Option -> Optimization Level (Optimize for performance (-O2))
* Component Config -> HTTP Server -> Max HTTP Request Header Length 1024
* Component Config -> ESP HTTPS Server -> Enable ESP_HTTPS_SERVER 
* Component Config -> ESP System Setting -> CPU frequency(240 MHz)
* Partition Table -> Partition Table (Single factory app (Large, no OTA))

## Build & and upload
```console
git clone https://github.com/sidreco214/esp-wol-server
cd esp-wol-server
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p <Port> flash monitor
```
When flash ESP32 push Boot button on ESP32

## Configuration
### SSL Certification
#### Open SSL
```console
openssl req -newkey rsa:2048 -nodes -keyout main/certs/prvtkey.pem -x509 -days 3650 -out main/certs/servercert.pem -subj "/C=KR/CN=ESP32 WOL Server"
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
```

```console
python ~/esp/esp-idf/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate nvs-partition.csv nvs-partition.bin 0x4000
python ~/esp/esp-idf/components/partition_table/parttool.py -p /dev/ttyUSB0 write_partition --partition-name=nvs --input nvs-partition.bin
```

##### Note
If your MAC Address: 72-47-28-d7-a1-e8, MAC data is 255781797119858(0xe8a1d7284772).

##### How to convert MAC Address to MAC data
1. Reverse the order of Mac Address. -> e8-a1-d7-28-42-72
2. Delete hypon(-) or colon(:) and add '0x' in front of them. ->   0xe8a1d7284272
3. Convert decimal to binary number. -> 255781797119858

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
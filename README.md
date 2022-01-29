# Esp32 conectado no Azure IoT Central

Este código deve permitir que um ESP32 genérico com um sensor DHT11 se conecte ao Azure IoT Central via MQTT e envie telemetria de temperatura e de umidade. O Azure IoT Central é usado para enviar alguns comandos que alteram o estado  da LED já embutida no ESP32 e como imprimir alguns comando no monitor serial.
O projeto foi desenvolvido usando o VS Code com a extensão PlatformIO e o arduino framework.
As configurações do WiFi, assim como à string de conexão do dispositivo devem ser adicionadas no arquivo iot_config.h.

A string de conexão do dispositivo pode ser recebida de algumas maneiras, através do sdk do azure, provisionando de maneira automática em um dps ou outras maneira pode ser encontradas no link:
https://github.com/Azure/azure-iot-device-ecosystem/blob/master/setup_iothub.md#create-new-device-in-the-iot-hub-device-identity-registry

# Instruções
1. Conecte o DHT11 com o pino de dados conectados ao GPIO13.
2. Baixe e descompacte o repositório.
3. Abra a pasta descompactada no vs code com a extensão do platformio instalada.
4. Atualize iot_config.h com as credenciais do WiFi e com a cadeia de conexão do dispositivo.
5. Tente compilar, vai ter alguns um erros, mas seguimos assim.
6. Abra o serial monitor, deverá ver mensagem sobre se conectar no Wifi e no iot hub no serial.
7. As mensagens já aparecem no Iot Central

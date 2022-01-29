#include <Arduino.h>

/*
  Este programa permite que um esp32 com o sensor DHT11 de temperatura e umidade,
  se conecte ao Azure IoT Central e envie telemetria e comandos ao dispositivo iot.

  No arquivo iot_configs possui as credencias do wifi e a string de conexão do dps.
  A string de conexão do dispositivo pode ser gerada pela keygen, por uma sdk em c.

  O codigo foi feito no platformio
  
*/

#include <WiFi.h>
#include <Esp32MQTTClient.h>
#include <WiFiClientSecure.h>
#include "Adafruit_Sensor.h"    //sensor DHT
#include "DHT.h"                //sensor DHT
#include "iot_configs.h"      //biblioteca onde tem as credenciais do wifi e a string de conexao do dispositivo

#define INTERVAL 240000        //tempo de mensagem enviadas em milissegundos
#define MESSAGE_MAX_LEN 256   //tam maximo da mensagem enviada

DHT dht(13,DHT11);             //cria uma instancia do sensor dht11 na porta 13

//Pega as credenciais de configs.h
const char* ssid     = IOT_CONFIG_WIFI_SSID;
const char* password = IOT_CONFIG_WIFI_PASSWORD;
static const char* connectionString = DEVICE_CONNECTION_STRING;

const char *messageData = "{\"messageId\":%d, \"temperature\":%f, \"humidity\":%f, \"LED\":%i}";  //Structure of the message sent to Azure IoT Hub
static bool hasIoTHub = false;
static bool hasWifi = false;
int messageCount = 1;
static bool messageSending = true;
static uint64_t send_interval_ms;
boolean LED = false;                //para ligar e desligar o led da placa


//Função que vai confirmar que o Azure IoT recebeu uma msg do dispositivo
static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

//Função que confirma quando o Azure enviou uma mensagem ao dispositivo
static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

//Função que sera excutada quando a atividade do disp gemeo for executada
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';

  Serial.println("Device twin callback active");
  Serial.println(temp);
  free(temp);
}

//Função que é executada quando uma mensagem é recebida no dispositivo no iot hub. Aqui é onde é feito os comandos
static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);                           //methodName é o nome do comando enviado do IoT Central
  const char *responseMessage = "\"Successfully invoke device method\"";    //Uma mensagem json formatada corretamente.
  int result = 200;                                                         //200 é bom, 400 é ruim. https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support

  //Verificando o methodName para saber se tem algum comando conhecido
  
  if (strcmp(methodName, "start") == 0)     //procurando um comando que chega chamado start
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)   //procurando um comando que chega chamado stop
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else if (strcmp(methodName, "echo") == 0)   //procurando um comando que chega chamado echo
  {
    Serial.println("echo command detected");
    Serial.print("Executed direct method payload: ");
    Serial.println((const char *)payload);

  }
  else if (strcmp(methodName, "toggleLED") == 0)   //procurando um comando que chega chamado toggleLED
  {
    Serial.println("toggle the LED");
    LED = !LED;
    digitalWrite(2, LED);
  }
  else
  {
    LogInfo("No method %s found", methodName);    //Se chegar algum comando desconhecido, ele cai nesse else
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage);
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}
float Chum, Ctemp;

void setup(){
  Serial.begin(115200);
  dht.begin();
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");
  Serial.println(" > WiFi");
  Serial.println("Starting connecting WiFi.");

  //inicia a conexao com o wifi, com as credencias dadas no iot_config.h
  delay(10);
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    hasWifi = false;
  }
  hasWifi = true;
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(" > IoT Hub");
  
  //conecta no iot hub usando a string de conexao do dispositivo dado no iot_config.h
  if (!Esp32MQTTClient_Init((const uint8_t*)connectionString, true)){
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
    return;
  }
  hasIoTHub = true;

  //configurando todas as funções de callback, todas tratadas no ESP32MQTTCLIENT
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
  Serial.println("Start sending events.");
  send_interval_ms = millis();              //começa a contar o tempo
  digitalWrite(2, LOW);                     //liga a led do disp como off padrão
  Chum = 0;
  Ctemp = 0;
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
if (hasWifi && hasIoTHub){
    if (Chum != humidity || Ctemp != temperature){
      if (messageSending && (int)(millis() - send_interval_ms) >= INTERVAL){
        char messagePayload[MESSAGE_MAX_LEN];         //cria uma matriz de caracter que vai ter a mensagem que vai ser enviada ao hub iot
        float temperature = dht.readTemperature();    //coleta a temperatura e umidade do sensor dht11
        float humidity = dht.readHumidity();
        snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, messageCount++, temperature, humidity, LED); //cria mensagem a partir dos dados da função medida. (temp, humidity, led)
        Serial.println(messagePayload);                                                                     //escreve msg no serial monitor para depuração
        EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);                  //prepara para enviar a msg para o broker MQTT
        Esp32MQTTClient_SendEventInstance(message);                                                         //Envia Mensagem
        send_interval_ms = millis();                                                                        //atualiza o temporizador
      }
      else
      {
        Esp32MQTTClient_Check();                                                                            //manter conexao com o hub da azure ativa, mesmo que nao estiver enviando mensagens
      }
      Chum = humidity;
      Ctemp = temperature;
    }
  }
  delay(10);
}
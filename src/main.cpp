/*Sistema de monitoramento da Temperatura
  MARCOS MEIRA E MILENA FREITAS 
  17 de fevereiro de 2021
*/
#include <Arduino.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Ticker.h>
#include <DHtesp.h>
#include <ArduinoJson.h>
#include <U8x8lib.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <SPI.h>
#include <WiFiClient.h>

#define EEPROM_SIZE 1024
#define LED_BUILTIN LED_BUILTIN
#define TOKEN "ib+r)WKRvHCGjmjGQ0"
#define ORG "n5hyok"
#define WIFI_SSID "Metropole"
#define WIFI_PASSWORD "908070Radio"
#define BROKER_MQTT "10.71.0.2"
#define DEVICE_TYPE "ESP32"
#define PUBLISH_INTERVAL 1000*60*20 //intervalo de 20 min para publicar temperatura

uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
uint16_t chip = (uint16_t)(chipid >> 32);
char DEVICE_ID[23];
char an = snprintf(DEVICE_ID, 23, "biit%04X%08X", chip, (uint32_t)chipid); // PEGA IP
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);
WiFiUDP udp;
char topic[]= "macaco"; // topico MQTT
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000); //Hr do Br
DHTesp dhtSensor;
DynamicJsonDocument doc (1024); //tamanho do doc do json
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(15, 4, 16);
bool publishNewState = false; 
TaskHandle_t retornoTemp;
bool novaTemp = false;
int tempAtual=0;
int tempAntiga=0;
int movimento=0;
bool mov=false;
bool tasksAtivo = true;
struct tm data; //armazena data 
char data_formatada[64];
char data_visor[64];
bool tensaoPin=false;
unsigned long tempo = 1000*60*5; // 5 min
unsigned long ultimoGatilho = millis()+tempo;
char timeStamp;  
IPAddress ip=WiFi.localIP();  
const int dhtPin1=14;
const int pirPin1=32; 
const int sensorTensao=23;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
///////////////////////////////// 
const char* host = "esp32-Transmissor";
// const int PIR 	= 22;
// const int TEMP	=	21;
// int StatusPIR;
// int StatusTEMP;
/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";
/* Login page */
String loginIndex =
"<form name=loginForm>"
"<h1>ESP32 Login</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='servidor908070')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
/* Server Index Page */
String serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;
void callback(char* topic, byte* payload3, unsigned int length){ 
//retorna infoMQTT
  char msg;
  if (topic == "status") {
    for (int i=0;i<length;i++) {
      Serial.print((char)payload3[i]);
      msg += (char)payload3[i];
    }
  Serial.println("ok");
  publishNewState = true;
  }
}
void conectaMQTT () {
  //Estabelece conexao c MQTT/WIFI
  while (!client.connected()) {
    Serial.println("Conectando MQTT...");
    if (client.connect("ESP32")) {
      Serial.println ("Conectado :)");
      client.publish ("teste", "ola mundo");
      client.subscribe ("macaco");      
      client.subscribe ("status");
    } else { //reconecta ate ter sucesso
      Serial.println("Falha na conexao");
      Serial.print(client.state());
      Serial.print("nova tentativa em 5s...");
      delay (5000);
    }
  }
}
void reconectaMQTT(){
  //reconecta MQTT caso desconecte
  if (!client.connected()) {
    conectaMQTT();
  }
  client.loop();
}
void datahora(){
  time_t tt=time(NULL);
  data = *gmtime(&tt);
  strftime(data_formatada, 64, "%w - %d/%m/%y %H:%M:%S", &data);//hora completa
  strftime(data_visor, 64, "%a - %H:%M", &data); //hora para o visor
  //Armazena na variável hora, o horário atual.
  int anoEsp = data.tm_year;
  if (anoEsp < 2020) {
    ntp.forceUpdate();
  }	
}
void sensorTemp(void *pvParameters){
  Serial.println ("sensorTemp inicio do LOOP");
  while (1) {//busca temp enquanto estiver ligado
    novaTemp=true;
    tempAtual = dhtSensor.getTemperature();
    tasksAtivo=false;
    vTaskSuspend (NULL);
  }
}
void IRAM_ATTR mudaStatusPir(){
  mov=true;
 }
void pegaTemp () {
      if (retornoTemp != NULL) {
    xTaskResumeFromISR (retornoTemp);
  }
}
void publish (){
  if (tempAntiga != tempAtual){
    // nova temperatura
    tempAntiga==tempAtual;
    novaTemp = 1;
  } else {
    // temperatura igual
    novaTemp==0;
  }
  
}
void Tensao(){
  tensaoPin=true;
}
void PinConfig () {
  // config das portas i/o
  pinMode(dhtPin1, INPUT);
	pinMode(pirPin1, INPUT_PULLUP);
  pinMode(sensorTensao, INPUT_PULLUP);
}
void U8x8visor() { //publica info no visor painel
  u8x8.setFont(u8x8_font_8x13B_1x2_f);
  u8x8.setCursor(2,0);
  u8x8.print(String(data_visor)+"h");
  u8x8.setFont(u8x8_font_inb21_2x4_f);
  u8x8.setCursor(2,3);
	u8x8.print(String(tempAtual)+"\xb0""C");
  u8x8.setFont(u8x8_font_8x13B_1x2_f);
  u8x8.setCursor(13,6);
  u8x8.print("M-");
  u8x8.setFont(u8x8_font_8x13B_1x2_f);
  u8x8.setCursor(15,6);
  u8x8.print(digitalRead(pirPin1));
}
void UpdateRemoto() {
  if (!MDNS.begin(host)) {
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}
	server.on("/", HTTP_GET, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/html", loginIndex);
	});
	server.on("/serverIndex", HTTP_GET, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/html", serverIndex);
	});
	server.on("/update", HTTP_POST, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		ESP.restart();
	}, []() {
		HTTPUpload& upload = server.upload();
		if (upload.status == UPLOAD_FILE_START) {
			Serial.printf("Update: %s\n", upload.filename.c_str());
			if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {//start with max available size
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_END) {
			if (Update.end(true)) {
				Serial.printf("Update Success: %u\nRebooting++...\n", upload.totalSize);
			} else {
				Update.printError(Serial);
			}
		}
	});
}
void payloadMQTT (){
  datahora();
  u8x8.clear();
  Serial.println("Tempo: " +String(ultimoGatilho));
  int tensao=digitalRead(sensorTensao);
  movimento = digitalRead(pirPin1);
  time_t tt=time(NULL);
  String payload = "{\"local\":";
  payload += "\"SalaTransmisssor\"";
  payload += ",";
  payload += "\"hora\":";
  payload += tt;
  payload += ",";
  payload += "\"temp\":";
  payload += tempAtual;
  payload += ",";
  payload += "\"movimento\":";
  payload += movimento;
  payload += ",";
  payload += "\"tensao\":";
  payload += tensao;
  payload += ",";
  payload += "\"ip\":";
  payload +="\"";
  payload += ip.toString();
  payload +="\"";
  payload += ",";
  payload += "\"mac\":";
  payload +="\"";
  payload += DEVICE_ID;
  payload +="\"";
  payload += "}";
  client.publish (topic, (char*) payload.c_str());
  novaTemp=false;
  tensaoPin=false; 
  movimento=false;
}
Ticker tickerpin(publish, PUBLISH_INTERVAL);
Ticker tempTicker(pegaTemp, 10000);
void setup () {
  Serial.begin (115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  while (WiFi.status()!= WL_CONNECTED){//incia wifi ate q funcione
    Serial.print (".");
    delay(1000);
    }
  ntp.begin ();
  ntp.forceUpdate();
  if (!ntp.forceUpdate ()){
      while (1) {
        Serial.println ("Erro ao carregar a hora");
        delay (1000);
      }
  } else {
    timeval tv; //define a estrutura da hora 
    tv.tv_sec = ntp.getEpochTime(); //segundos
    settimeofday (&tv, NULL);
  }
  ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(DEVICE_ID);
  client.setServer (BROKER_MQTT, 1883);//define mqtt
  client.setCallback(callback); 
  server.begin();
  PinConfig();
  dhtSensor.setup(dhtPin1, DHTesp::DHT11);
  xTaskCreatePinnedToCore (sensorTemp, "sensorTemp", 4000, NULL, 5, &retornoTemp, 0);
  attachInterrupt (digitalPinToInterrupt(32), mudaStatusPir, RISING);
  attachInterrupt (digitalPinToInterrupt(sensorTensao), Tensao, CHANGE);
  tickerpin.start();
  tempTicker.start();
  u8x8.begin();
  u8x8.clear();
  UpdateRemoto();
  movimento = digitalRead(pirPin1);
}
void loop(){
  U8x8visor();
  server.handleClient();
  if(!client.connected()){
    reconectaMQTT();
  }
  client.loop();
  if(tempAtual>50){
    novaTemp=false;
  }
  if((ultimoGatilho < millis()) && movimento==1){ //publica no MQTT  
    payloadMQTT();
    ultimoGatilho = millis()+tempo;
  }else if (tensaoPin||novaTemp==1){
    payloadMQTT();
  }
tempTicker.update();
tickerpin.update();
}


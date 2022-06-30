//esp32-光线传感器(面向光敏电阻且接口在下方时，接线柱从左到右依次命名为1，2，3)
//gnd-3
//P35-1
//3.3V-2

//esp32-led灯(灯管内部薄片较粗的为1，较薄的为2)
//gnd-1
//P18-2
//其实，led灯接线柱的选择可以试出来，且最多试一次。

//oled
//gnd-gnd
//3.3V-3.3V
//P21-SDA
//P22-SCL

//only for ESP32-Lora
#define SCK 14
#define MISO 12
#define MOSI 13
#define SS 21
#define DIO0 23


#include <Wire.h>
#include <LoRaNow.h>
#include <WiFi.h>
#include <WebServer.h>
#include <StreamString.h>
#include <PubSubClient.h>
#include "SSD1306.h"


const char *ssid =  "test";     // Wi-Fi Name
const char *password =  "12345678"; // Wi-Fi Password
const char *mqtt_server = "broker.hivemq.com";

WebServer server(80);
WiFiClient espClient;
PubSubClient mqttclient(espClient);

const char *script = "<script>function loop() {var resp = GET_NOW('loranow'); var area = document.getElementById('area').value; document.getElementById('area').value = area + resp; setTimeout('loop()', 1000);} function GET_NOW(get) { var xmlhttp; if (window.XMLHttpRequest) xmlhttp = new XMLHttpRequest(); else xmlhttp = new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.open('GET', get, false); xmlhttp.send(); return xmlhttp.responseText; }</script>";


unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (60)
char msg[MSG_BUFFER_SIZE];

int value = 0;

void handleRoot()
{
  String str = "";
  str += "<html>";
  str += "<head>";
  str += "<title>monitor</title>";
  str += "<style type=\"text/css\">body{background: url(https://img.zcool.cn/community/015ea357afda5f0000018c1b2f966d.jpg@1280w_1l_2o_100sh.jpg) repeat-x center top fixed}</style>";
  str += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  str += script;
  str += "</head>";
  str += "<body onload='loop()'>";
  str += "<center>";
  str += "<textarea id='area' style='width:800px; height:400px;'></textarea>";
  str += "</center>";
  str += "</body>";
  str += "</html>";
  server.send(200, "text/html", str);
}

static StreamString string;

void handleLoRaNow()
{
  server.send(200, "text/plain", string);
  while (string.available()) // clear
  {
    string.read();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("有新消息[");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}




SSD1306 display(0x3c, 21, 22);

int freq = 9000;    // 频率
int channel = 0;    // 通道
int resolution = 10;   // 分辨率
const int led = 18;
const int gm = 35; //光敏电阻引脚
int Filter_Value;

void onMessage(uint8_t *buffer, size_t size)
{
  int i=35,j=0,k=0;
  char tmp[6];
  unsigned long id = LoRaNow.id();
  byte count = LoRaNow.count();
  //display.clear();
  Serial.print("节点ID: ");
  Serial.println(id, HEX);
  Serial.print("接收次数: ");
  Serial.println(count);
  Serial.print("信息内容: ");
  Serial.write(buffer, size);
  while ('0'>=buffer[i]||'9'<=buffer[i])i++;
  j=i+5;
  for(i;i<=j;i++,k++) tmp[k]=buffer[i];
  tmp[k-1]='\0';
  Serial.println();
  Serial.println();
//  display.setTextAlignment(TEXT_ALIGN_LEFT);
  //display.setFont(ArialMT_Plain_24);
  //display.drawString(0, 0, "Temperature:");
 // display.setFont(ArialMT_Plain_24);
 // display.drawString(0, 32, tmp);
  //display.display();
   //此处通过mqtt发送信息。
   snprintf (msg, MSG_BUFFER_SIZE, "#%d#%s", count,buffer);
   Serial.print("发布信息: ");
   Serial.println(msg);
   mqttclient.publish("C2TLOutTopic", msg);

  if (string.available() > 512)
  {
    while (string.available())
    {
      string.read();
    }
  }

  string.print("节点ID: ");
  string.println(id, HEX);
  string.print("接收次数: ");
  string.println(count);
  string.print("信息内容: ");
  string.write(buffer, size);
  string.println();
  string.println();

  // Send data to the node
  LoRaNow.clear();
  LoRaNow.print("LoRaNow 网关信息");
  LoRaNow.print(millis());
  LoRaNow.send();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.println("尝试与MQTT服务器进行连接");
    // Create a random mqttclient ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttclient.connect(clientId.c_str())) {
      Serial.println("连接成功");
      // Once connected, publish an announcement...
      mqttclient.publish("C2TLOutTopic", "hello world");
      // ... and resubscribe
      mqttclient.subscribe("C2TLInTopic");
    } else {
      Serial.print("失败, 错误代码：");
      Serial.print(mqttclient.state());
      Serial.println("。5秒后再次尝试。");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup(void)
{
  Serial.begin(115200); 


 // pinMode(A0, INPUT);



 WiFi.mode(WIFI_STA);
  //if (ssid != "")
    WiFi.begin(ssid, password);
  WiFi.begin();
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("已经成功连接到：");
  Serial.println(ssid);
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/loranow", handleLoRaNow);
  server.begin();
  Serial.println("已启用HTTP服务器");

  LoRaNow.setFrequencyCN(); // Select the frequency 486.5 MHz - Used in China
  // LoRaNow.setFrequencyEU(); // Select the frequency 868.3 MHz - Used in Europe
  // LoRaNow.setFrequencyUS(); // Select the frequency 904.1 MHz - Used in USA, Canada and South America
  // LoRaNow.setFrequencyAU(); // Select the frequency 917.0 MHz - Used in Australia, Brazil and Chile

  LoRaNow.setFrequency(433E6);
   LoRaNow.setSpreadingFactor(7);
  // LoRaNow.setPins(ss, dio0);

  LoRaNow.setPinsSPI(SCK, MISO, MOSI, SS, DIO0); // Only works with ESP32

  if (!LoRaNow.begin())
  {
    Serial.println("LoRa加载失败。检查你的连接。");
    while (true)
      ;
  }

  LoRaNow.onMessage(onMessage);
  LoRaNow.gateway();
  //mqtt  
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);

  
    
  display.init();
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 0,"Welcome!" );
  display.display();
  delay(5000);
  display.clear();
  //设置光敏电阻输入
  pinMode(gm,INPUT); //光敏电阻
  //randomSeed(analogRead(gm));
  
  ledcSetup(channel, freq, resolution); // 设置通道
  ledcAttachPin(led, channel);  // 将通道与对应的引脚连接

}



#define FILTER_N 20
int Filter() {
  int i;
  int filter_sum = 0;
  int filter_max, filter_min;
  int filter_buf[FILTER_N];
  for(i = 0; i < FILTER_N; i++) {
    filter_buf[i] = analogRead(gm);
    //Serial.println(analogRead(gm));
    delay(1);
  }
  filter_max = filter_buf[0];
  filter_min = filter_buf[0];
  filter_sum = filter_buf[0];
  for(i = FILTER_N - 1; i > 0; i--) {
    if(filter_buf[i] > filter_max)
      filter_max=filter_buf[i];
    else if(filter_buf[i] < filter_min)
      filter_min=filter_buf[i];
    filter_sum = filter_sum + filter_buf[i];
    filter_buf[i] = filter_buf[i - 1];
  }
  i = FILTER_N - 2;
  filter_sum = filter_sum - filter_max - filter_min + i / 2; // +i/2 的目的是为了四舍五入
  filter_sum = filter_sum / i;
  return filter_sum;
}


void mqttloop() {

  if (!mqttclient.connected()) {
    reconnect();
  }
  mqttclient.loop();

//  unsigned long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    mqttclient.publish("C2TLOutTopic", msg);
//  }
}


void loop()
{ 

  // Serial.println(analogRead(A0));
    Filter_Value = Filter();
  Filter_Value=(Filter_Value*3.3)/4.5;
  //Serial.println(Filter_Value);
  
  int m=map(Filter_Value,0,2650,255,0);
  
  if(m<2)
  {
    ledcWrite(0,0);
  }
  else
  {
    ledcWrite(0,m);
  }
  String result = String(Filter_Value);
  display.init();
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0,"Light-lux:\n"+result );
  display.display();+
  delay(500);
  display.clear();
  LoRaNow.loop();
  server.handleClient();
  mqttloop();
  //Serial.println(map(Filter_Value,0,2650,255,0));
}

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#include <ESP8266httpUpdate.h>

#include <PubSubClient.h>

//DHT部分
#include <DHT.h>
/**
#define DHTPIN  4        // 设置DHT11连接的引脚
#define DHTTYPE DHT11    // 设置使用的传感器类型

DHT dht(DHTPIN, DHTTYPE);  // 创建DHT对象
*/

String dhtInfo = "";
int    dhtRun  = 0;

//OLED
#include <U8g2lib.h>
int    oledRun = 0;

//红外遥控
#include <IRremoteESP8266.h> // 引入 IRremoteESP8266 库
#include <IRrecv.h>  // 引入 IRrecv 类
#include <IRutils.h> // 引入 IRutils 工具类

// 定义红外接收模块连接的引脚
const int ir_recv_pin = D2;
IRrecv irrecv(ir_recv_pin); // 创建 IRrecv 对象
decode_results results;     // 创建 decode_results 对象

/*测试部分*/
/*测试部分-结束*/

//自定义lib-----------------------------------
#include "lib/ws2812.h"
#include "lib/sr04.h"
#include "lib/switch.h"
#include "lib/oled.h"

//变量
#include "common.h"

//函数
#include "func.h"
//-------------------------------------------


//////////////////////////////

void getDHT(){
  DHT dht(4, DHT11);
  dht.begin();

  float humidity    = dht.readHumidity();
  float temperature = dht.readTemperature();

  // 检查读取是否成功
  if (isnan(humidity) || isnan(temperature)) {
    dhtInfo= "无法从DHT传感器读取数据！";
  }else{
    dhtInfo = "温度: " + String(temperature) + " °C, 湿度: " + String(humidity) + " %";
  }
  Serial.println(dhtInfo);
}

void getOLED(){
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 14, /* data=*/ 12);

  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.clearBuffer();
  u8g2.setFontPosTop();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
  
  u8g2.drawUTF8(0, 1,  "春种一粒粟,秋收万颗子");
  u8g2.drawUTF8(0, 13, "寒来暑往,不负韶华。");
  u8g2.drawUTF8(0, 25, "");
  u8g2.drawUTF8(0, 49, "    中技云第十波    ");

  u8g2.sendBuffer();  // 将缓冲区内容发送到显示屏
}

////////////////////////////////////////////////////////
void setup() {
  Serial.begin(intSerial);  

  /*测试部分*/
  /*测试部分-结束*/

  //初始化littleFS
  initFS();

  //---------------------------
  //从LittleFS读取内容 系统配置
  strTmp  = readFile(LittleFS, sysPath);
  
  int count = 1;
  int i = 0;
  for (i = 0; i < strTmp.length(); i++) {
      if (strTmp.charAt(i) == '|') {
          count++; // 遇到分隔符，计数器递增
      }
  }  
  
  char charArray[strTmp.length() + 1];
  strTmp.toCharArray(charArray, sizeof(charArray));

  // 使用strtok函数将字符串按照 | 分割，并存储到变量中
  char* token = strtok(charArray, "|");
  char* tokens[count];
  
  i = 0;
  while (token != NULL && i < count) {
      tokens[i++] = token;
      token = strtok(NULL, "|");
  }

  ssid    = tokens[0];
  pass    = tokens[1];
  ip      = tokens[2];
  gateway = tokens[3];
  ota     = tokens[4];
  otahost = tokens[5];
  checkid = tokens[6];

  sinfo("ssid=",    ssid);
  sinfo("pass=",    pass);
  sinfo("ip=",      ip);
  sinfo("gateway=", gateway);
  sinfo("ota=",     ota);
  sinfo("otahost=", otahost);
  sinfo("checkid=", checkid);
  //
  server.serveStatic("/", LittleFS, "/");

  //搜索网络
  Serial.println("Search Wifi");
  ///////////////
  if(initWiFi()) {

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });    

    server.on("/autofs", HTTP_GET, [](AsyncWebServerRequest *request) {
      sinfo("begin ota fs", OTAFSURL);

      //到时候改成websocket方式
      ws.textAll("开始 OTA FS");
      
      ota = "2";
      writeFile(LittleFS, "sys");

      ESP.restart();
    });
    
    // 重启
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/html", "<script>alert('重启中，请5秒后刷新当前页面。');</script>");
      
      ESP.restart();
    });
    
    // 重置
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
      ssid = "";
      writeFile(LittleFS, "sys");
      ESP.restart();      
      
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    server.on("/otahost", HTTP_POST, [](AsyncWebServerRequest *request){
      // 获取提交表单的内容
      if (request->hasArg("otahost")) {
        otahost = request->arg("otahost");
        writeFile(LittleFS, "sys");

        sinfo("otahost: ", otahost);
      }
  
      request->send(200, "text/html",  "<script>alert('修改OTA地址完毕，可以点击重启之后，看看是否生效。')</script>");
    });

    //DHT11通过http请求
    server.on("/dht", HTTP_GET, [](AsyncWebServerRequest *request){
      dhtRun = 1;

      sinfo(dhtInfo);

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", dhtInfo);
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
      request->send(response);

      //request->send(200, "text/plain", dhtInfo);
    });

    //DHT11通过http请求
    server.on("/oled", HTTP_GET, [](AsyncWebServerRequest *request){
      oledRun = 1;

      //跨域
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "看看是否显示");
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
      request->send(response);      
    });


    
    server.begin();
    canTest = 1;

    ///////////
    // 初始化OTA
    
    if(ota == "1") updateOTA();
    if(ota == "2") updateOTAFS();

    if(ota == "1" || ota == "2"){
      ota = 0;
      writeFile(LittleFS, "sys");
      ESP.restart();
    }
    
    sinfo(BOARD, VERSION);

    /////////////
    //搜索到
    Serial.println("Search Wifi Succ");
  } else {
    sinfo("Setting AP (Access Point)");
    
    //ap名
    strTmp = "ZJY-" + String(ESP.getChipId());
    WiFi.softAP(strTmp, "123456");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });

    //直连
    server.on("/c", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });    
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasArg("ssid"))    ssid    = request->arg("ssid");
      if (request->hasArg("pass"))    pass    = request->arg("pass");
      if (request->hasArg("ip"))      ip      = request->arg("ip");
      if (request->hasArg("gateway")) gateway = request->arg("gateway");
      if (request->hasArg("checkid")) checkid = request->arg("checkid");
      writeFile(LittleFS, "sys");

      request->send(200, "text/html",  "<html><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'/><body><h1>写入完毕，重启中，修改后的IP为：<a href='http://"+ip+"'>"+ip+" 点击直达</a></h1></body></html>");

      restart = true;
    });
    server.begin();
  }

  ws.onEvent(onEventHandle); // 绑定回调函数
  server.addHandler(&ws);    // 将WebSocket添加到服务器中
  server.on("/ws", HTTP_GET, handleRoot); //注册链接"/"与对应回调函数
  server.begin(); //启动服务器
  delay(1000);
  ws.textAll("websocket server On"); // 向所有建立连接的客户端发送数据

  //
  irrecv.enableIRIn(); // 启动红外接收器


}

void loop() {
  ws.cleanupClients();     // 关闭过多的WebSocket连接以节省资源

  if (restart){
    delay(5000);
    ESP.restart();
  }
  
  //正式运行这里会执行的
  if(canTest==1){
    //Serial.println("runing version 1.0.0");
    delay(1000);
  }

  if(dhtRun == 1){
    getDHT();
    dhtRun = 0;
  }

  if(oledRun == 1){
    getOLED();
    oledRun = 0;
  }

  /*测试开始*/
  if (irrecv.decode(&results)) { // 如果接收到了红外信号
    Serial.println(results.value, HEX); // 输出红外信号的值（16 进制格式）
    irrecv.resume(); // 继续接收下一个红外信号

    ws.textAll("按键编码:"+String(results.value));
  }
  /*测试结束*/
}
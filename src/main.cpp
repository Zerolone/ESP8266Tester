const char* VERSION = "1.0.1 LOCAL";

////////////////////////////////////////////
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

//
#include <U8g2lib.h>
int    oledRun = 0;

/*测试部分*/

//自定义lib-----------------------------------
#include "lib/myFunc.h"
//-------------------------------------------
//GPIO口
const int ledPin   = 2;
const int lightPIN = D2;

//LED状态
String ledState;

boolean restart = false;

int intReset = 0;
int intReal  = 0;

//是否休眠
int canTest = 0;

//
#ifdef BOARD_TYPE
    #if BOARD_TYPE == ESP01S
        const char* BOARD   = "ESP01S";
    #elif BOARD_TYPE == ESP8266
        const char* BOARD   = "ESP8266";
    #else
        const char* BOARD "未知";
    #endif
#else
    const char* BOARD   = "未定义";
#endif

#define intSerial 115200\

WiFiClient espClient;
PubSubClient client(espClient);

//启用webserver
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");    // WebSocket对象，url为/

//OTA 在线更新
const char* OTAURL   = "/esp8266.bin";
const char* OTAFSURL = "/esp8266fs.bin";

//常用的输入框
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
const char* PARAM_INPUT_5 = "UserKey";
const char* PARAM_INPUT_6 = "otahost";

const char* PARAM_INPUT_7 = "Location";
const char* PARAM_INPUT_8 = "BILIBILI_UID";
const char* PARAM_INPUT_9 = "intBright";

//可以通过webserver修改的内容
String ssid;
String pass;
String ip;
String gateway;
String ota;
String otahost;
String checkid;
String UserKey;       // 和风天气API私钥 https://dev.heweather.com/docs/start/ 需要自行申请
String Location;      // 和风天气城市代码 https://github.com/qwd/LocationList/blob/master/China-City-List-latest.csv Location_ID 需要自行查询
String BILIBILI_UID;  // B站用户ID
String intBright;     // 亮度
String MType;         // 矩阵类型

//临时变量
char buffer[20];

//文件保存路径
const char* ssidPath    = "/ssid.txt"; 
const char* passPath    = "/pass.txt";
const char* ipPath      = "/ip.txt";
const char* gatewayPath = "/gateway.txt";
const char* otaPath     = "/ota.txt";
const char* otahostPath = "/otahost.txt";

const char* UserKeyPath      = "/UserKey.txt";
const char* LocationPath     = "/Location.txt";
const char* BILIBILI_UIDPath = "/BILIBILI_UID.txt";
const char* intBrightPath    = "/intBright.txt";

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 255, 0);



//////////////////////////////
//初始化littlefs
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Err");
  }
  else{
    Serial.println("LittleFS mounted Succ");
  }
}

//写文件内容
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
  
  file.close();
}

//读取文件内容
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}

//////////////////////////////
//OTA 在线更新部分
void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void updateOTA() {
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);
    ESPhttpUpdate.rebootOnUpdate(false); // remove automatic update
      
    t_httpUpdate_return ret = ESPhttpUpdate.update(espClient, otahost + OTAURL);

    writeFile(LittleFS, otaPath, "0");

    switch (ret) {
      case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;
      case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;
      case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
    }  
}

void updateOTAFS() {
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

  // Add optional callback notifiers
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  ESPhttpUpdate.rebootOnUpdate(false); // remove automatic update
  
  t_httpUpdate_return ret = ESPhttpUpdate.updateFS(espClient, otahost + OTAFSURL);
  writeFile(LittleFS, ssidPath,         ssid.c_str());
  writeFile(LittleFS, passPath,         pass.c_str());
  writeFile(LittleFS, ipPath,           ip.c_str());
  writeFile(LittleFS, gatewayPath,      gateway.c_str());
  writeFile(LittleFS, otaPath,          "0");
  writeFile(LittleFS, otahostPath,      otahost.c_str());
  writeFile(LittleFS, UserKeyPath,      UserKey.c_str());
  writeFile(LittleFS, LocationPath,     Location.c_str());
  writeFile(LittleFS, BILIBILI_UIDPath, BILIBILI_UID.c_str());

  switch (ret) {
    case HTTP_UPDATE_FAILED: Serial.printf("FS HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;
    case HTTP_UPDATE_NO_UPDATES: Serial.println("FS HTTP_UPDATE_NO_UPDATES"); break;
    case HTTP_UPDATE_OK: Serial.println("FS HTTP_UPDATE_OK"); break;
  }  
}

//////////////////////////////
// 初始化WIFI
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.println("Connecting to WiFi...");

  int intLoop = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");

    intLoop+=1;
    if(intLoop > 40){
      Serial.println("Failed to connect.");
      return false;
    }
  }
  
  /**
  delay(20000);
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect.");
    
    return false;
  }
  /**/

  Serial.println(WiFi.localIP());
  return true;
}

// 显示到网页各种效果
String processor(const String& var) {
  //Serial.println(var);

  if(var == "STATE") {
    if(!digitalRead(ledPin)) {
      ledState = "开";
    }
    else {
      ledState = "关";
    }
    return ledState;
  }

  if(var == "UserKey")      return UserKey;
  if(var == "Location")     return Location;
  if(var == "BILIBILI_UID") return BILIBILI_UID;
  if(var == "intBright")    return intBright;
  if(var == "VERSION")      return VERSION;
  if(var == "BOARD")        return BOARD;
  if(var == "OTAHOST")      return otahost;
  if(var == "IP")           return ip.c_str();
  
  return String();
}

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

  /*测试*
  dht.begin();           // 初始化DHT传感器
  /*测试结束*/

  //初始化littleFS
  initFS();

  //主板灯
  pinMode(ledPin, OUTPUT);
  
  //从LittleFS读取内容
  ssid    = readFile(LittleFS, ssidPath);
  pass    = readFile(LittleFS, passPath);
  ip      = readFile(LittleFS, ipPath);
  gateway = readFile(LittleFS, gatewayPath);
  ota     = readFile(LittleFS, otaPath);
  otahost = readFile(LittleFS, otahostPath);

  UserKey      = readFile(LittleFS, UserKeyPath);
  Location     = readFile(LittleFS, LocationPath);
  BILIBILI_UID = readFile(LittleFS, BILIBILI_UIDPath);
  intBright    = readFile(LittleFS, intBrightPath);

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);
  Serial.println(ota);
  Serial.println(otahost);
  Serial.println(UserKey);
  Serial.println(Location);


  Serial.print("BILIBILI_UID=");
  Serial.println(BILIBILI_UID);

  Serial.print("intBright=");
  Serial.println(intBright);

  //
  server.serveStatic("/", LittleFS, "/");

  // 拉低
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(ledPin, LOW);
    request->send(LittleFS, "/index.html", "text/html", false, processor);
  });

  // 拉高
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(ledPin, HIGH);
    request->send(LittleFS, "/index.html", "text/html", false, processor);
  });

  //搜索网络
  Serial.println("Search Wifi");
  ///////////////
  if(initWiFi()) {

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });    

    // Route to set GPIO state to LOW
    server.on("/autofs", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, HIGH);

      Serial.println("begin ota fs");
      Serial.println(OTAFSURL);

      request->send(200, "text/html", "begin ota fs");
      
      writeFile(LittleFS, otaPath, "2");
      
      //zpost("OTAFS 升级");
      //updateOTAFS();
      ESP.restart();
    });
    
    // 重启
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/html", "<script>alert('重启中，请5秒后刷新当前页面。');</script>");
      
      //zpost("OTA 升级");
      ESP.restart();
    });
    
    // 重置
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
      ssid = "";
      writeFile(LittleFS, ssidPath, ssid.c_str());
      ESP.restart();      
      
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    /**/
    server.on("/checkid", HTTP_POST, [](AsyncWebServerRequest *request){
      // 获取提交表单的内容
      if (request->hasArg("UserKey")) {
        UserKey = request->arg("UserKey");

        writeFile(LittleFS, UserKeyPath, UserKey.c_str());

        Serial.print("UserKey: ");
        Serial.println(UserKey);
      }
      
      if (request->hasArg("Location")) {
        Location = request->arg("Location");

        writeFile(LittleFS, LocationPath, Location.c_str());

        Serial.print("Location: ");
        Serial.println(Location);
      }

      if (request->hasArg("BILIBILI_UID")) {
        BILIBILI_UID = request->arg("BILIBILI_UID");

        writeFile(LittleFS, BILIBILI_UIDPath, BILIBILI_UID.c_str());

        Serial.print("BILIBILI_UID: ");
        Serial.println(BILIBILI_UID);
      }

      //intBright 亮度
      if (request->hasArg("intBright")) {
        intBright = request->arg("intBright");
        writeFile(LittleFS, intBrightPath, intBright.c_str());

        Serial.print("intBright: ");
        Serial.println(intBright);
      }

      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    server.on("/otahost", HTTP_POST, [](AsyncWebServerRequest *request){
      // 获取提交表单的内容
      if (request->hasArg("otahost")) {
        otahost = request->arg("otahost");
      
        writeFile(LittleFS, otahostPath, otahost.c_str());

        Serial.print("otahost: ");
        Serial.println(otahost);
      }
  
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    //DHT11通过http请求
    server.on("/dht", HTTP_GET, [](AsyncWebServerRequest *request){
      //delay(1000);
      //getDHT();

      dhtRun = 1;

      Serial.println(dhtInfo);

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", dhtInfo);
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
      request->send(response);

      //request->send(200, "text/plain", dhtInfo);
    });


    //DHT11通过http请求
    server.on("/oled", HTTP_GET, [](AsyncWebServerRequest *request){
      //delay(1000);
      //getOLED();
      oledRun = 1;

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
    
    if(ota == "1"){
      updateOTA();
      writeFile(LittleFS, otaPath, "0");
      ESP.restart();
    }
    
    if(ota == "2"){
      updateOTAFS();
      writeFile(LittleFS, otaPath, "0");
      ESP.restart();
    }

    Serial.println(VERSION);
    Serial.println(BOARD);

    /////////////
    //搜索到
    Serial.println("Search Wifi Succ");

    //解析时间
    //solveTime();
    //updateAllData();
    //updateTimeTask.attach(updatePeriod * 60, updateTime); //更新时间
    //scheTask.attach(1, scheTime);

    /////////////
  } else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    
    // 获取当前时间戳
    unsigned long currentTimestamp = millis(); // 获取当前时间戳的毫秒数部分
  
    // 提取时间戳的后3位数字
    unsigned int lastThreeDigits = currentTimestamp % 1000;
  
    // 生成随机数
    int randomNum = random(1000);
  
    // 将时间戳的后3位和随机数拼接成一个字符串
    char result[15];
    sprintf(result, "ZJY-%d-%03u", randomNum, lastThreeDigits);

    //WiFi.softAP("ZJY-NINTH", NULL);
    WiFi.softAP(result, NULL);

    IPAddress IP = WiFi.softAPIP();
    char* ipStr = new char[16];
    sprintf(ipStr, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
    
    delay(2000);
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });

    server.on("/c", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });    
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }

          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            writeFile(LittleFS, passPath, pass.c_str());
          }

          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            writeFile(LittleFS, ipPath, ip.c_str());
          }

          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }

          //
          writeFile(LittleFS, otaPath, "0");

          if (p->name() == PARAM_INPUT_6) {
            otahost = p->value().c_str();
            Serial.print("otahost set to: "); 
            Serial.println(otahost);
            writeFile(LittleFS, otahostPath, otahost.c_str());
          }          

          if (p->name() == PARAM_INPUT_5) {
            UserKey = p->value().c_str();
            Serial.print("UserKey set to: "); 
            Serial.println(UserKey);
            writeFile(LittleFS, UserKeyPath, UserKey.c_str());
          }

          if (p->name() == PARAM_INPUT_7) {
            Location = p->value().c_str();
            Serial.print("Location set to: "); 
            Serial.println(Location);
            writeFile(LittleFS, LocationPath, Location.c_str());
          }

          if (p->name() == PARAM_INPUT_8) {
            BILIBILI_UID = p->value().c_str();
            Serial.print("BILIBILI_UID set to: "); 
            Serial.println(BILIBILI_UID);
            writeFile(LittleFS, BILIBILI_UIDPath, BILIBILI_UID.c_str());
          }                    

          if (p->name() == PARAM_INPUT_9) {
            BILIBILI_UID = p->value().c_str();
            Serial.print("intBright set to: "); 
            Serial.println(intBright);
            writeFile(LittleFS, intBrightPath, intBright.c_str());
          }
          
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      restart = true;
      
      //request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      request->send(200, "text/html",  "<html><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'/><body>写入完毕，重启中，修改后的IP为：<a href='http://"+ip+"'>"+ip+"</a>点击直达</body></html>");
    });
    server.begin();
  }


  ws.onEvent(onEventHandle); // 绑定回调函数
  server.addHandler(&ws);    // 将WebSocket添加到服务器中
  server.on("/ws", HTTP_GET, handleRoot); //注册链接"/"与对应回调函数
  server.begin(); //启动服务器
  delay(1000);
  ws.textAll("websocket"); // 向所有建立连接的客户端发送数据
}

void loop() {
  ws.cleanupClients();     // 关闭过多的WebSocket连接以节省资源

  if (restart){
    delay(5000);
    ESP.restart();
  }
  
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


  /*测试开始

  /*测试结束*/
  /**/
}

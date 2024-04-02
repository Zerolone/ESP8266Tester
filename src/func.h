//调试显示
void sinfo(String str, String strInfo=""){
  if (SHOWINFO == 1) {
    if (strInfo == ""){
      Serial.println(str);
    }else{
      Serial.println(str + "" + strInfo);
    }
  }

  //  sprintf(output, "Sensor value: %d, Temperature: %.2f", sensorValue, temperature);
}

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
void writeFile(fs::FS &fs, String type){
  String path = sysPath;
  String m    = ssid + "|" + pass + "|" + ip + "|" + gateway + "|" + ota + "|" + otahost + "|" + checkid;

  /*
  if(type=="soft") {
    path   = softPath;
    m =  String(intBright) + "|" +  String(mType) + "|" + biliID + "|" + wUserKey + "|" + wLocation;
  } 
  */

  sinfo("Writing:", m);

  File file = fs.open(path, "w");
  if(!file){
    sinfo("- failed to open file for writing");
    return;
  }

  if(file.print(m)){
    sinfo("- file written");
  } else {
    sinfo("- frite failed");
  }
  
  file.close();
}

//读取文件内容
String readFile(fs::FS &fs, const char * path){
  sinfo("读取文件：", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    sinfo("- failed to open file for reading");
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
  Serial.println("HTTP update process started");
}

void update_finished() {
  Serial.println("HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("HTTP update fatal error code %d\n", err);
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

    ota = "0";
    writeFile(LittleFS, "sys");

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
  ota = "0";
  writeFile(LittleFS, "sys");

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

  Serial.println(WiFi.localIP());
  return true;
}

// 显示到网页各种效果
String processor(const String& var) {
  //Serial.println(var);

  if(var == "VERSION")      return VERSION;
  if(var == "BOARD")        return BOARD;
  if(var == "OTAHOST")      return otahost;
  if(var == "IP")           return ip.c_str();
  
  return String();
}

//--------------------------------------------------------------
//websocket部分----------------
void handleRoot(AsyncWebServerRequest *request) //回调函数
{
  Serial.println("User requested.");
  request->send(200, "html", "<p>Hello World!</p>"); //向客户端发送响应和内容
}

// WebSocket事件回调函数
void onEventHandle(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT) // 有客户端建立连接
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u !", client->id()); // 向客户端发送数据
    client->ping();                                    // 向客户端发送ping
  }
  else if (type == WS_EVT_DISCONNECT) // 有客户端断开连接
  {
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR) // 发生错误
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG) // 收到客户端对服务器发出的ping进行应答（pong消息）
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA) // 收到来自客户端的数据
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
    data[len] = 0;
    Serial.printf("%s\n", (char *)data);

    char strc[info->index + len]; // 额外一个位置用于存储字符串结束符 '\0'
    // 将 uint8_t 数组转换为字符串
    for (int i = 0; i < info->index + len; i++) {
      strc[i] = (char)data[i];
    }

    /**
    client->printf((char *)data); // 向客户端发送数据
    /**/

    String input = String(strc);
    int partCount = 0;  
    int pos = 0;  
    
    // 计算部分数量  
    for (int i = 0; i < input.length(); i++) {  
      if (input[i] == '|') {  
        partCount++;  
      }  
    }  
    partCount++; // 因为字符串末尾没有'|'，所以部分数量需要加1  
    
    // 分配字符串数组来存储每个部分  
    String parts[partCount];  
    
    // 切开字符串并存储每个部分  
    for (int i = 0; i < partCount; i++) {  
      int nextPos = input.indexOf('|', pos);  
      if (nextPos == -1) {  
        nextPos = input.length(); // 如果找不到'|'，则到字符串末尾  
      }  
      parts[i] = input.substring(pos, nextPos);  
      pos = nextPos + 1; // 更新位置以查找下一个部分  
    }

    String strMode = parts[0];
    int isHas   = 0;

    //ws2812灯珠
    if(strMode == "ws2812"){isHas = 1; ws2812Test(parts[1], parts[2], parts[3], parts[4]);}

    //sr04距离感应
    if(strMode == "sr04")  {isHas = 1; client->text(sr04Test(parts[1], parts[2]));}

    //switch
    if(strMode == "switch"){isHas = 1; switchTest(parts[1], parts[2]);}

    //oled
    if(strMode == "oled")  {isHas = 1; oledTest(parts[1], parts[2]);}

    if(isHas == 1){
      client->printf("功能命中");
    }else{
      client->printf("功能未命中");
    } 
    client->ping(); 
  }
}
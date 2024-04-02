//接D2
const char* VERSION = "1.0.2 LOCAL";

//是否显示调试信息
const int SHOWINFO = 1;

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

#define intSerial 115200

WiFiClient   espClient;
PubSubClient client(espClient);

//启用webserver
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");    // WebSocket对象，url为/

//OTA 在线更新
const char* OTAURL   = "/esp8266.bin";
const char* OTAFSURL = "/esp8266fs.bin";

//可以通过webserver修改的内容
String ssid;
String pass;
String ip;
String gateway;
String ota;
String otahost;
String checkid;

//临时变量
char buffer[20];

//系统配置
const  char* sysPath = "/sys.txt";

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 255, 0);

//临时变量
String strTmp;
int    intTmp;



#include "oled.h" 
#include <U8g2lib.h>


void oledTest(String pinSCL, String pinSDA) {
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ pinSCL.toInt(), /* data=*/ pinSDA.toInt());

  u8g2.begin();

  u8g2.clearBuffer();  // 清空缓冲区

  u8g2.setFont(u8g2_font_6x10_tf);  // 设置字体和字号

  // 绘制第一行文本
  u8g2.setCursor(0, 10);
  u8g2.print("第一行文本");

  // 绘制第二行文本
  u8g2.setCursor(0, 20);
  u8g2.print("第二行文本");

  u8g2.sendBuffer();  // 将缓冲区内容发送到显示屏

  //delay(1000);  // 延迟1秒钟


} 



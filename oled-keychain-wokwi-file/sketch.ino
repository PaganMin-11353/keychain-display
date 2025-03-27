#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "images.h"

// 屏幕参数
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

// 按钮针脚
#define IMAGE_BUTTON_PIN 2
#define COUNTER_BUTTON_PIN 3

// 电量显示相关
#define BATTERY_PIN A0 // 定义电压测量针脚
#define BATTERY_MIN 320  // 最小电压
#define BATTERY_MAX 420  // 最大电压

// 睡眠时间设定
#define SLEEP_TIMEOUT 10000  // 10s

// 屏幕初始化
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


int currentImageIndex = 0;
int counter = 0;
unsigned long lastActivityTime = 0;
bool displaySleeping = false;
bool counterMode = false;
// 按键记录
bool prevImageButtonState = HIGH;
bool prevCounterButtonState = HIGH;

void setup() {
    Serial.begin(9600);
    Serial.println(F("OLED Keychain starting..."));
    
    pinMode(IMAGE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(COUNTER_BUTTON_PIN, INPUT_PULLUP);
    
    // init display
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); //loop forever if failed to init display
    }
    
    // Clear the display buffer
    display.clearDisplay();
    display.display();
    
    // Initial display settings
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    // 显示启动画面, 等待1s
    displaySplashScreen();
    delay(1000);
    
    // 显示第一张图片
    displayImage(currentImageIndex);
    
    // reset last activity time
    lastActivityTime = millis();
    Serial.println(F("OLED Keychain ready."));
}

void loop() {
    // 读取按钮状态，按下时为LOW
    bool imageButtonState = digitalRead(IMAGE_BUTTON_PIN);
    bool counterButtonState = digitalRead(COUNTER_BUTTON_PIN);
    
    // 图像按钮处理
    // 短按
    if (imageButtonState == LOW && prevImageButtonState == HIGH) {
        // 图像按钮按下
        wakeDisplay();
        
        if (!counterMode) {
            // 图像模式下，切换到下一张图片
            currentImageIndex = (currentImageIndex + 1) % NUM_IMAGES;
            displayImage(currentImageIndex);
            Serial.print(F("Showing image "));
            Serial.println(currentImageIndex);
        } else {
            // 在计数器模式下，切换到图像模式
            counterMode = false;
            displayImage(currentImageIndex);
            Serial.println(F("Switching to image mode"));
        }
    }
    
    // 计数器按钮处理
    if (counterButtonState == LOW && prevCounterButtonState == HIGH) {
        // 计数器按钮按下
        Serial.println(F("Switching to counter mode"));
        wakeDisplay();
        
        if (!counterMode) {
            counterMode = true;
            counter = 0;
        } else {
            counter++;
        }

        displayCounter();
    }
    
    // 更新按钮状态
    prevImageButtonState = imageButtonState;
    prevCounterButtonState = counterButtonState;
    
    // 检查睡眠超时
    if (!displaySleeping && (millis() - lastActivityTime > SLEEP_TIMEOUT)) {
        Serial.println(F("Entering sleep mode"));
        sleepDisplay();
    }
    
    // 每5秒更新一次电池指示器
    if (millis() % 5000 == 0) {
        updateBatteryIndicator();
    }
    
    delay(50); // 小延迟用于按钮去抖和减少CPU使用率
}

void displaySplashScreen() {
    display.clearDisplay();
    
    display.drawBitmap(0, 0, splashScreen, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    
    display.display();
    lastActivityTime = millis();
}

void displayImage(int index) {
    display.clearDisplay();
    updateBatteryIndicator();
    
    // 绘制全屏图像
    display.drawBitmap(0, 0, images[index], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    
    // 在底部显示图像编号
    char imageNumStr[8];
    sprintf(imageNumStr, "%d/%d", index + 1, NUM_IMAGES);
    display.setCursor(0, SCREEN_HEIGHT - 7);
    display.print(imageNumStr);
    
    display.display();
    lastActivityTime = millis();
}

void displayCounter() {
    display.clearDisplay();
    updateBatteryIndicator();
    
    // 显示计数器
    // 使用前导零格式化计数器（7位数）
    char counterStr[8];
    sprintf(counterStr, "%07d", counter);
    
    display.setTextSize(1);
    
    // 绘制7段式数字
    int digitWidth = 7;
    int digitHeight = 15;
    int startX = (SCREEN_WIDTH - (digitWidth * 7)) / 2;
    int startY = (SCREEN_HEIGHT - digitHeight) / 2;
    
    for (int i = 0; i < 7; i++) {
        int digit = counterStr[i] - '0';
        drawSevenSegmentDigit(startX + (i * digitWidth), startY, digit);
    }
    
    display.display();
    lastActivityTime = millis();
}

// 七段显示数字0-9的模式
const byte SEVEN_SEG_DIGITS[10] = {
    0b1111110, // 0
    0b0110000, // 1
    0b1101101, // 2
    0b1111001, // 3
    0b0110011, // 4
    0b1011011, // 5
    0b1011111, // 6
    0b1110000, // 7
    0b1111111, // 8
    0b1111011  // 9
};
  
void drawSevenSegmentDigit(int x, int y, int digit) {
    byte segments = SEVEN_SEG_DIGITS[digit];
    
    // 段位置（a,b,c,d,e,f,g）相对于左上角
    // 段A（顶部水平）
    if (segments & 0b1000000)
      display.fillRect(x+1, y, 5, 1, SSD1306_WHITE);
    
    // 段B（右上垂直）
    if (segments & 0b0100000)
      display.fillRect(x+6, y+1, 1, 6, SSD1306_WHITE);
    
    // 段C（右下垂直）
    if (segments & 0b0010000)
      display.fillRect(x+6, y+8, 1, 6, SSD1306_WHITE);
    
    // 段D（底部水平）
    if (segments & 0b0001000)
      display.fillRect(x+1, y+14, 5, 1, SSD1306_WHITE);
    
    // 段E（左下垂直）
    if (segments & 0b0000100)
      display.fillRect(x, y+8, 1, 6, SSD1306_WHITE);
    
    // 段F（左上垂直）
    if (segments & 0b0000010)
      display.fillRect(x, y+1, 1, 6, SSD1306_WHITE);
    
    // 段G（中间水平）
    if (segments & 0b0000001)
      display.fillRect(x+1, y+7, 5, 1, SSD1306_WHITE);
}

void updateBatteryIndicator() {
    // 读取电池电压
    int rawValue = analogRead(BATTERY_PIN);
    
    // 将ADC值转换为电压
    // 电压分压器使值减半，所以乘以2
    float voltage = rawValue * (5.0 / 1023.0) * 2.0;
    
    // 电压转换为百分比（3.2V = 0%，4.2V = 100%）
    int percentage = map(int(voltage * 100), BATTERY_MIN, BATTERY_MAX, 0, 100);
    percentage = constrain(percentage, 0, 100);
    //Serial.print(F("Batt percentage:"));
    //Serial.println(percentage);
    
    // 右上角
    int indicatorWidth = 10;
    int indicatorHeight = 4;
    int indicatorX = SCREEN_WIDTH - indicatorWidth - 2;
    int indicatorY = 2;
    
    // 绘制电池外框
    display.drawRect(indicatorX, indicatorY, indicatorWidth, indicatorHeight, SSD1306_WHITE);
    
    // 根据百分比填充电池
    int fillWidth = (percentage / 100.0) * (indicatorWidth - 2);
    display.fillRect(indicatorX + 1, indicatorY + 1, fillWidth, indicatorHeight - 2, SSD1306_WHITE);
    
    // 在不刷新整个屏幕的情况下更新
    display.display();
}

void sleepDisplay() {
    // 通过降低对比度使显示变暗
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(0); // 最小对比度
    
    displaySleeping = true;
}
  
void wakeDisplay() {
    if (displaySleeping) {
      // 将显示恢复到正常对比度
      Serial.println(F("Waking up"));
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(0x8F); // 默认对比度
      
      displaySleeping = false;
    }
    
    lastActivityTime = millis();
}
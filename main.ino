#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

// Button pins
#define IMAGE_BUTTON_PIN 2
#define COUNTER_BUTTON_PIN 3

// Battery measurement
#define BATTERY_PIN A0
#define BATTERY_MIN 320  // Minimum voltage
#define BATTERY_MAX 420  // Maximum voltage

// Sleep/Timeout settings
#define SLEEP_TIMEOUT 10000  // 10 seconds in milliseconds

// Initialize the display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables
int currentImageIndex = 0;
int counter = 0;
unsigned long lastActivityTime = 0;
bool displaySleeping = false;
bool counterMode = false;

// Button state tracking
bool prevImageButtonState = HIGH;
bool prevCounterButtonState = HIGH;

// Sample images (16x16 bitmaps expanded to display size)
// This is a placeholder - you'll replace these with your actual images
const unsigned char PROGMEM image1[] = {
  // 16x16 smiley face bitmap
  0x00, 0x00, 0x07, 0xE0, 0x08, 0x10, 0x10, 0x08, 0x21, 0x84, 0x21, 0x84,
  0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x21, 0x84, 0x21, 0x84,
  0x10, 0x08, 0x08, 0x10, 0x07, 0xE0, 0x00, 0x00
};

const unsigned char PROGMEM image2[] = {
  // 16x16 sad face bitmap
  0x00, 0x00, 0x07, 0xE0, 0x08, 0x10, 0x10, 0x08, 0x21, 0x84, 0x21, 0x84,
  0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x11, 0x88, 0x11, 0x88,
  0x10, 0x08, 0x08, 0x10, 0x07, 0xE0, 0x00, 0x00
};

const unsigned char PROGMEM image3[] = {
  // 16x16 heart bitmap
  0x00, 0x00, 0x0C, 0x30, 0x12, 0x48, 0x21, 0x84, 0x21, 0x84, 0x40, 0x02,
  0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40,
  0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Array of image pointers for easy cycling
const unsigned char* images[] = {image1, image2, image3};
const int NUM_IMAGES = 3;

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
    
    // TODO: set a splash screen
    // Show initial image
    displayImage(currentImageIndex);
    
    // Reset last activity time
    lastActivityTime = millis();
    Serial.println(F("OLED Keychain ready."));
}

void loop() {
    // read button states, LOW when pressed
    bool imageButtonState = digitalRead(IMAGE_BUTTON_PIN);
    bool counterButtonState = digitalRead(COUNTER_BUTTON_PIN);
    
    if (imageButtonState == LOW && prevImageButtonState == HIGH) {
      // Image button pressed
      Serial.println(F("Switching to image mode"));
      wakeDisplay();
      counterMode = false;
      currentImageIndex = (currentImageIndex + 1) % NUM_IMAGES;
      displayImage(currentImageIndex);
    }
    
    if (counterButtonState == LOW && prevCounterButtonState == HIGH) {
      // Counter button pressed
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
    
    prevImageButtonState = imageButtonState;
    prevCounterButtonState = counterButtonState;
    
    // check sleep timeout
    if (!displaySleeping && (millis() - lastActivityTime > SLEEP_TIMEOUT)) {
        Serial.println(F("Entering sleep mode"));
        sleepDisplay();
    }
    
    // update battery indicator every 5 seconds
    if (millis() % 5000 == 0) {
      updateBatteryIndicator();
    }
    
    delay(50); // Small delay to debounce buttons and reduce CPU usage
}

void displayImage(int index) {
    display.clearDisplay();
    updateBatteryIndicator();
    
    // Draw the image centered on screen
    // TODO
    display.drawBitmap((SCREEN_WIDTH - 16) / 2, (SCREEN_HEIGHT - 16) / 2, 
                       images[index], 16, 16, SSD1306_WHITE);
    
    display.display();
    lastActivityTime = millis();
}

void displayCounter() {
    display.clearDisplay();
    updateBatteryIndicator();
    
    // Display counter
    // Format counter with leading zeros (7 digits)
    char counterStr[8];
    sprintf(counterStr, "%07d", counter);
    
    display.setTextSize(1);
    
    // Draw 7-segment style digits
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

// Seven Segment pattern for digits 0-9
const byte SEVEN_SEG_DIGITS[10] = {
    0b1111110, // 0
    0b0110000, // 1
    0b1011011, // 2
    0b1111001, // 3
    0b0110101, // 4
    0b1101101, // 5
    0b1101111, // 6
    0b1110000, // 7
    0b1111111, // 8
    0b1111101  // 9
};
  
void drawSevenSegmentDigit(int x, int y, int digit) {
    byte segments = SEVEN_SEG_DIGITS[digit];
    
    // Segment positions (a,b,c,d,e,f,g) relative to top-left corner
    // Segment A (top horizontal)
    if (segments & 0b1000000)
      display.fillRect(x+1, y, 5, 1, SSD1306_WHITE);
    
    // Segment B (top-right vertical)
    if (segments & 0b0100000)
      display.fillRect(x+6, y+1, 1, 6, SSD1306_WHITE);
    
    // Segment C (bottom-right vertical)
    if (segments & 0b0010000)
      display.fillRect(x+6, y+8, 1, 6, SSD1306_WHITE);
    
    // Segment D (bottom horizontal)
    if (segments & 0b0001000)
      display.fillRect(x+1, y+14, 5, 1, SSD1306_WHITE);
    
    // Segment E (bottom-left vertical)
    if (segments & 0b0000100)
      display.fillRect(x, y+8, 1, 6, SSD1306_WHITE);
    
    // Segment F (top-left vertical)
    if (segments & 0b0000010)
      display.fillRect(x, y+1, 1, 6, SSD1306_WHITE);
    
    // Segment G (middle horizontal)
    if (segments & 0b0000001)
      display.fillRect(x+1, y+7, 5, 1, SSD1306_WHITE);
}


void updateBatteryIndicator() {
    // Read battery voltage
    int rawValue = analogRead(BATTERY_PIN);
    
    // Convert ADC value to voltage
    // voltage divider halves the value so times 2
    float voltage = rawValue * (5.0 / 1023.0) * 2.0;
    
    // voltage to percentage (3.2V = 0%, 4.2V = 100%)
    int percentage = map(int(voltage * 100), BATTERY_MIN, BATTERY_MAX, 0, 100);
    int percentage = constrain(percentage, 0, 100);
    Serial.print(F("Batt percentage:"));
    Serial.println(percentage);
    
    // top right corner
    int indicatorWidth = 10;
    int indicatorHeight = 4;
    int indicatorX = SCREEN_WIDTH - indicatorWidth - 2;
    int indicatorY = 2;
    
    // Draw battery outline
    display.drawRect(indicatorX, indicatorY, indicatorWidth, indicatorHeight, SSD1306_WHITE);
    
    // Fill battery based on percentage
    int fillWidth = (percentage / 100.0) * (indicatorWidth - 2);
    display.fillRect(indicatorX + 1, indicatorY + 1, fillWidth, indicatorHeight - 2, SSD1306_WHITE);
    
    display.display();
}

void sleepDisplay() {
    // Dim the display by reducing contrast
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(0); // Minimum contrast
    
    displaySleeping = true;
}
  
void wakeDisplay() {
    if (displaySleeping) {
      // Reset display to normal contrast
      Serial.println(F("Waking up"));
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(0x8F); // Default contrast
      
      displaySleeping = false;
    }
    
    lastActivityTime = millis();
}
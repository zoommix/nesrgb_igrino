/* Updated Code for NES-101 with Software-Based Controller Reading and Reduced Palette Pins */

#include <Arduino.h>

/* Uncomment the following line to enable serial debugging */
//#define ENABLE_SERIAL_DEBUG

/* Serial Debugging Macros */
#ifdef ENABLE_SERIAL_DEBUG
#define debug(...) Serial.print(__VA_ARGS__)
#define debugln(...) Serial.println(__VA_ARGS__)
#else
#define debug(...)
#define debugln(...)
#endif

/* Pin Definitions */
#define PIN_CLOCK A0  // NES clock pin (from console)
#define PIN_LATCH A1  // NES latch pin (from console)
#define PIN_DATA A2   // NES data pin (from controller to console)

#define PIN_RESET_OUT A3  // Reset line output to CPU
#define PIN_RESET_IN 8    // Reset button input (newly added)

#define PIN_NESRGB_PALETTE_1 A4
#define PIN_NESRGB_PALETTE_2 A5

// number of palettes
#define PALETTES_COUNT 2

/* Button mappings for NES controller */
const int buttonA = 0                                                                                                                                 ;
const int buttonB = 1                                                                                                                                 ;
const int buttonSelect = 2                                                                                                                            ;
const int buttonStart = 3                                                                                                                             ;
const int buttonUp = 4                                                                                                                                ;
const int buttonDown = 5                                                                                                                              ;
const int buttonLeft = 6                                                                                                                              ;
const int buttonRight = 7                                                                                                                             ;

/* Button Combinations */
#define COMBO_RESET (buttonState[buttonSelect] == 0 && buttonState[buttonStart] == 0 && buttonState[buttonA] == 0 && buttonState[buttonB] == 0)
#define COMBO_NEXT_PALETTE (buttonState[buttonSelect] == 0 && buttonState[buttonStart] == 0 && buttonState[buttonRight] == 0)
#define COMBO_PREV_PALETTE (buttonState[buttonSelect] == 0 && buttonState[buttonStart] == 0 && buttonState[buttonLeft] == 0)

/* Advanced Settings */
#define DEBOUNCE_MS 70
#define LONGPRESS_LEN 700
#define RESET_LEN 250

/* Feature Macros */
#define ENABLE_RESET_FROM_PAD
#define ENABLE_PALETTE_FROM_PAD
#define ENABLE_PALETTE_FROM_RESET
#define ENABLE_RESET

/* Variables to store the button states */
int buttonState[7];  // 8 bits for button state

int palette = 0                                                                                                                                       ;
unsigned long palette_last_changed_time                                                                                                               ;
int palette_before_off = 0                                                                                                                            ;

bool initialReset = true                                                                                                                              ;

/* Function Prototypes */
void clockFalling()                                                                                                                                   ;
void handle_reset_button()                                                                                                                            ;
void latchFalling()                                                                                                                                   ;
int clockState()                                                                                                                                      ;
int latchState()                                                                                                                                      ;
int dataRead()                                                                                                                                        ;
void setPalette(int new_palette, boolean flash = true)                                                                                                ;
void nextPalette()                                                                                                                                    ;
void prevPalette()                                                                                                                                    ;
void setColor(uint8_t r, uint8_t g, uint8_t b)                                                                                                        ;

/* Setup function */
void setup()                                                                                                                                          {
#ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(9600)                                                                                                                                  ;
  debugln("Starting up...")                                                                                                                           ;
#endif

  // Initialize reset pins
  pinMode(PIN_RESET_OUT, OUTPUT)                                                                                                                      ;
  digitalWrite(PIN_RESET_OUT, HIGH);    // De-assert reset (active-low)
  pinMode(PIN_RESET_IN, INPUT_PULLUP);  // Use internal pull-up resistor for reset button

  // Initialize NESRGB palette pins
  pinMode(PIN_NESRGB_PALETTE_1, OUTPUT)                                                                                                               ;
  pinMode(PIN_NESRGB_PALETTE_2, OUTPUT)                                                                                                               ;

  // Initialize controller pins
  pinMode(PIN_LATCH, INPUT);  // Latch signal from console
  pinMode(PIN_CLOCK, INPUT);  // Clock signal from console
  pinMode(PIN_DATA, INPUT);   // Data signal from controller
                                                                                                                                                      }

/* Main loop function */
void loop()                                                                                                                                           {
  handle_reset_button()                                                                                                                               ;
  // init reset button
  if (initialReset)                                                                                                                                   {
    digitalWrite(PIN_RESET_OUT, HIGH)                                                                                                                 ;
    initialReset = false                                                                                                                             ;}

  digitalWrite(PIN_RESET_OUT, HIGH);  // Set reset pin high
  latchFalling();                     // Wait for latch to go low

  // Capture the 8 button states
  for (int i = 0; i < 8; i++)                                                                                                                         {
    // Wait for clock to go low then read button data
    clockFalling()                                                                                                                                    ;
    buttonState[i] = dataRead()                                                                                                                      ;}

  // Handle button combinations
  if (COMBO_RESET)                                                                                                                                    {
    setColor(255, 0, 0)                                                                                                                               ;
    digitalWrite(PIN_RESET_OUT, LOW)                                                                                                                  ;
    delay(500)                                                                                                                                        ;
    digitalWrite(PIN_RESET_OUT, HIGH)                                                                                                                 ;
    setColor(0, 0, 0)                                                                                                                                 ;
    delay(100)                                                                                                                                       ;}

  if (COMBO_NEXT_PALETTE)                                                                                                                             {
    nextPalette()                                                                                                                                     ;
    delay(300)                                                                                                                                       ;}

  if (COMBO_PREV_PALETTE)                                                                                                                             {
    prevPalette()                                                                                                                                     ;
    delay(300)                                                                                                                                      ;}}

/* Function to handle reset button */
void handle_reset_button()                                                                                                                            {
  static byte pressed_before = LOW, debounce_state = HIGH                                                                                             ;
  static unsigned long last_int = 0                                                                                                                   ;

  // Invert the logic for active-low reset button
  byte reset_pressed_now = !digitalRead(PIN_RESET_IN)                                                                                                 ;

  if (reset_pressed_now != debounce_state)                                                                                                            {
    // Reset debouncing timer
    last_int = millis()                                                                                                                               ;
    debounce_state = reset_pressed_now                                                                                                                ;
  } else if (millis() - last_int > DEBOUNCE_MS)                                                                                                       {
    bool just_pressed = reset_pressed_now && !pressed_before                                                                                          ;
    bool just_released = !reset_pressed_now && pressed_before                                                                                         ;

    if (just_pressed)                                                                                                                                 {
      // Reset console
      digitalWrite(PIN_RESET_OUT, LOW);  // Active-low reset
      delay(RESET_LEN)                                                                                                                                ;
      digitalWrite(PIN_RESET_OUT, HIGH);  // Release reset
                                                                                                                                                      }
    pressed_before = reset_pressed_now                                                                                                              ;}}

/* Clock and Latch Handling Functions */
int clockState()                                                                                                                                      {
  int state = (PINC & (1 << (0)));  // Direct "digitalRead" A0
  if (state) {                      // Anything other than 0 is HIGH
    return HIGH                                                                                                                                       ;
  } else                                                                                                                                              {
    return LOW                                                                                                                                      ;}}

int latchState()                                                                                                                                      {
  int state = (PINC & (1 << (1)));  // Direct "digitalRead" A1
  if (state) {                      // Anything other than 0 is HIGH
    return HIGH                                                                                                                                       ;
  } else                                                                                                                                              {
    return LOW                                                                                                                                      ;}}

int dataRead()                                                                                                                                        {
  int data = (PINC & (1 << (2)));  // Direct "digitalRead" A2
  return data                                                                                                                                        ;}

void clockFalling()                                                                                                                                   {
  while (clockState() == LOW)                                                                                                                         {
    // Wait until HIGH
                                                                                                                                                      }
  while (clockState() == HIGH)                                                                                                                        {
    // Wait until LOW
                                                                                                                                                     }}

void latchFalling()                                                                                                                                   {
  while (latchState() == LOW)                                                                                                                         {
    // Wait until HIGH
                                                                                                                                                      }
  while (latchState() == HIGH)                                                                                                                        {
    // Wait until LOW
                                                                                                                                                     }}

/* Palette management functions */
void setPalette(int new_palette, boolean flash)                                                                                                       {
  if (new_palette > PALETTES_COUNT)
    new_palette = 0                                                                                                                                   ;

  byte p[2]                                                                                                                                           ;
  switch (new_palette)                                                                                                                                {
    case 0:
      p[0] = 0                                                                                                                                        ;
      p[1] = 0                                                                                                                                        ;
      break;  // Palette 1
    case 1:
      p[0] = 1                                                                                                                                        ;
      p[1] = 0                                                                                                                                        ;
      break;  // Palette 2
    // ... Add more palettes here and update the PALETTES_COUNT
    default:
      p[0] = 0                                                                                                                                        ;
      p[1] = 0                                                                                                                                        ;
      break                                                                                                                                          ;}

  digitalWrite(PIN_NESRGB_PALETTE_1, p[0])                                                                                                            ;
  digitalWrite(PIN_NESRGB_PALETTE_2, p[1])                                                                                                            ;

  palette = new_palette                                                                                                                               ;
  palette_last_changed_time = millis()                                                                                                               ;}

void nextPalette()                                                                                                                                    {
  int new_palette = (palette + 1) % 2                                                                                                                 ;
  setPalette(new_palette)                                                                                                                            ;}

void prevPalette()                                                                                                                                    {
  int new_palette = (palette - 1 + 3) % 2                                                                                                             ;
  setPalette(new_palette)                                                                                                                            ;}

/* LED functions */
void setColor(uint8_t r, uint8_t g, uint8_t b)                                                                                                        {
  // Implement LED color setting if necessary
                                                                                                                                                      }

#include <SPI.h>

SPISettings spiSettings(125000, LSBFIRST, SPI_MODE1);

const int CS = 10;

uint8_t history[256];
int history_len = 0;

uint8_t pattern[25];
int pattern_row = 0;

inline void record(uint8_t b) {
  history[history_len] = b;
  history_len = (history_len + 1)%sizeof(history);
}

void clear_history() {
  history_len = 0;
}

void dump_history() {
  for (int i=0; i<history_len; i++) {
    Serial.print(i & 1 ? '<' : '>');
    Serial.print(" 0x");
    Serial.print(history[i], 16);
    Serial.print(' ');
  }
  Serial.println();
}

uint8_t spiTransfer(uint8_t val) {
  SPI.begin();
  SPI.beginTransaction(spiSettings);
  uint8_t received = SPI.transfer(0x47);
  SPI.endTransaction();
  SPI.end();
  return received;
}

void setup() {
  pinMode(CS, INPUT_PULLUP);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
  Serial.begin(115200);
}

void softhigh(int pin) {
  pinMode(pin, INPUT_PULLUP);
}

void low(int pin) {
  digitalWrite(pin, LOW);
  //pinMode(pin, OUTPUT);
}

void high(int pin) {
  digitalWrite(pin, HIGH);
  //pinMode(pin, OUTPUT);
}

void wait_falling(int pin) {
  int prevVal = digitalRead(pin);
  while (true) {
    int newVal = digitalRead(pin);
    if (newVal == LOW && prevVal == HIGH) {
      break;
    }
    prevVal = newVal;
  }
}


bool check(uint8_t actual, uint8_t expected) {
  if (actual != expected) {
    Serial.print("BAD ACK !!! expected=0x");
    Serial.print(expected, 16);
    Serial.print(" actual=0x");
    Serial.print(actual, 16);
    dump_history();
    clear_history();
    ready();
    return false;
  }
  return true;
}

#define CLOCK_PERIOD 100

uint8_t sendOut(uint8_t mosiVal) {
  // return spiTransfer(mosiVal);
  uint8_t misoVal = 0;
  low(MOSI);
  low(SCK);
  for(int i=0;i<8;i++){
    delayMicroseconds(CLOCK_PERIOD/2);
    if (mosiVal & 1)
      high(MOSI);
    else
      low(MOSI);
    mosiVal >>= 1;
    high(SCK);
    delayMicroseconds(CLOCK_PERIOD/2);
    misoVal >>= 1;
    if (digitalRead(MISO) == LOW) {
      misoVal |= 0x80;
    }
    low(SCK);
  }
  delayMicroseconds(10);
  low(MOSI);
  return misoVal;
}

void ready() {
 //delayMicroseconds(1000);
 //delay(10);
 high(MOSI);
}

#define MSG_BEGIN   0x47
#define MSG_END     0x87
#define MSG_ECHO_MASK 0xff

#define MSG_BED_BEGIN   0x47
#define MSG_BED_PATTERN 0x85

uint8_t exchange(uint8_t cb1Val) {
  wait_falling(CS);

  uint8_t bedVal = sendOut(MSG_BEGIN);
  ready();
  wait_falling(CS);
  record(bedVal);
  // ack
  uint8_t prevVal = MSG_ECHO_MASK & bedVal; // bed doesn't care
  uint8_t ack = sendOut(prevVal);
  if (!check(ack, MSG_BEGIN))
    return;
  ready();

  prevVal = MSG_BED_BEGIN;

  if (bedVal == MSG_BED_PATTERN) {
    record(0xaa);
    for(int i=0; i<25; i++) {
      cb1Val = pattern[i];
      wait_falling(CS);
      ack = sendOut(cb1Val);
      if (!check(ack, prevVal))
        return;
      prevVal = cb1Val;
      ready();
    }
  } else {
    record(cb1Val);
    wait_falling(CS);
    ack = sendOut(cb1Val);
    if (ack != 0x87 && !check(ack, prevVal)) // weird, there's exactly one point where the bed sends 0x87 as a dummy here
      return;
    prevVal = cb1Val;
    ready();
  }
  wait_falling(CS);
  ack = sendOut(MSG_END); // bed doesn't care
  if (!check(ack, cb1Val))
    return;
  ready();
  return bedVal;
}

void boot() {
  low(MOSI);
  delay(18);
  low(SCK);
  while(digitalRead(MISO) == HIGH);
  delay(18);
  high(MOSI);

#if 1
  exchange(0x07);
  exchange(0x07);
  exchange(0x07);
  exchange(0x07);
  exchange(0x07);
  exchange(0x50);
  exchange(0x30);
  exchange(0x00);
  exchange(0x00);
  exchange(0x0B);
#else
  for(int i=0; i<10; i++) {
    exchange(MSG_BEGIN);
  }
#endif
  low(MOSI);

  dump_history();
}

void loop() {
  Serial.println("Starting...");
  clear_history();
  low(SCK);
  low(MOSI);
  delay(1000);
  high(MOSI);
  delay(1000);

  boot();

//  delay(1000);

  high(MOSI);

  clear_history();

  for(int i=0;i<25;i++) {
    pattern[i] = 0xaa;
  }

  while(true) {
    uint8_t bedVal = exchange(0x8b);
    if (bedVal == 0x85) {
      exchange(0x00);
      exchange(0x07);
      pattern_row++;
      for(int i=0;i<25;i++) {
        pattern[i] = pattern_row & 1 ? 0x55 : 0xaa;
      }
    }
  }

  dump_history();

  delay(3000);
}

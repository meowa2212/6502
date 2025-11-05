#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13
unsigned long size = 32768; // size of the eeprom in bits

#include <avr/pgmspace.h> // added for PROGMEM access

/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setAddress(int address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

/*
 * Read a byte from the EEPROM at the specified address.
 */
byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}

/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  for (int tries = 0; tries < 3; tries += 1) {
    if (readEEPROM(address) == data) {
      return;
    }

    setAddress(address, /*outputEnable*/ false);
    delayMicroseconds(1);

    for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
      pinMode(pin, OUTPUT);
    }

    byte temp = data;
    for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
      digitalWrite(pin, temp & 1);
      temp = temp >> 1;
    }
    digitalWrite(WRITE_EN, LOW);
    delayMicroseconds(1);
    digitalWrite(WRITE_EN, HIGH);
    delay(5);
  }
}

/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents() {
  readEEPROM(0x0000); // dummy read

  for (unsigned long base = 0; base < size; base += 16) {
    byte data[16];
    for (unsigned long offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%04lX:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
    Serial.println(buf);
  }
}

// Erase entire EEPROM
void eraseEEPROM() {
  Serial.println(" ");
  Serial.println("Erasing EEPROM");

  float progress = 0;
  for (unsigned long address = 0; address < size; address += 1) {
    if (readEEPROM(address) != 0x00) {
      writeEEPROM(address, 0x00);
    }

    if (address % 1024 == 0) {
      Serial.print((progress / 32.00) * 100);
      Serial.println("%");
      progress += 1;
    }
  }
  Serial.println(" done");
}

const byte data[] PROGMEM = {
    0xa9, 0xff, 0x8d, 0x02, 0x60, 0xa9, 0x50, 0x8d, 0x00, 0x60, 0x6a, 0x8d, 0x00, 0x60, 0x4c, 0x0a,
    0x80
};

void setup() {
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  Serial.begin(57600);

  eraseEEPROM();

  // Program data bytes from flash
  Serial.print("Programming EEPROM");
  for (int address = 0; address < sizeof(data); address += 1) {
    byte value = pgm_read_byte(&data[address]); // read from PROGMEM
    writeEEPROM(address, value);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done");

  writeEEPROM(0x7ffc, 0x00);
  writeEEPROM(0x7ffd, 0x80);

  Serial.println("Reading EEPROM");
  printContents();
}

void loop() {
}

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13
unsigned long size = 32768; // size of the eeprom in bytes


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


void setup() {
    pinMode(SHIFT_DATA, OUTPUT);
    pinMode(SHIFT_CLK, OUTPUT);
    pinMode(SHIFT_LATCH, OUTPUT);
    digitalWrite(WRITE_EN, HIGH);
    pinMode(WRITE_EN, OUTPUT);
    Serial.begin(57600);
}

void loop() {
    byte dataByte = 0x00; // Initialize dataByte to zero
    Serial.print("S");
    while (Serial.available() < 2) {
    }
    byte highByte = Serial.read();
    byte lowByte = Serial.read();
    int address = (highByte << 8) | lowByte;

    Serial.print("R");
    bool transfer = true;
    while (transfer) {
        while (Serial.available() == 0) {
        }
        dataByte = Serial.read();
        switch (dataByte) {
            case 'H':
                printContents();
                transfer = false;
                break;
            default:
                writeEEPROM(address, dataByte);
                address = address + 1;
                Serial.print("R");
                break;
        }
    }
}

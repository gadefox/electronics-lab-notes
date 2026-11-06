#include <Wire.h>

#define LOBYTE(x)  ((uint8_t)((x) & 0xFF))
#define HIBYTE(x)  ((uint8_t)(((x) >> 8) & 0xFF))
#define TOWORD(low, high) ((uint16_t)(uint8_t)(low) | ((uint16_t)(uint8_t)(high) << 8))

#define GT_ADDR  0x5D

#define PIN_SDA  4   // D2
#define PIN_SCL  5   // D1
#define PIN_RST  14  // D5
#define PIN_INT  12  // D6

static uint8_t gt_maxpoints;
static bool gt_initialized;

static inline void gt_write_reg(uint16_t reg) {
  Wire.write(HIBYTE(reg));
  Wire.write(LOBYTE(reg));
}

static bool gt_read(uint8_t addr, uint16_t reg, uint8_t *buf, size_t len) {
  Wire.beginTransmission(addr);
  gt_write_reg(reg);

  if (Wire.endTransmission(false) != 0)
    return false;   // repeated start
  if (Wire.requestFrom(addr, (uint8_t)len) != len)
    return false;

  for (size_t i = 0; i < len; i++)
    buf[i] = Wire.read();

  return true;
}

static void gt_reset(void) {
  pinMode(PIN_INT, OUTPUT);
  digitalWrite(PIN_INT, LOW);

  digitalWrite(PIN_RST, LOW);
  delay(10);
  digitalWrite(PIN_RST, HIGH);
  delay(5);

  pinMode(PIN_INT, INPUT);
  delay(60);
}

static bool gt_write8(uint16_t reg, uint8_t val) {
  Wire.beginTransmission(GT_ADDR);
  gt_write_reg(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static void gt_print_config() {
  uint8_t c[8];

  if (!gt_read(GT_ADDR, 0x8047, c, 8)) {
    Serial.println("error: config read fail");
    return;
  }

  Serial.printf("config version: 0x%02X\r\n", c[0]);
  Serial.printf("geometry: width=%u height=%u\r\n", c[1] | (c[2] << 8), c[3] | (c[4] << 8));

  gt_maxpoints = c[5] & 0x0F;
  Serial.printf("max touch: %u\r\n", gt_maxpoints);
}

static bool gt_poll() {
  uint8_t st;

  if (!gt_initialized)
    return false;

  if (!gt_read(GT_ADDR, 0x814E, &st, 1))
    return false;

  // Bit 7: touch data is ready
  if (!(st & 0x80))
    return false;

  // Bits 0-3: number of touch points
  uint8_t count = st & 0x0F;

  for (uint8_t i = 0; i < count; i++) {
    uint8_t p[8];

    if (gt_read(GT_ADDR, 0x814F + i * 8, p, sizeof(p))) {
      uint16_t x = TOWORD(p[1], p[2]);
      uint16_t y = TOWORD(p[3], p[4]);
      uint16_t s = TOWORD(p[5], p[6]);
      Serial.printf("T%u id=%u x=%u y=%u size=%u\r\n", i, p[0], x, y, s);
    }
  }

  gt_write8(0x814E, 0);
  return true;
}

static void gt_init() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  Wire.setClockStretchLimit(150000);

  pinMode(PIN_RST, OUTPUT);
  gt_reset();

  uint8_t id[4] = {0};
  if (!gt_read(GT_ADDR, 0x8140, id, sizeof(id))) {
    Serial.printf("error: no response @ 0x%02X\r\n", GT_ADDR);
    return;
  }

  Serial.printf("product ID: %c%c%c%c\r\n", id[0], id[1], id[2], id[3]);
  gt_print_config();
  gt_initialized = true;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  gt_init();
}

void loop() {
  if (gt_poll())
    delay(10);
}

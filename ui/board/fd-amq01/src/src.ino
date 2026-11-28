#define count_of(arr) (sizeof(arr) / sizeof((arr)[0]))

#define LOGS  0

#define PIN_BUZZER  D12

#define PIN6   D11
#define PIN7   D10
#define PIN8   D9
#define PIN9   D8
#define PIN10  D4
#define PIN11  D5
#define PIN12  D6
#define PIN13  D7

//==========================
// Pin mapping

static const uint8_t matrix[] = { PIN6, PIN7, PIN8, PIN9, PIN10, PIN11, PIN12, PIN13 };

static inline void matrix_reset(void) {
  for (int i = 0; i < count_of(matrix); i++)
    pinMode(matrix[i], INPUT);
}

//==========================
// Buzzer

static bool beep_active;
static unsigned long beep_start;

static inline void buz_init(void) {
  pinMode(PIN_BUZZER, OUTPUT);
}

static bool buz_beep(void) {
  if (beep_active)
    return false;

  beep_active = true;
  beep_start = millis();
  tone(PIN_BUZZER, 2700);
  return true;
}

static void buz_mux(void) {
  if (!beep_active)
    return;

  if (millis() - beep_start < 100)
    return;

  noTone(PIN_BUZZER);
  beep_active = false;
}

//==========================
// LED charlieplexing

typedef struct {
  uint8_t lo;
  uint8_t hi;
  uint8_t state;
} led_t;

static led_t leds[] = {
  { PIN11, PIN8, 1 },  // LED1
  { PIN11, PIN7, 0 },  // LED2
  { PIN11, PIN6, 0 },  // LED3
  { PIN12, PIN6, 0 },  // LED4
  { PIN12, PIN7, 0 },  // LED5
  { PIN12, PIN8, 1 },  // LED6
  { PIN12, PIN9, 0 },  // LED7
  { PIN10, PIN6, 1 },  // LED8
  { PIN10, PIN7, 0 },  // LED9
  { PIN10, PIN8, 0 },  // LED10
  { PIN10, PIN9, 1 },  // LED11
  { PIN13, PIN6, 0 },  // LED12
  { PIN13, PIN7, 0 },  // LED13
  { PIN13, PIN8, 1 },  // LED14
  { PIN13, PIN9, 0 }   // LED15
};

static inline void led_toggle(int index) {
  led_t *led = leds + index;
  led->state ^= 1;
}

static inline void led_on(int lo, int hi) {
  pinMode(lo, OUTPUT);
  digitalWrite(lo, LOW);

  pinMode(hi, OUTPUT);
  digitalWrite(hi, HIGH);
}

static int leds_mux(int index) {
  if (index >= count_of(leds))
    return count_of(leds);

  led_t *led = leds + index;
  if (led->state)
    led_on(led->lo, led->hi);

  return -1;
}

//==========================
// Buttons

typedef struct {
  uint8_t a;
  uint8_t b;
  uint8_t last;
} btn_t;

static btn_t btns[] = {
  { PIN11, PIN7, 0 },  // SW1
  { PIN12, PIN7, 0 },  // SW2
  { PIN12, PIN9, 0 },  // SW3
  { PIN10, PIN7, 0 },  // SW4
  { PIN13, PIN9, 0 },  // SW5
  { PIN11, PIN9, 0 },  // SW6
  { PIN10, PIN9, 0 }   // SW7
};

static bool btn_read(int a, int b) {
  pinMode(a, INPUT_PULLUP);
  pinMode(b, OUTPUT);
  digitalWrite(b, LOW);
  return digitalRead(a) == LOW;
}

static bool btn_pressed(int index) {
  btn_t *btn = btns + index;

  bool pressed = btn_read(btn->a, btn->b);
  bool edge = pressed && !btn->last;  // raising edge
  btn->last = pressed;
  return edge;
}

//==========================
// Single LED

typedef struct {
  uint8_t btn;
  uint8_t led;
} single_t;

single_t singles[] = {
  { 2, 6 },
  { 4, 14 },
  { 5, 10 }
};

static int singles_mux(int index) {
  if (index >= count_of(singles))
    return count_of(singles);

  single_t *single = singles + index;
  if (!btn_pressed(single->btn))
    return -1;

  led_toggle(single->led);

#if LOGS
  char buf[12];
  sprintf(buf, "single=%d", index);
  Serial.println(buf);
#endif

  buz_beep();
  return -1;
}

//==========================
// LED group

typedef struct {
  uint8_t btn;
  uint8_t led;
  uint8_t count;
  uint8_t dir;
  uint8_t index;
} group_t;

group_t groups[] = {
  { 0, 0, 3, 1, 0 },
  { 1, 3, 3, 0, 2 },
  { 3, 7, 3, 1, 0 },
  { 6, 11, 3, 0, 2 }
};

static inline int next_index(bool dir, int index, int count) {
  int step = dir ? 1 : count - 1;
  return (index + step) % count;

/*
  if (dir) {
    if (++index == count)
      return 0;
    return index;
  }

  if (index == 0)
    return count - 1;
  return index - 1;
*/
}

static int groups_mux(int index) {
  if (index >= count_of(groups))
    return count_of(groups);

  group_t *group = groups + index;
  if (!btn_pressed(group->btn))
    return -1;

  led_toggle(group->led + group->index);
  group->index = next_index(group->dir, group->index, group->count);
  led_toggle(group->led + group->index);

#if LOGS
  char buf[22];
  sprintf(buf, "group=%d index=%d", index, group->index);
  Serial.println(buf);
#endif

  buz_beep();
  return -1;
}

//==========================
// Multiplex

#if LOGS
static uint32_t start;
#endif

static uint8_t phase;

static void mux(void) {
  matrix_reset();

  int index = phase;
  int ret = singles_mux(index);
  if (ret == -1) {
    phase++;
    return;
  }

  index -= ret;
  ret = groups_mux(index);
  if (ret == -1) {
    phase++;
    return;
  }

  index -= ret;
  ret = leds_mux(index);
  if (ret == -1) {
    phase++;
    return;
  }

  buz_mux();

#if LOGS
  uint32_t end = micros();
  float time = (end - start) / 1000.0;
  int freq = 1000.0 / time;

  char buf[64];
  sprintf(buf, "frame: %d ms, %d Hz, phases=%d", (int)time, freq, phase + 1);
  Serial.println(buf);

  start = end;
#endif

  phase = 0;
}

//==========================
// Setup

void setup(void) {
  buz_init();

#if LOGS
  Serial.begin(115200);
  while (!Serial);
#endif
}

//==========================
// Loop

void loop(void) {
  mux();
  delayMicroseconds(600);
}

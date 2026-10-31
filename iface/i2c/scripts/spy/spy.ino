#define PINS  ((GPI >> 12) & 0b101)

void setup(void) {
  Serial.begin(115200);
  Serial.flush();

  pinMode(D5, INPUT);  // GPIO14
  pinMode(D6, INPUT);  // GPIO12
}

#define MAX  50000
uint8_t logs[MAX];

void print_log(uint8_t log) {
  Serial.print(log & 1);     // D6
  Serial.print(" ");
  Serial.println(log >> 2);  // D5
}

void print_logs(size_t n) {
  for (size_t i = 0; i < n; i++)
    print_log(logs[i]);
}

uint32_t cnt2us(uint32_t maxcnt) {
  uint32_t cnt = 0;
  uint32_t prev = PINS;
  uint32_t start = micros();

  while (true) {
    uint32_t pins = PINS;
    if (prev == pins) {
      if (++cnt == maxcnt) break;
      continue;
    }
    prev = pins;
  }
  return micros() - start;
}

void print_cntus(uint32_t cnt, uint32_t us) {
  Serial.print("count: ");
  Serial.print(cnt);
  Serial.print(" us: ");
  Serial.println(us);
}

void calibrate(void) {
  for (int cnt = 1000000; cnt <= 4000000; cnt *= 2) {
    uint32_t us = cnt2us(cnt);
    print_cntus(cnt, us);
  }
}

void lock(void) {
  while (true);
}

void loop(void) {
  // trigger
  uint32_t prev = PINS;

  while (true) {
    uint32_t pins = PINS;
    if (prev == pins) {
      yield();
      continue;
    }
    prev = pins;
    logs[0] = pins;
    break;
  }

  // log
  int i = 1;
  uint32_t cnt = 0;
  uint32_t maxcnt = 4000000;

  while (true) {
    uint32_t pins = PINS;
    if (prev == pins) {
      if (++cnt == maxcnt) break;
      continue;
    }

    cnt = 0;
    prev = pins;
    logs[i] = pins;

    if (++i == MAX) break;
  }

  print_logs(i);
  lock();
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define BETWEEN(x, min, max) ((x) >= (min) && (x) <= (max))

#define GPIO_IOCTL_CMD  0x700
#define GPIO_BASE       881
#define GPIO_COUNT      143
#define GPIO_TO_PIN(n)  ((n) - GPIO_BASE)

static inline const char* gpio_to_dir(int dir) {
  return dir == 0 ? "in" : "out";
}

static int gpio_set_dir(int fd, int gpio, int dir) {
  if (!BETWEEN(dir, 0, 1)) {
    fprintf(stderr, "error: dir must be 0 or 1\n");
    return 1;
  }

  int data[3] = { GPIO_TO_PIN(gpio), dir, 0 };
  int ret = ioctl(fd, GPIO_IOCTL_CMD, data);
  if (ret < 0) {
    perror("error: ioctl");
    return 1;
  }

  printf("gpio#%d: set dir to '%s'\n", gpio, gpio_to_dir(dir));
  return 0;
}

static inline const char* gpio_to_val(int val) {
  return val == 0 ? "lo" : "hi";
}

static int gpio_set_val(int fd, int gpio, int val) {
  if (!BETWEEN(val, 0, 1)) {
    fprintf(stderr, "error: value must be 0 or 1\n");
    return 1;
  }

  int data[3] = { GPIO_TO_PIN(gpio), 1, val };
  int ret = ioctl(fd, GPIO_IOCTL_CMD, data);
  if (ret < 0) {
    perror("error: ioctl");
    return 1;
  }

  printf("gpio#%d: set value to '%s'\n", gpio, gpio_to_val(val));
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <gpio> <action> <value>\n", argv[0]);
    return 1;
  }

  int gpio = (int)strtol(argv[1], NULL, 0);
  if (!BETWEEN(gpio, GPIO_BASE, GPIO_BASE + GPIO_COUNT - 1)) {
    fprintf(stderr, "error: gpio must be in range %d..%d\n",
        GPIO_BASE, GPIO_BASE + GPIO_COUNT - 1);
    return 1;
  }

  char *action = argv[2];
  int val = (int)strtol(argv[3], NULL, 0);

  int fd = open("/dev/mtkgpio", O_RDWR);
  if (fd < 0) {
    perror("error: open /dev/mtkgpio");
    return 1;
  }

  int ret = 1;
  if (strcmp(action, "dir") == 0)
    ret = gpio_set_dir(fd, gpio, val);
  else if (strcmp(action, "val") == 0)
    ret = gpio_set_val(fd, gpio, val);
  else
    fprintf(stderr, "error: unknown action '%s'\n", action);

  close(fd);
  return ret;
}

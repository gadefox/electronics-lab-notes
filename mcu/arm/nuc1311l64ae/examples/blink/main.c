#include "NUC131.h"

int main(void) {
  PB->PMD = (PB->PMD & ~GPIO_PMD_PMD8_Msk) | (GPIO_PMD_OUTPUT << GPIO_PMD_PMD8_Pos);

  for (;;) {
    PB->DOUT ^= (1u << 8);
    for (volatile uint32_t i = 0; i < 500000; i++) { }
}

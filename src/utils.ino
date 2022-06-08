// Utils
void swap_r_b() {
    // BMP is BGR, swap colors and correct gamma
    for (int i=0; i<LED_COUNT; i++) {
        uint8_t temp = gamma8[leds[i].r];
        leds[i].r = gamma8[leds[i].b];
        leds[i].b = temp;
        leds[i].g = gamma8[leds[i].g];
    }
}

// BMP file helper functions
uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}



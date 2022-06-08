// LED animations
void holo_led_test() {
    for (int i=0; i<LED_COUNT; i++) {
        leds[i] = CRGB(255, 0, 0);
        if (i>0) {
            leds[i-1] = CRGB(0, 0, 0);
        }
        FastLED.show();
        delay(10);
    }
    leds[LED_COUNT-1] = CRGB(0, 0, 0);
    FastLED.show();
    for (int i=LED_COUNT-1; i>=0; i--) {
        leds[i] = CRGB(0, 255, 0);
        if (i<LED_COUNT-1) {
            leds[i+1] = CRGB(0, 0, 0);
        }
        FastLED.show();
        delay(10);
    }
    leds[0] = CRGB(0, 0, 0);
    FastLED.show();
    for (int i=0; i<LED_COUNT; i++) {
        leds[i] = CRGB(0, 0, 255);
        if (i>0) {
            leds[i-1] = CRGB(0, 0, 0);
        }
        FastLED.show();
        delay(10);
    }
    leds[LED_COUNT-1] = CRGB(0, 0, 0);
    FastLED.show();
}

void holo_stripes() {
    if (stripe_anim_params.steps && stripe_anim_params.step_delay < stripe_anim_params.time) {
        // calculate ON time
        int on_delay = (stripe_anim_params.time - (stripe_anim_params.steps * stripe_anim_params.step_delay)) / stripe_anim_params.steps;
        for (int s=0; s<stripe_anim_params.steps; s++) {
            // fill
            for (int i=0; i<LED_COUNT; i+=stripe_anim_params.width*2) {
                for (int j=0; j<stripe_anim_params.width; j++) {
                    leds[i+j] = CRGB(stripe_anim_params.r, stripe_anim_params.g, stripe_anim_params.b);
                }
            }
            FastLED.show();
            delay(on_delay);
            // clear
            fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
            FastLED.show();
            delay(stripe_anim_params.step_delay);
        }
    } else {
        // static stripes
        fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
        for (int i=0; i<LED_COUNT; i+=stripe_anim_params.width*2) {
            for (int j=0; j<stripe_anim_params.width; j++) {
                leds[i+j] = CRGB(stripe_anim_params.r, stripe_anim_params.g, stripe_anim_params.b);
            }
        }
        FastLED.show();
        delay(stripe_anim_params.time);
    }
    
    // off
    fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
    FastLED.show();
}

void holo_rainbow() {
    // colors
    CRGB rainbow_colors[6] = {
        {229, 0, 0},
        {255, 141, 0},
        {255, 238, 0},
        {0, 129, 33},
        {0, 76, 255},
        {118, 1, 136}
    };
    // calculate stripe width
    uint8_t width = LED_COUNT / 6;
    
    if (stripe_anim_params.steps && stripe_anim_params.step_delay < stripe_anim_params.time) {
        // calculate ON time
        int on_delay = (stripe_anim_params.time - (stripe_anim_params.steps * stripe_anim_params.step_delay)) / stripe_anim_params.steps;
        for (int s=0; s<stripe_anim_params.steps; s++) {
            // fill
            for (int i=0; i<6; i++) {
                fill_solid(leds+(i*width), width, rainbow_colors[i]);
            }
            FastLED.show();
            delay(on_delay);
            // clear
            fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
            FastLED.show();
            delay(stripe_anim_params.step_delay);
        }
    } else {
        // static stripes
        for (int i=0; i<6; i++) {
            fill_solid(leds+(i*width), width, rainbow_colors[i]);
        }
        FastLED.show();
        delay(stripe_anim_params.time);
    }
    
    // off
    fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
    FastLED.show();  
}

void holo_picture() {
    int      bmp_width, bmp_height;     // W+H in pixels
    uint8_t  bmp_depth;                 // Bit depth (currently must be 24)
    uint32_t bmp_image_offset;          // Start of image data in file
    uint32_t row_size;                  // Not always = bmp_width; may have padding
    
    Serial.print("Picture: ");
    Serial.println(picture_params.filename);
    
    // open image file
    File bmp_file = SPIFFS.open(picture_params.filename.c_str(), FILE_READ);

    if (!bmp_file) {
        //request->send(500, "text/plain", "Can't open image");
        last_error.code = FILE_ERROR;
        last_error.desc = "Can't open image";
        return;
    }

    // get bmp file signature
    if(read16(bmp_file) != 0x4D42) {
        //request->send(500, "text/plain", "File is not a BMP file");
        last_error.code = FORMAT_ERROR;
        last_error.desc = "File is not a BMP file";
        return;
    }
    
    // Get BMP image format
    Serial.println("Image info:");
    Serial.print(" File size: "); Serial.println(read32(bmp_file));
    
    (void)read32(bmp_file); // Read & ignore creator bytes
    
    bmp_image_offset = read32(bmp_file); // Start of image data
    
    (void) read32(bmp_file); // header size
                              
    bmp_width  = read32(bmp_file);
    bmp_height = read32(bmp_file);
    Serial.print(" Width: "); Serial.println(bmp_width);
    Serial.print(" Height: "); Serial.println(bmp_height);
    
    // test correct size
    // images are rotated 90º
    if (bmp_width != LED_COUNT) {
        //request->send(500, "text/plain", "Bad BMP height.");
        last_error.code = SIZE_ERROR;
        last_error.desc = "Bad BMP height.";
        return;
    }
    
    // test format
    if(read16(bmp_file) == 1) { // # planes -- must be '1'

        bmp_depth = read16(bmp_file); // bits per pixel
        Serial.print(" Bit Depth: "); Serial.println(bmp_depth);
        
        if((bmp_depth == 24) && (read32(bmp_file) == 0)) { // 0 = uncompressed
            // BMP rows are padded (if needed) to 4-byte boundary
            //row_size = (bmp_width * 3 + 3) & ~3;
            row_size = (bmp_width * 3) + ((bmp_width * 3) % 4);       
            
            // read columns (images are rotated 90º)
            for (int i = 0; i < bmp_height; i++) {
                // seek to row
                bmp_file.seek(bmp_image_offset + (i*row_size));
                // read line
                bmp_file.readBytes((char*)leds, LED_COUNT*3);
                // bmp is BGR, swap them
                swap_r_b();
                // send it
                FastLED.show();
                delay(picture_params.anim_delay);
            }
                
            // turn off
            fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
            FastLED.show();
        } else {
            last_error.code = FORMAT_ERROR;
            last_error.desc = "BMP wrong format (depth or compression).";
            return;   
        }
    }

    // close file and return
    bmp_file.close();   
}

void holo_low_batt() {
    last_error.code = LOW_BATT_ERROR;
    last_error.desc = "Low battery!";
    fill_solid(leds+LED_COUNT-1, 1, CRGB( 255, 0, 0));
    FastLED.show();
    delay(1000);
    fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
    FastLED.show();
    delay(1000);
}


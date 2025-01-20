/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

/*
 * SPDX-FileCopyrightText: 2019-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "detection_responder.h"
#include "tensorflow/lite/micro/micro_log.h"

#include "lvgl.h"

#include "esp_main.h"
#if DISPLAY_SUPPORT
#include "image_provider.h"
#include <bsp/esp-bsp.h>
// #include "esp_lcd.h"
// static QueueHandle_t xQueueLCDFrame = NULL;

static lv_obj_t *camera_canvas = NULL;
#define IMG_WD (240)
#define IMG_HT (320)
#endif

void RespondToDetection(float person_score, float no_person_score) {
  int person_score_int = (person_score) * 100 + 0.5;
  (void) no_person_score; // unused
#if DISPLAY_SUPPORT
#if 1 // USE LVGL
    // Create LVGL canvas for camera image
    if (!camera_canvas) {
      bsp_display_start();
      bsp_display_backlight_on(); // Set display brightness to 100%
      bsp_display_lock(0);
      camera_canvas = lv_canvas_create(lv_scr_act());
      assert(camera_canvas);
      // lv_obj_center(camera_canvas);
      bsp_display_unlock();
    }

    uint16_t *buf = (uint16_t *) image_provider_get_display_buf();

    uint16_t blue =  0xf800;
    uint16_t green = 0x07e0;
    uint16_t red =   0x001f;
    int color = green;
    if (person_score_int > 80) {
      color = red;
    }
    for (int i = 192 * 240; i < 320 * 240; i++) {
        buf[i] = color;
    }

    bsp_display_lock(0);
    #define LV_IMG_CF_TRUE_COLOR LV_COLOR_FORMAT_RGB565
    lv_canvas_set_buffer(camera_canvas, buf, IMG_WD, IMG_HT, LV_IMG_CF_TRUE_COLOR);

    // lv_style_t style;
    // lv_style_init(&style);
    // lv_style_set_text_color(&style, LV_STATE_DEFAULT, lv_color_black());
    // // lv_style_set_text_font(&style, LV_STATE_DEFAULT, &lv_font_montserrat_14);
    // lv_canvas_draw_text(camera_canvas, 0, 0, IMG_WD, &style, 
    //     "person socre " + person_score_int, LV_LABEL_ALIGN_LEFT)

    // lv_obj_t * label = lv_label_create(lv_scr_act());
    // // 设置标签的位置和大小
    // lv_obj_set_pos(label, 128, 240);
    // lv_obj_set_size(label, 192, 64);
    // // 使用 lv_label_set_text 函数设置标签的文本内容
    // lv_label_set_text(label, "person socre " + person_score_int);


    lv_obj_t * textarea = lv_textarea_create(lv_scr_act());
    // 设置文本区域的位置和大小
    lv_obj_set_pos(textarea, 0, 192);
    lv_obj_set_size(textarea, 192, 64);
    char texts[30];
    sprintf(texts, "person socre %d", person_score_int);
    // 使用 lv_textarea_set_text 函数设置文本区域的内容
    lv_textarea_set_text(textarea, texts);


    bsp_display_unlock();
#else
  if (xQueueLCDFrame == NULL) {
    xQueueLCDFrame = xQueueCreate(2, sizeof(struct lcd_frame));
    register_lcd(xQueueLCDFrame, NULL, false);
  }

  int color = 0x1f << 6; // red
  if (person_score_int < 60) { // treat score less than 60% as no person
    color = 0x3f; // green
  }
  app_lcd_color_for_detection(color);

  // display frame (freed by lcd task)
  lcd_frame_t *frame = (lcd_frame_t *) malloc(sizeof(lcd_frame_t));
  frame->width = 96 * 2;
  frame->height = 96 * 2;
  frame->buf = image_provider_get_display_buf();
  xQueueSend(xQueueLCDFrame, &frame, portMAX_DELAY);
#endif
#endif
  MicroPrintf("person score:%d%%, no person score %d%%",
              person_score_int, 100 - person_score_int);
}

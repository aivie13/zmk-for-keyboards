/*
 * Profile-aware operating system logo for the central OLED.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <lvgl.h>

#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

#define LOGO_WIDTH 26
#define LOGO_HEIGHT 50
#define ART_SIZE 24
#define ART_X ((LOGO_WIDTH - ART_SIZE) / 2)
#define ART_Y ((LOGO_HEIGHT - ART_SIZE) / 2)

static lv_obj_t *logo_canvas;
static lv_color_t logo_buffer[LOGO_WIDTH * LOGO_HEIGHT];

static lv_color_t background_color(void) {
    return IS_ENABLED(CONFIG_NICE_OLED_WIDGET_INVERTED) ? lv_color_black() : lv_color_white();
}

static lv_color_t foreground_color(void) {
    return IS_ENABLED(CONFIG_NICE_OLED_WIDGET_INVERTED) ? lv_color_white() : lv_color_black();
}

static void set_clockwise_pixel(int x, int y, lv_color_t color) {
    lv_canvas_set_px_color(logo_canvas, ART_X + ART_SIZE - 1 - y, ART_Y + x, color);
}

/*
 * Rasterized at 24x24 from the Simple Icons Apple mark:
 * https://github.com/simple-icons/simple-icons/blob/develop/icons/apple.svg
 * Simple Icons is released under CC0-1.0.
 */
static const uint32_t apple_logo_rows[ART_SIZE] = {
    0x000180, 0x000380, 0x000700, 0x000f00, 0x000e00, 0x000000,
    0x03e3e0, 0x0ffff8, 0x0ffff8, 0x1ffff0, 0x1fffe0, 0x3fffe0,
    0x3fffe0, 0x3fffe0, 0x3fffe0, 0x3fffe0, 0x1ffff0, 0x1ffff8,
    0x1ffff8, 0x0ffff8, 0x0ffff0, 0x07fff0, 0x03ffe0, 0x01c1c0,
};

static void draw_apple_logo(void) {
    const lv_color_t foreground = foreground_color();

    for (int y = 0; y < ART_SIZE; y++) {
        for (int x = 0; x < ART_SIZE; x++) {
            if ((apple_logo_rows[y] & (1U << (ART_SIZE - 1 - x))) != 0) {
                set_clockwise_pixel(x, y, foreground);
            }
        }
    }
}

static void draw_windows_logo(void) {
    lv_draw_rect_dsc_t pane;
    lv_draw_rect_dsc_init(&pane);
    pane.bg_color = foreground_color();
    pane.bg_opa = LV_OPA_COVER;
    pane.border_width = 0;

    lv_canvas_draw_rect(logo_canvas, ART_X + 2, ART_Y + 2, 9, 9, &pane);
    lv_canvas_draw_rect(logo_canvas, ART_X + 13, ART_Y + 2, 9, 9, &pane);
    lv_canvas_draw_rect(logo_canvas, ART_X + 2, ART_Y + 13, 9, 9, &pane);
    lv_canvas_draw_rect(logo_canvas, ART_X + 13, ART_Y + 13, 9, 9, &pane);
}

static void update_logo(uint8_t profile_index) {
    if (logo_canvas == NULL) {
        lv_obj_t *screen_widget = lv_obj_get_child(lv_scr_act(), 0);
        lv_obj_t *status_canvas = lv_obj_get_child(screen_widget, 0);

        logo_canvas = lv_canvas_create(status_canvas);
        lv_canvas_set_buffer(logo_canvas, logo_buffer, LOGO_WIDTH, LOGO_HEIGHT,
                             LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(logo_canvas, LV_ALIGN_TOP_LEFT, CONFIG_NICE_OLED_WIDGET_BONGO_CAT_CUSTOM_X,
                     CONFIG_NICE_OLED_WIDGET_BONGO_CAT_CUSTOM_Y);
        lv_obj_clear_flag(logo_canvas, LV_OBJ_FLAG_SCROLLABLE);
    }

    if (profile_index > 1) {
        lv_obj_add_flag(logo_canvas, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(logo_canvas, LV_OBJ_FLAG_HIDDEN);
    lv_canvas_fill_bg(logo_canvas, background_color(), LV_OPA_COVER);

    if (profile_index == 0) {
        draw_apple_logo();
    } else {
        draw_windows_logo();
    }
}

struct profile_logo_state {
    uint8_t index;
};

static struct profile_logo_state profile_logo_get_state(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *event = as_zmk_ble_active_profile_changed(eh);
    return (struct profile_logo_state){
        .index = event == NULL ? zmk_ble_active_profile_index() : event->index,
    };
}

static void profile_logo_update_cb(struct profile_logo_state state) { update_logo(state.index); }

ZMK_DISPLAY_WIDGET_LISTENER(profile_logo, struct profile_logo_state, profile_logo_update_cb,
                            profile_logo_get_state)
ZMK_SUBSCRIPTION(profile_logo, zmk_ble_active_profile_changed);

static void profile_logo_init_work_cb(struct k_work *work) { profile_logo_init(); }

K_WORK_DEFINE(profile_logo_init_work, profile_logo_init_work_cb);

static void profile_logo_start_work_cb(struct k_work *work) {
    if (!zmk_display_is_initialized()) {
        k_work_reschedule(k_work_delayable_from_work(work), K_MSEC(100));
        return;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &profile_logo_init_work);
}

K_WORK_DELAYABLE_DEFINE(profile_logo_start_work, profile_logo_start_work_cb);

static int profile_logo_start(void) {
    k_work_schedule(&profile_logo_start_work, K_MSEC(500));
    return 0;
}

SYS_INIT(profile_logo_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

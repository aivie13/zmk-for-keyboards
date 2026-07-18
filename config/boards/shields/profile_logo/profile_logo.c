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

#define LOGO_SIZE 28

static lv_obj_t *logo_canvas;
static lv_color_t logo_buffer[LOGO_SIZE * LOGO_SIZE];

static lv_color_t background_color(void) {
    return IS_ENABLED(CONFIG_NICE_OLED_WIDGET_INVERTED) ? lv_color_black() : lv_color_white();
}

static lv_color_t foreground_color(void) {
    return IS_ENABLED(CONFIG_NICE_OLED_WIDGET_INVERTED) ? lv_color_white() : lv_color_black();
}

static void draw_apple_logo(void) {
    const lv_color_t foreground = foreground_color();

    for (int y = 0; y < LOGO_SIZE; y++) {
        for (int x = 0; x < LOGO_SIZE; x++) {
            const int left_x = x - 9;
            const int right_x = x - 17;
            const int lower_x = x - 13;
            const int upper_y = y - 12;
            const int lower_y = y - 16;
            const int bite_x = x - 22;
            const int bite_y = y - 10;

            bool body = (left_x * left_x * 36 + upper_y * upper_y * 49 <= 1764) ||
                        (right_x * right_x * 36 + upper_y * upper_y * 49 <= 1764) ||
                        (lower_x * lower_x * 25 + lower_y * lower_y * 49 <= 2025);
            const bool bite = bite_x * bite_x + bite_y * bite_y <= 16;

            if (body && !bite && y >= 7 && y <= 23) {
                lv_canvas_set_px_color(logo_canvas, x, y, foreground);
            }
        }
    }

    /* Leaf */
    lv_canvas_set_px_color(logo_canvas, 15, 3, foreground);
    lv_canvas_set_px_color(logo_canvas, 16, 3, foreground);
    lv_canvas_set_px_color(logo_canvas, 14, 4, foreground);
    lv_canvas_set_px_color(logo_canvas, 15, 4, foreground);
    lv_canvas_set_px_color(logo_canvas, 16, 4, foreground);
    lv_canvas_set_px_color(logo_canvas, 13, 5, foreground);
    lv_canvas_set_px_color(logo_canvas, 14, 5, foreground);
    lv_canvas_set_px_color(logo_canvas, 15, 5, foreground);
}

static void draw_windows_logo(void) {
    lv_draw_rect_dsc_t pane;
    lv_draw_rect_dsc_init(&pane);
    pane.bg_color = foreground_color();
    pane.bg_opa = LV_OPA_COVER;
    pane.border_width = 0;

    lv_canvas_draw_rect(logo_canvas, 4, 4, 9, 9, &pane);
    lv_canvas_draw_rect(logo_canvas, 15, 4, 9, 9, &pane);
    lv_canvas_draw_rect(logo_canvas, 4, 15, 9, 9, &pane);
    lv_canvas_draw_rect(logo_canvas, 15, 15, 9, 9, &pane);
}

static void update_logo(uint8_t profile_index) {
    if (logo_canvas == NULL) {
        logo_canvas = lv_canvas_create(lv_scr_act());
        lv_canvas_set_buffer(logo_canvas, logo_buffer, LOGO_SIZE, LOGO_SIZE, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(logo_canvas, LV_ALIGN_RIGHT_MID, -2, 0);
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

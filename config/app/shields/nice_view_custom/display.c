#include <lvgl.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/battery_status.h>

LV_FONT_DECLARE(zmk_font_mono_10);

static struct zmk_widget_layer_status layer_status;
static struct zmk_widget_battery_status battery_status;

void zmk_display_app_screen_init(lv_obj_t *root) {
    // Title
    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Corne");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Layer
    zmk_widget_layer_status_init(&layer_status, root);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status), LV_ALIGN_CENTER, 0, 0);

    // Battery
    zmk_widget_battery_status_init(&battery_status, root);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status), LV_ALIGN_BOTTOM_MID, 0, -2);
}


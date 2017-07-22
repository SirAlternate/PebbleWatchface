#include <pebble.h>

static Window *s_main_window;

static Layer *s_canvas_layer;
static Layer *s_accent_layer;
static Layer *s_battery_layer;
static TextLayer *s_date_layer;
static TextLayer *s_temp_layer;
static TextLayer *s_location_layer;
static TextLayer *s_time_layer;
static TextLayer *s_summary_layer;
static TextLayer *s_steps_layer;

static int s_battery_level;
static int s_temperature;
static char* s_location;
static char* s_summary;


char *upcase(char *str) {
  for (int i = 0; str[i] != 0; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] -= 0x20;
    }
  }
  return str;
}

static void update_steps() {
  #if defined(PBL_HEALTH)
  HealthMetric metric = HealthMetricStepCount;
  
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, 
                                                                    start, end);
  
  if(mask & HealthServiceAccessibilityMaskAvailable) {
    static char buffer[17];
    snprintf(buffer, sizeof(buffer), "%d STEPS", (int)health_service_sum_today(metric));
    text_layer_set_text(s_steps_layer,  buffer);
  } else {
    // No data recorded yet today
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
  }
  #endif
}

static void update_weather() {
  static char temp_buffer[9];  
  snprintf(temp_buffer, sizeof(temp_buffer), "%d\u00B0", (int)s_temperature);

  text_layer_set_text(s_temp_layer,  temp_buffer);
  text_layer_set_text(s_summary_layer,  s_summary);
  text_layer_set_text(s_location_layer,  upcase(s_location));
}

static void update_time_and_date() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char time_buffer[8];
  static char date_buffer[20];
  
  strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  strftime(date_buffer, sizeof(date_buffer), "%A %b %e", tick_time);
  
  text_layer_set_text(s_time_layer, time_buffer);
  text_layer_set_text(s_date_layer, upcase(date_buffer));
}

static void update_battery(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void draw_steps_layer() {
  text_layer_set_text_color(s_steps_layer, GColorMalachite);
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);
  text_layer_set_font(s_steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_steps_layer, "? STEPS");
}

static void draw_summary_layer() {
  text_layer_set_text_color(s_summary_layer, GColorWhite);
  text_layer_set_background_color(s_summary_layer, GColorClear);
  text_layer_set_text_alignment(s_summary_layer, GTextAlignmentCenter);
  text_layer_set_font(s_summary_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_summary_layer, "Reading weather..");
}

static void draw_time_layer() {
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text(s_time_layer, "00:00");
}

static void draw_location_layer() {
  text_layer_set_text_color(s_location_layer, GColorWhite);
  text_layer_set_background_color(s_location_layer, GColorClear);
  text_layer_set_text_alignment(s_location_layer, GTextAlignmentRight);
  text_layer_set_font(s_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_location_layer, "");
}

static void draw_temperature_layer() {
  text_layer_set_text_color(s_temp_layer, GColorWhite);
  text_layer_set_background_color(s_temp_layer, GColorClear);
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentLeft);
  text_layer_set_font(s_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_temp_layer, "");
}

static void draw_date_layer() {
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_date_layer, "DATE GOES HERE");
}

static void draw_battery_layer(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int width = (s_battery_level*bounds.size.w)/100;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void draw_accent_layer(Layer *layer, GContext *ctx) {
  GRect topbar_bounds = GRect(0, 0, layer_get_bounds(layer).size.w, 30);
  GRect summary_bounds = GRect(0, 109, layer_get_bounds(layer).size.w, 37);
   
  graphics_context_set_fill_color(ctx, GColorBlueMoon);
  graphics_fill_rect(ctx, topbar_bounds, 0, GCornersAll);
  
  graphics_context_set_fill_color(ctx, GColorPictonBlue);
  graphics_fill_rect(ctx, GRect(summary_bounds.origin.x, summary_bounds.origin.y, summary_bounds.size.w, 1), 0, GCornersAll);
  graphics_fill_rect(ctx, GRect(summary_bounds.origin.x, summary_bounds.origin.y+summary_bounds.size.h, summary_bounds.size.w, 1), 0, GCornersAll);
}

static void draw_canvas(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, bounds);
  graphics_fill_rect(ctx, bounds, 0, GCornersAll);
}

static void main_window_load(Window *window) {  
  // Get information about the Window
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  
  // Create canvas layer and add to window
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, draw_canvas);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
  
  // Create accent layer and add to window
  s_accent_layer = layer_create(bounds);
  layer_set_update_proc(s_accent_layer, draw_accent_layer);
  layer_add_child(s_canvas_layer, s_accent_layer);
  
  // Create battery level layer and add to window
  s_battery_layer = layer_create(GRect(0, 0, bounds.size.w, 2));
  layer_set_update_proc(s_battery_layer, draw_battery_layer);
  layer_add_child(s_canvas_layer, s_battery_layer);
  
  // Create date layer and add to window
  s_date_layer = text_layer_create(GRect(0, 4, bounds.size.w, 19));
  draw_date_layer();
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_date_layer));
  
  // Create temperature layer and add to canvas
  s_temp_layer = text_layer_create(GRect(4, 30, 30, 26));
  draw_temperature_layer();
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_temp_layer));
  
  // Create location layer and add to canvas
  s_location_layer = text_layer_create(GRect(42, 30, bounds.size.w-46, 26));
  draw_location_layer();
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_location_layer));
  
  // Create time layer and add to window
  s_time_layer = text_layer_create(GRect(0, 48, bounds.size.w, 50)); //66 centered
  draw_time_layer();
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_time_layer));
  
  // Create summary layer and add to window
  s_summary_layer = text_layer_create(GRect(0, 111, bounds.size.w, 36));
  draw_summary_layer();
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_summary_layer));
  
  // Create steps layer and add to window
  s_steps_layer = text_layer_create(GRect(0, bounds.size.h - 22, bounds.size.w, 18));
  draw_steps_layer();
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_steps_layer));
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  layer_destroy(s_accent_layer);
  layer_destroy(s_battery_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_temp_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_summary_layer);
  text_layer_destroy(s_steps_layer);
}

static void health_event_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) {
    update_steps();
  }
}

// static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
//   APP_LOG(APP_LOG_LEVEL_INFO, "Tap!");
// }

static void tick_event_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time_and_date();
  
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
}

static void app_message_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void app_message_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void app_message_sent(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void app_message_recieved(DictionaryIterator *iterator, void *context) {
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *location_tuple = dict_find(iterator, MESSAGE_KEY_LOCATION);
  Tuple *summary_tuple = dict_find(iterator, MESSAGE_KEY_SUMMARY);
  
  s_temperature = (int)temp_tuple->value->int32;
  s_location = location_tuple->value->cstring;
  s_summary = summary_tuple->value->cstring;
  
  update_weather();
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  const int inbox_size = 128;
  const int outbox_size = 128;
  
  battery_state_service_subscribe(update_battery);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_event_handler);
  health_service_events_subscribe(health_event_handler, NULL);
//   accel_tap_service_subscribe(accel_tap_handler);
  
  app_message_register_inbox_received(app_message_recieved);
  app_message_open(inbox_size, outbox_size);
  app_message_register_inbox_dropped(app_message_dropped);
  app_message_register_outbox_failed(app_message_failed);
  app_message_register_outbox_sent(app_message_sent);
  
  update_battery(battery_state_service_peek());
  update_time_and_date();
  update_steps();
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
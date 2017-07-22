#include <pebble.h>

static Window *s_main_window;

static Layer *s_canvas_layer;
static Layer *s_topbar_layer;
static Layer *s_battery_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static TextLayer *s_steps_layer;

static int s_battery_level;


/* The Itoa code is in the puiblic domain */
char* Itoa(int value, char* str, int radix) {
    static char dig[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz";
    int n = 0, neg = 0;
    unsigned int v;
    char* p, *q;
    char c;
    if (radix == 10 && value < 0) {
        value = -value;
        neg = 1;
    }
    v = value;
    do {
        str[n++] = dig[v%radix];
        v /= radix;
    } while (v);
    if (neg)
        str[n++] = '-';
    str[n] = '\0';
    for (p = str, q = p + n/2; p != q; ++p, --q)
        c = *p, *p = *q, *q = c;
    return str;
}

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
    static char buffer[] = "00000000000";
    snprintf(buffer, sizeof(buffer), "%d", (int)health_service_sum_today(metric));
    strcat(buffer, " STEPS");
    text_layer_set_text(s_steps_layer,  buffer);
  } else {
    // No data recorded yet today
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
  }
  #endif
}

static void update_battery(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
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

static void health_event_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) {
    update_steps();
  }
}

static void draw_battery_layer(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int width = (s_battery_level*bounds.size.w)/100;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void draw_canvas(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, bounds);
  graphics_fill_rect(ctx, bounds, 0, GCornersAll);
  
  // Create topbar layer and add to canvas
  s_topbar_layer = layer_create(GRect(0, 0, bounds.size.w, 28));
  graphics_context_set_fill_color(ctx, GColorBlueMoon);
  graphics_fill_rect(ctx, layer_get_bounds(s_topbar_layer), 0, GCornersAll);
  
  // Create battery meter layer and add to canvas
  s_battery_layer = layer_create(GRect(0, 0, bounds.size.w, 2));
  layer_set_update_proc(s_battery_layer, draw_battery_layer);
  layer_add_child(s_canvas_layer, s_battery_layer);

  // Create date layer and add to canvas
  s_date_layer = text_layer_create(GRect(0, 2, bounds.size.w, 18));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_date_layer, "Hello World!");
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_date_layer));
  
  // Create time layer and add to canvas
  s_time_layer = text_layer_create(GRect(0, 52, bounds.size.w, 50));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text(s_time_layer, "00:00");
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_time_layer));
  
  // Create steps layer and add to canvas
  s_steps_layer = text_layer_create(GRect(0, bounds.size.h - 26, bounds.size.w, 18));
  text_layer_set_text_color(s_steps_layer, GColorMalachite);
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);
  text_layer_set_font(s_steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_steps_layer, "Hello World!");
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_steps_layer));
  
  // Update all values initially
  update_battery(battery_state_service_peek());
  update_time_and_date();
  update_steps();
}


static void main_window_load(Window *window) {  
  // Get information about the Window
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  
  // Create canvas layer and add to window
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, draw_canvas);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  layer_destroy(s_battery_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
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
  
  battery_state_service_subscribe(update_battery);
  health_service_events_subscribe(health_event_handler, NULL);
  // Register with TickTimerService
//   tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
//   update_time();
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
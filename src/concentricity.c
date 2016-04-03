// Copyright [2015] Pebble Technology

#include <pebble.h>
#include "./ui.h"

#define WAKEUP_REASON 0

static WakeupId s_wakeup_id = 0;

// Define struct to store colors for each time unit
typedef struct Palette {
  GColor seconds;
  GColor minutes;
  GColor hours;
} Palette;

static Window *s_window;
static Window *w_window;
static Layer *s_layer;
static Palette *s_palette;

static uint8_t s_hour;
static uint8_t s_minute;
static uint8_t s_second;
struct tm *tick_time;
time_t temp ;
static AppTimer *s_timer;
static TextLayer *s_time_layer;

// Set the color for drawing routines
static void set_color(GContext *ctx, GColor color) {
  graphics_context_set_fill_color(ctx, color);
}
static void w_window_load(Window *window){
 // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "Time's up!");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}
static void w_window_unload(Window *window){
}
static void timer_callback(void *data){
  window_stack_pop(w_window);
}
static void time_up(){
    w_window=window_create();
    window_set_window_handlers(w_window, (WindowHandlers) {
      .load = w_window_load,
      .unload = w_window_unload
    });
    window_stack_push(w_window,true);  
    app_timer_register(2000, timer_callback, NULL);
    vibes_short_pulse();
}

static void wakeup_handler(WakeupId wakeup_id, int32_t reason) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Woke up due to: %lu", reason);
  time_up();
}
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // A tap event occured
  //wakeup
  if(!wakeup_query(s_wakeup_id, NULL)) {
  temp= time(NULL);
   tick_time = localtime(&temp);

  	time_t wakeup_time = temp+5;//(tick_time->tm_hour * 60 * 60) + 5; //current hour + 59 minutes, 58 seconds
		tick_time = localtime(&wakeup_time);		
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Next wake: %d:%d:%d", tick_time->tm_hour, tick_time->tm_min, tick_time->tm_sec);
		
		// Schedule wakeup event
    s_wakeup_id = wakeup_schedule(wakeup_time, WAKEUP_REASON, true);	
  // Subscribe to Wakeup API
  wakeup_service_subscribe(wakeup_handler);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "WakeupId: %lu", s_wakeup_id);
  }
}
// Update the watchface display
static void update_display(Layer *layer, GContext *ctx) {
  set_color(ctx, s_palette->seconds);
  draw_seconds(ctx, s_second, layer);

  set_color(ctx, s_palette->minutes);
  draw_minutes(ctx, s_minute, layer);

  set_color(ctx, s_palette->hours);
  draw_hours(ctx, s_hour % 12, layer);
}

// Update the current time values for the watchface
static void update_time(struct tm *tick_time) {
  s_hour = tick_time->tm_hour;
  s_minute = tick_time->tm_min;
  s_second = tick_time->tm_sec;
  layer_mark_dirty(s_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}
static void window_load(Window *window) {
  // Subscribe to tap events
  accel_tap_service_subscribe(accel_tap_handler);
  s_palette = malloc(sizeof(Palette));
  *s_palette = (Palette) {
      COLOR_FALLBACK(GColorRichBrilliantLavender, GColorWhite),
      COLOR_FALLBACK(GColorVividViolet, GColorWhite),
      COLOR_FALLBACK(GColorBlueMoon, GColorWhite)
  };

  s_layer = layer_create(layer_get_bounds(window_get_root_layer(s_window)));
  layer_add_child(window_get_root_layer(s_window), s_layer);
  layer_set_update_proc(s_layer, update_display);
}

static void window_unload(Window *window) {
  free(s_palette);
  layer_destroy(s_layer);
}

static void init(void) {
  
  s_window = window_create();

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_set_background_color(s_window, GColorBlack);
  window_stack_push(s_window, true);

  time_t start = time(NULL);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  update_time(localtime(&start));  // NOLINT(runtime/threadsafe_fn)
  
  // Check to see if we were launched by a wakeup event
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    time_up();
  }
}
static void deinit(void) {
  tick_timer_service_unsubscribe();
  // Unsubscribe from tap events
  accel_tap_service_unsubscribe();
  window_destroy(s_window);
  window_destroy(w_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
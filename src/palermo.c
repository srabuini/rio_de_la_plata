#include <pebble.h>

static Window *window;
static TextLayer *time_layer;
static TextLayer *wind_layer;
static TextLayer *wind_direction_layer;
static TextLayer *temp_and_level_layer;
static TextLayer *last_update_layer;
static Layer *rounded_layer;

static AppSync sync;
static uint8_t sync_buffer[128];

enum WeatherKey {
  WIND_DIRECTION_KEY = 0x0,
  WIND_SPEED_KEY = 0x1,
  TEMP_AND_LEVEL_KEY = 0x2,
  LAST_UPDATE_KEY = 0x3
};

static void sync_error_callback(DictionaryResult dict_error,
                                AppMessageResult app_message_error,
                                void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple *new_tuple,
                                        const Tuple *old_tuple, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "received: %lu", key);
  switch (key) {
    case WIND_DIRECTION_KEY:
      text_layer_set_text(wind_direction_layer, new_tuple->value->cstring);
      break;

    case WIND_SPEED_KEY:
      text_layer_set_text(wind_layer, new_tuple->value->cstring);
      break;

    case TEMP_AND_LEVEL_KEY:
      text_layer_set_text(temp_and_level_layer, new_tuple->value->cstring);
      break;
    
    case LAST_UPDATE_KEY:
      text_layer_set_text(last_update_layer, new_tuple->value->cstring);
      break;
  }
}

static void handle_timechanges(struct tm *tick_time, TimeUnits units_changed) {
  static char time_buffer[10];

  strftime(time_buffer, sizeof(time_buffer), "%k:%M", tick_time);
  text_layer_set_text(time_layer, time_buffer);
}

static void send_cmd(void) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void rounded_layer_update_callback(Layer *me, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 112, 144, 56), 6, GCornersAll);
}

static void window_load(Window *window) {
  Layer *root_layer = window_get_root_layer(window);

  // Create the Time text_layer
  time_layer = text_layer_create(GRect(0, 0, 144, 56));
  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_font(time_layer,
                      fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  layer_add_child(root_layer, text_layer_get_layer(time_layer));

  // Create the Wind Direction text_layer
  wind_direction_layer = text_layer_create(GRect(0, 56, 144, 28));
  text_layer_set_background_color(wind_direction_layer, GColorBlack);
  text_layer_set_text_color(wind_direction_layer, GColorWhite);
  text_layer_set_text_alignment(wind_direction_layer, GTextAlignmentCenter);
  text_layer_set_font(wind_direction_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD ));
  layer_add_child(root_layer, text_layer_get_layer(wind_direction_layer));

  // Create the last_update text_layer
  last_update_layer = text_layer_create(GRect(0, 88, 144, 16));
  text_layer_set_background_color(last_update_layer, GColorBlack);
  text_layer_set_text_color(last_update_layer, GColorWhite);
  text_layer_set_text_alignment(last_update_layer, GTextAlignmentCenter);
  text_layer_set_text(last_update_layer, "_-_ -:-");
  text_layer_set_font(last_update_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14 ));
  layer_add_child(root_layer, text_layer_get_layer(last_update_layer));

  Layer *window_layer = root_layer;
  GRect frame = layer_get_frame(window_layer);

  rounded_layer = layer_create(frame);
  layer_set_update_proc(rounded_layer, rounded_layer_update_callback);
  layer_add_child(window_layer, rounded_layer);

  // Create the Wind text_layer
  wind_layer = text_layer_create(GRect(8, 112, 128, 24));
  text_layer_set_text_alignment(wind_layer, GTextAlignmentCenter);
  text_layer_set_font(wind_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD ));
  layer_add_child(root_layer, text_layer_get_layer(wind_layer));

  // Create the temp_and_level text_layer
  temp_and_level_layer = text_layer_create(GRect(8, 136, 128, 24));
  text_layer_set_text_alignment(temp_and_level_layer, GTextAlignmentCenter);
  text_layer_set_font(temp_and_level_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(root_layer, text_layer_get_layer(temp_and_level_layer));

  // Subscribe to tick_timer_service
  tick_timer_service_subscribe(MINUTE_UNIT, handle_timechanges);

  // Initialize app_message
  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);

  Tuplet initial_values[] = {
    TupletCString(WIND_DIRECTION_KEY, "-\u00B0 ---"),
    TupletCString(WIND_SPEED_KEY, "-kt -km/h"),
    TupletCString(TEMP_AND_LEVEL_KEY, "-\u00B0C | -m"),
    TupletCString(LAST_UPDATE_KEY, "_-_ -:-")
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values),
    sync_tuple_changed_callback, sync_error_callback, NULL);

  send_cmd();

  window_set_background_color(window, GColorBlack);

  // Push the window
  window_stack_push(window, true);
}

static void window_unload(Window *window) {
  // Deinit sync
  app_sync_deinit(&sync);

  layer_destroy(rounded_layer);

  // Destroy the text layer
  text_layer_destroy(time_layer);

  // Destroy the wind_layer
  text_layer_destroy(wind_layer);

  // Destroy the temp_and_level_layer
  text_layer_destroy(temp_and_level_layer);

  // Destroy the wind_direction_layer
  text_layer_destroy(wind_direction_layer);
  
  text_layer_destroy(last_update_layer);
}

static void init(void) {
  // Create a window
  window = window_create();
  window_set_background_color(window, GColorBlack);

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const bool animated = false;
  window_stack_push(window, animated);
}

static void deinit(void) {
  // Destroy the window
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
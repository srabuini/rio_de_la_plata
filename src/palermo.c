#include <pebble.h>

static Window *window;
static TextLayer *time_layer;
static TextLayer *wind_layer;
static TextLayer *wind_direction_layer;
static TextLayer *temp_and_level_layer;

static InverterLayer *inverter_time;
static InverterLayer *inverter_wind_direction;

static AppSync sync;
static uint8_t sync_buffer[64];

enum WeatherKey {
  WIND_DIRECTION_KEY = 0x0, // TUPLE_CSTRING
  WIND_SPEED_KEY = 0x1,     // TUPLE_CSTRING
  TEMP_AND_LEVEL_KEY = 0x2, // TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
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
  }
}

static void handle_timechanges(struct tm *tick_time, TimeUnits units_changed) {
  static char time_buffer[10];

  strftime(time_buffer, sizeof(time_buffer), "%H:%M", tick_time);
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

static void window_load(Window *window) {
  // Create the Time text_layer
  time_layer = text_layer_create(GRect(0, 0, 144, 56));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

  inverter_time = inverter_layer_create(GRect(0, 0, 144, 56));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_time));

  // Create the Wind Direction text_layer
  wind_direction_layer = text_layer_create(GRect(0, 56, 144, 56));
  text_layer_set_text_alignment(wind_direction_layer, GTextAlignmentCenter);
  text_layer_set_font(wind_direction_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD ));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(wind_direction_layer));

  inverter_wind_direction = inverter_layer_create(GRect(0, 56, 144, 56));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_wind_direction));

  // Create the Wind text_layer
  wind_layer = text_layer_create(GRect(0, 112, 144, 28));
  text_layer_set_text_alignment(wind_layer, GTextAlignmentCenter);
  text_layer_set_font(wind_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD ));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(wind_layer));

  // Create the temp_and_level text_layer
  temp_and_level_layer = text_layer_create(GRect(0, 140, 144, 28));
  text_layer_set_text_alignment(temp_and_level_layer, GTextAlignmentCenter);
  text_layer_set_font(temp_and_level_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(temp_and_level_layer));

  // Subscribe to tick_timer_service
  tick_timer_service_subscribe(MINUTE_UNIT, handle_timechanges);

  // Initialize app_message
  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);

  Tuplet initial_values[] = {
    TupletCString(WIND_DIRECTION_KEY, "0\u00B0 N"),
    TupletCString(WIND_SPEED_KEY, "0kt | 0km/h"),
    TupletCString(TEMP_AND_LEVEL_KEY, "0\u00B0C | 0m")
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
    sync_tuple_changed_callback, sync_error_callback, NULL);

  send_cmd();

  // Push the window
  window_stack_push(window, true);
}

static void window_unload(Window *window) {
  // Deinit sync
  app_sync_deinit(&sync);

  // Destroy the text layer
  text_layer_destroy(time_layer);
  inverter_layer_destroy(inverter_time);

  // Destroy the wind_layer
  text_layer_destroy(wind_layer);

  // Destroy the temp_and_level_layer
  text_layer_destroy(temp_and_level_layer);

  // Destroy the wind_direction_layer
  text_layer_destroy(wind_direction_layer);
  inverter_layer_destroy(inverter_wind_direction);
}

static void init(void) {
  // Create a window
  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const bool animated = true;
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

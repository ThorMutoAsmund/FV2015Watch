#include <pebble.h>

#define KEY_POLL_RED_BLOCK 0
#define KEY_POLL_BLUE_BLOCK 1
  
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_timeLeft_layer;
static TextLayer *s_block_layer;
static char redBlock[8];
static char blueBlock[8];

static TextLayer *create_default_text_layer(Window *window, GRect rect, GFont font, GTextAlignment alignment) {
  TextLayer *layer = text_layer_create(rect);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(layer));
  return layer;
}

static void main_window_load(Window *window) {
  // Create time TextLayer
  s_time_layer = create_default_text_layer(window, GRect(0, 5, 144, 50), fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD), GTextAlignmentCenter);
  s_date_layer = create_default_text_layer(window, GRect(0, 45, 144, 30), fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GTextAlignmentCenter);
  s_timeLeft_layer = create_default_text_layer(window, GRect(0, 78, 144, 70), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GTextAlignmentCenter);
  s_block_layer = create_default_text_layer(window, GRect(0, 130, 144, 30), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GTextAlignmentCenter);
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
}

// http://www.b.dk/upload/webred/bmsandbox/opinion_poll/midi/midi.js

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Time buffer
  static char lastBuffer[6];
  static char buffer[6];

  // Write the current hours and minutes into the buffer
  if (clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof(buffer), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof(buffer), "%I:%M", tick_time);
  }
  
  if (!strncmp(lastBuffer, buffer, sizeof(buffer))) {
    return;
  }
  
  // Display this time on the TextLayer
  strncpy(lastBuffer, buffer, sizeof(lastBuffer));
  text_layer_set_text(s_time_layer, buffer);
  
  // Time left buffer
  static char lastTimeLeftBuffer[64];
  static char timeLeftBuffer[64];

  int finished = 0;
  if (tick_time->tm_year != 115 || tick_time->tm_mon != 5 || tick_time->tm_mday > 18)
  {
    finished = 1;
  }
  else {
    int minsTillElectionStart = 60 *((18 - tick_time->tm_mday)*24 + 9 - tick_time->tm_hour) - tick_time->tm_min;
    int minsTillElectionEnd = minsTillElectionStart + 11*60;
    
    if (minsTillElectionEnd < 0) {
      finished = 1;
    }
    else if (minsTillElectionStart >= 24*60) {
      int days = 18 - tick_time->tm_mday;
      if (days > 1)
        snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "%d dage til valget starter", days);
      else
        snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "1 dag til valget starter");
    }
    else if (minsTillElectionStart >= 60) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "%d timer til valgstederne åbner", minsTillElectionStart / 60);
    }
    else if (minsTillElectionStart >= 2) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "%d minutter til valgstederne åbner", minsTillElectionStart);
    }
    else if (minsTillElectionStart >= 1) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "1 minut til valgstedere åbner");
    }
    else if (minsTillElectionStart >= 0) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "Mindre end 1 minut til valgstedere åbner");
    }
    else if (minsTillElectionEnd >= 60) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "%d timer til valgstederne lukker", minsTillElectionEnd / 60);
    }
    else if (minsTillElectionEnd >= 2) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "%d minutter til valgstederne lukker", minsTillElectionEnd);
    }
    else if (minsTillElectionEnd >= 1) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "1 minut til valgstedere lukker");
    }
    else if (minsTillElectionEnd >= 0) {
      snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "Mindre end 1 minut til valgstedere lukker");
    }
  }
  
  if (finished) {
    snprintf(timeLeftBuffer, sizeof(timeLeftBuffer), "Valgstederne er lukket");
  }

  if (strncmp(lastTimeLeftBuffer, timeLeftBuffer, sizeof(timeLeftBuffer))) {
    // Display this on the TextLayer
    strncpy(lastTimeLeftBuffer, timeLeftBuffer, sizeof(lastTimeLeftBuffer));
    text_layer_set_text(s_timeLeft_layer, timeLeftBuffer);
  }


  // Date buffer
  static char * const monthArray[] = { "januar", "februar", "marts", "april", "maj", "juni", "juli", "august", "september", "oktober", "november", "december" };
  static char lastDateBuffer[24];
  static char dateBuffer[24];

  snprintf(dateBuffer, sizeof(dateBuffer), "%d. %s %d", tick_time->tm_mday, monthArray[tick_time->tm_mon], tick_time->tm_year + 1900);

  if (!strncmp(lastDateBuffer, dateBuffer, sizeof(dateBuffer))) {
    return;
  }

  // Display this date on the TextLayer
  strncpy(lastDateBuffer, dateBuffer, sizeof(lastDateBuffer));
  text_layer_set_text(s_date_layer, dateBuffer);
}

static void update_polls() {
     APP_LOG(APP_LOG_LEVEL_INFO, "Updating text! %s", redBlock);
  static char pollBuffer[32];
  snprintf(pollBuffer, sizeof(pollBuffer), "Rød %s  Blå %s", redBlock, blueBlock);
  text_layer_set_text(s_block_layer, pollBuffer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  
  // For all items
  bool updatePolls = false;
  while (t != NULL) {
     APP_LOG(APP_LOG_LEVEL_INFO, "Message received! %ld", t->key);
    // Which key was received?
    switch(t->key) {
      case KEY_POLL_RED_BLOCK:
        strncpy(redBlock, t->value->cstring, sizeof(redBlock));
        updatePolls = true;
      break;
      case KEY_POLL_BLUE_BLOCK:
        strncpy(blueBlock, t->value->cstring, sizeof(redBlock));
        updatePolls = true;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    
    if (updatePolls) {
        update_polls();
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  if (tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_set_background_color(s_main_window, GColorBlack);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Register with TickTimerService
  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Init complete!");
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

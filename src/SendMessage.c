#include <pebble.h>

// #undef APP_LOG
// #define APP_LOG(...)
  
#define MSG_KEY 1
#define LABEL1_KEY 2
#define LABEL2_KEY 3
#define LABEL3_KEY 4
#define QUERY1_KEY 5
#define QUERY2_KEY 6
#define QUERY3_KEY 7
#define TEXT1_KEY 8
#define TEXT2_KEY 9
#define TEXT3_KEY 10

// If you want to change this, you'll need to define more keys above and edit the sendmessage function below and the javascript to use them.
#define MAX_QUERIES 3

static Window *window;
static TextLayer *message1_layer;
static TextLayer *message2_layer;
static TextLayer *message3_layer;
static char label[3][20];
static uint8_t querynum, queries[MAX_QUERIES];
static TextLayer *hint_layer = NULL;
static char hint_text[40];
static GRect hint_layer_size;
static uint8_t message;

#ifdef PBL_MICROPHONE
static DictationSession *s_dictation_session[MAX_QUERIES];
#else
#include <tertiary_text.h>
void mytertiarytextcallback(const char* result, size_t result_length, void* extra);
#endif
static char s_last_text[MAX_QUERIES][512];

static void send_message() {   
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got to sending message.");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
      vibes_short_pulse();
      APP_LOG(APP_LOG_LEVEL_WARNING, "Can not send request to phone!");
      return;
    }
    dict_write_uint8(iter, MSG_KEY, message);
    dict_write_cstring(iter, TEXT1_KEY, s_last_text[0]);
    dict_write_cstring(iter, TEXT2_KEY, s_last_text[1]);
    dict_write_cstring(iter, TEXT3_KEY, s_last_text[2]);
    const uint32_t final_size = dict_write_end(iter);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent message '%d' to phone! (%d bytes)", message, (int) final_size);
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
    snprintf(hint_text, sizeof(hint_text), "Sent\nmessage\n'%s'",label[message-1]);
    text_layer_set_text(hint_layer, hint_text);   
  }
}

static void build_message() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Building message %d of %d.", querynum, queries[message-1]);
#ifdef PBL_MICROPHONE
  if ((querynum < queries[message-1]) && (querynum < MAX_QUERIES) && (s_dictation_session[querynum]))
    dictation_session_start(s_dictation_session[querynum]);
#else
  if ((querynum < queries[message-1]) && (querynum < MAX_QUERIES))
    tertiary_text_prompt(label[message-1], mytertiarytextcallback, NULL);
#endif
else
  send_message();
}
    
#ifdef PBL_MICROPHONE
static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got to the callback.");
  if(status == DictationSessionStatusSuccess) {
    // Display the dictated text
    APP_LOG(APP_LOG_LEVEL_DEBUG, transcription);
    strncpy(s_last_text[querynum], transcription, sizeof(s_last_text[querynum]));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got transcription %d of %d.", querynum, queries[message-1]);
    querynum++;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "About to build message %d of %d.", querynum, queries[message-1]);
    build_message();
 } else {
    // Display the reason for any error
    snprintf(hint_text, sizeof(hint_text), "Transcription\nfailed.\nError:%d", (int)status);
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
    text_layer_set_text(hint_layer, hint_text);
  }
}
#else
void mytertiarytextcallback(const char* result, size_t result_length, void* extra) { 
  APP_LOG(APP_LOG_LEVEL_DEBUG, result);
  strncpy(s_last_text[querynum], result, sizeof(s_last_text[querynum]));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got result %d of %d.", querynum, queries[message-1]);
  querynum++;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "About to build message %d of %d.", querynum, queries[message-1]);
  build_message();
}
#endif

static void up_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Up button pressed.");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else {
    message = 1;
    querynum = 0;
    build_message();
  }
}

static void select_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select button pressed.");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else {
     message = 2;
    querynum = 0;
    build_message();
  }
}

static void down_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Down button pressed.");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else {
    message = 3;
    querynum = 0;
    build_message();
  }
}

static void go_back_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Back button pressed.");
  if (hint_layer) {
    text_layer_destroy(hint_layer);
    hint_layer = NULL;
  } else
    window_stack_pop_all(true);  // Exit the app.  
}  

void out_sent_handler(DictionaryIterator *sent, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message accepted by phone.");
  if (!hint_layer) {
    Layer *window_layer = window_get_root_layer(window);
    hint_layer = text_layer_create(hint_layer_size);
    text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(hint_layer));
  }
  text_layer_set_text(hint_layer, "Message accepted by phone.");
  vibes_short_pulse();
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Message rejected by phone: %d", reason);
  if (reason == APP_MSG_SEND_TIMEOUT) {
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
  text_layer_set_text(hint_layer, "Message rejected by phone.");
  }
  vibes_long_pulse();
}

void in_received_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming message received.");

  Tuple *message_tuple = dict_find(iterator, MSG_KEY);
  if (message_tuple) {
    if (!hint_layer) {
      Layer *window_layer = window_get_root_layer(window);
      hint_layer = text_layer_create(hint_layer_size);
      text_layer_set_font(hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      text_layer_set_text_alignment(hint_layer, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(hint_layer));
    }
    snprintf(hint_text, sizeof(hint_text), "%s", message_tuple->value->cstring);
    text_layer_set_text(hint_layer, hint_text);   
  }
  Tuple *label1_tuple = dict_find(iterator, LABEL1_KEY);
  if (label1_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Got Label 1.");
    snprintf(label[0], sizeof(label[0]), "%s ", label1_tuple->value->cstring);
    text_layer_set_text(message1_layer, label[0]);   
  }
  Tuple *label2_tuple = dict_find(iterator, LABEL2_KEY);
  if (label2_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Got Label 2.");
    snprintf(label[1], sizeof(label[1]), "%s ", label2_tuple->value->cstring);
    text_layer_set_text(message2_layer, label[1]);   
  }
  Tuple *label3_tuple = dict_find(iterator, LABEL3_KEY);
  if (label3_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Got Label 3.");
    snprintf(label[2], sizeof(label[2]), "%s ", label3_tuple->value->cstring);
    text_layer_set_text(message3_layer, label[2]);   
  }
  Tuple *query1_tuple = dict_find(iterator, QUERY1_KEY);
  if (query1_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Got Query 1.");
    queries[0] = query1_tuple->value->int8;
  }
  Tuple *query2_tuple = dict_find(iterator, QUERY2_KEY);
  if (query2_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Got Query 2.");
    queries[1] = query2_tuple->value->int8;
  }
  Tuple *query3_tuple = dict_find(iterator, QUERY3_KEY);
  if (query3_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Got Query 3.");
    queries[2] = query3_tuple->value->int8;
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
  APP_LOG(APP_LOG_LEVEL_WARNING, "Could not handle message from watch: %d", reason);
}
 
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, go_back_handler);
//  window_long_click_subscribe(BUTTON_ID_UP, 0, reset_handler, NULL);
//  window_long_click_subscribe(BUTTON_ID_SELECT, 0, reset_handler, NULL);
//  window_long_click_subscribe(BUTTON_ID_DOWN, 0, reset_handler, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
#if defined(PBL_ROUND)
  message1_layer = text_layer_create(GRect(18, 24, 144, 40));
#else
  message1_layer = text_layer_create(GRect(0, 12, 144, 40));
#endif
#ifdef PBL_COLOR
  text_layer_set_background_color(message1_layer, GColorRajah);
#else
  text_layer_set_background_color(message1_layer, GColorLightGray);
#endif  
  text_layer_set_font(message1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(message1_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(message1_layer));
  
#if defined(PBL_ROUND)
  message2_layer = text_layer_create(GRect(18, 70, 144, 40));
#else
  message2_layer = text_layer_create(GRect(0, 64, 144, 40));
#endif
#ifdef PBL_COLOR
  text_layer_set_background_color(message2_layer, GColorSunsetOrange);
#else
  text_layer_set_background_color(message2_layer, GColorLightGray);
#endif  
  text_layer_set_font(message2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(message2_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(message2_layer));
  
#if defined(PBL_ROUND)
  message3_layer = text_layer_create(GRect(18, 116, 144, 40));
#else
  message3_layer = text_layer_create(GRect(0, 116, 144, 40));
#endif
#ifdef PBL_COLOR
  text_layer_set_background_color(message3_layer, GColorGreen);
#else
  text_layer_set_background_color(message3_layer, GColorLightGray);
#endif  
  text_layer_set_font(message3_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(message3_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(message3_layer)); 
}

static void window_unload(Window *window) {
  text_layer_destroy(message1_layer);
  text_layer_destroy(message2_layer);
  text_layer_destroy(message3_layer);
  if (hint_layer) text_layer_destroy(hint_layer);
}

static void init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
#if defined(PBL_ROUND)
  hint_layer_size = GRect(34, 48, 112, 88);
#else
  hint_layer_size = GRect(16, 36, 112, 88);
#endif
  const uint32_t inbound_size = 256;                     // was APP_MESSAGE_INBOX_SIZE_MINIMUM;
  const uint32_t outbound_size = 528 * MAX_QUERIES + 64; // was APP_MESSAGE_OUTBOX_SIZE_MINIMUM;
  app_message_open(inbound_size, outbound_size);
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);
#ifdef PBL_MICROPHONE
  for (uint8_t i=0; i < MAX_QUERIES; i++)
    s_dictation_session[i] = dictation_session_create(sizeof(s_last_text), dictation_session_callback, NULL);
#endif
}

static void deinit(void) {
#ifdef PBL_MICROPHONE
  for (uint8_t i=0; i < MAX_QUERIES; i++)
    dictation_session_destroy(s_dictation_session[i]);
#endif
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

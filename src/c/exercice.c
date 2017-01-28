#include <pebble.h>
#include "exercice.h"

static Window *s_main_window;
static TextLayer *s_text_layer_u;
static TextLayer *s_repetition_layer_m;
static TextLayer *s_time_layer_d;

static const int MAX_REPETITION = 5;

static int tMax = 0;
static int tMin = 0;
static int tWork = 0;
static int tRest = 0;
static int repetition = -2;
static bool phase_running = false;

static time_t s_start_time;

void compute_times();
void next_repetition(int elapsed_time, bool incomplete);

static void click_handler(ClickRecognizerRef recognizer, void *context) {
  if (repetition == -2) {
    repetition = -1;
  } else if (repetition == -1) {
    compute_times();
  } else if (repetition < MAX_REPETITION * 2 &&  repetition % 2 == 1) {
    next_repetition(time(NULL) - s_start_time, true);
  }
}

static void run_phase_c() {
  text_layer_set_text(s_text_layer_u, "Max time");
  text_layer_set_text(s_repetition_layer_m, NULL);
  int seconds = (int)tMax % 60;
  int minutes = (int)tMax / 60 % 100;
  static char et_buffer[] = "00:00";
  snprintf(et_buffer, sizeof(et_buffer), "%d:%02d", minutes, seconds);
  text_layer_set_text(s_time_layer_d, et_buffer);
  
  tMin = tMin / 0.8;
  if (tMin > tMax) {
    tMin = tMax;
  }
  
  seconds = (int)tMin % 60;
  minutes = (int)tMin / 60 % 100;
  static char et_buffer_2[] = "00:00";
  snprintf(et_buffer_2, sizeof(et_buffer_2), "%d:%02d", minutes, seconds);
  text_layer_set_text(s_repetition_layer_m, et_buffer_2);
  
  vibes_double_pulse();
}

static void run_phase_a(time_t tick) {
  if (!phase_running) {
    phase_running = true;
    s_start_time = tick;
    text_layer_set_text(s_text_layer_u, "Hold as long as you can");
  }
  
  int elapsed_time = tick - s_start_time;
  int seconds = (int)elapsed_time % 60;
  int minutes = (int)elapsed_time / 60 % 100;
  
  static char et_buffer[] = "00:00";
  snprintf(et_buffer, sizeof(et_buffer), "%d:%02d", minutes, seconds);
  text_layer_set_text(s_time_layer_d, et_buffer);
}

static void run_phase_b(time_t tick) {
  bool isRest = repetition % 2 == 0;
  if (!phase_running) {
    phase_running = true;
    s_start_time = tick;
    text_layer_set_text(s_text_layer_u, isRest ? "Rest" : "Work");
    static char rep_buffer[] = "0";
    snprintf(rep_buffer, sizeof(rep_buffer), "%d", MAX_REPETITION - repetition / 2);
    text_layer_set_text(s_repetition_layer_m, rep_buffer);
  }
  
  int elapsed_time = tick - s_start_time;
  int remaining_time = (isRest ? tRest : tWork) - elapsed_time;
  int seconds = (int)remaining_time % 60;
  int minutes = (int)remaining_time / 60 % 100;
  
  static char et_buffer[] = "00:00";
  snprintf(et_buffer, sizeof(et_buffer), "%d:%02d", minutes, seconds);
  text_layer_set_text(s_time_layer_d, et_buffer);
  
  if (remaining_time <= 0) {
    next_repetition(elapsed_time, false);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  time_t tick = time(NULL);
  if (repetition < -1) {
  } else if (repetition == -1) {
    run_phase_a(tick);
  } else if (repetition < MAX_REPETITION * 2) {
    run_phase_b(tick);
  } else {
    tick_timer_service_unsubscribe();
  }
}

static TextLayer *configure_text_layer(Layer *window_layer, GRect box, GFont font) {
  TextLayer *layer = text_layer_create(box);
  
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(layer));
  
  return layer;
}

static void text_layer_set_initial_values() {
  text_layer_set_text(s_time_layer_d, "00:00");
  text_layer_set_text(s_text_layer_u, "Get ready");
}

static void config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_DOWN, click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(s_main_window);
    
  GRect bounds = layer_get_bounds(window_layer);
  
  s_text_layer_u = configure_text_layer(window_layer, 
                                        GRect(0, PBL_IF_ROUND_ELSE(10, 0), bounds.size.w, 100), fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  s_time_layer_d = configure_text_layer(window_layer,
                                        GRect(0, bounds.size.h - PBL_IF_ROUND_ELSE(70, 45), bounds.size.w, 45), fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  s_repetition_layer_m = configure_text_layer(window_layer,
                                        GRect(0, 85, bounds.size.w, 50), fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_text_layer_u);
  text_layer_destroy(s_repetition_layer_m);
  text_layer_destroy(s_time_layer_d);
}

static void init(void) {
  s_main_window = window_create();
  
  window_set_background_color(s_main_window, GColorBlack);
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  text_layer_set_initial_values();
  
  window_set_click_config_provider(s_main_window, (ClickConfigProvider) config_provider);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

void compute_times() {
  phase_running = false;
  tMax = time(NULL) - s_start_time;
  tMin = tMax;
  tRest = tMax * 0.5;
  tWork = tMax * 0.8;
  tMin = tWork;
  repetition = 0;
}

void next_repetition(int elapsed_time, bool incomplete) {
  phase_running = false;
  repetition++;
  if (repetition >= MAX_REPETITION * 2) {
    run_phase_c();
  } else {
    vibes_short_pulse();
  }
  if (incomplete && elapsed_time < tMin) {
    tMin = elapsed_time;
  }
}
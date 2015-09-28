#include <pebble.h>

#define KEY_CONFIG_BACKGROUND_COLOR  0
#define KEY_CONFIG_FOREGROUND_COLOR  1
#define KEY_CONFIG_STATUS_BAR        2

/** Main screen window */
static Window *mainWindow = NULL;
/** A layer on the window for the bars */
static Layer *barLayer    = NULL;
/** The bars */
static GPath* hourBars[12] = { NULL };
static GPath* min10Bars[5] = { NULL };
static GPath* minBars[9]   = { NULL };
/** Layers for the battery */
static Layer *batteryLayer    = NULL;
static TextLayer *batteryTextLayer    = NULL;
/** Layer for date */
static TextLayer *dateTextLayer    = NULL;
/** Layer for BT connection */
static Layer *btLayer    = NULL;
/** BT icon path */
static GPath* btIcon = NULL;
/** font for status bar */
static GFont statusFont;

#define BAR_WIDTH     40
#define BAR_HEIGHT     6

static const int INTERNAL_VGAP =  4; // gap between bars
static const int EXTERNAL_VGAP =  7; // gap between groups (additional to internal)
static const int HOUR_HPOS     =  6; // horizontal position of hour bars
static const int MIN10_HPOS    = 52; // horizontal position of 10min bars
static const int MIN_HPOS      = 98; // horizontal position of minutes
static const int VPOS          = 162; // The lower bound of the bars
/** Definition of a single bar */
static const GPathInfo BAR_PATH_INFO = {
  .num_points = 4,
  .points = (GPoint []) {{0, 0}, {BAR_WIDTH, 0}, {BAR_WIDTH,BAR_HEIGHT}, {0,BAR_HEIGHT}}
};
/** Definition of BT icon */
static const GPathInfo BT_OK_PATH_INFO = {
  .num_points = 8,
  .points = (GPoint []) {{0,2}, {5,7}, {3,9}, {2,9}, {2,0}, {3,0}, {5,2}, {0,7}}
};

static time_t epoch_time;
static struct tm *tm_p;
static bool config_display_status_bar = false;
static int config_background_color;
static int config_foreground_color;
static int charge_percent;
static bool bt_connected = false;

/** Main time function */
static void update_time() {
	layer_mark_dirty(barLayer);
	layer_mark_dirty(batteryLayer);
	layer_mark_dirty(text_layer_get_layer(batteryTextLayer));

	if (config_display_status_bar) {
		char *sys_locale = setlocale(LC_ALL, "");
		epoch_time = time( NULL );
		tm_p = localtime( &epoch_time );
		static char s_date_buffer[32];
		if (strcmp("de_DE", sys_locale) == 0) {
			snprintf(s_date_buffer, sizeof(s_date_buffer), "%02d.%02d.%02d", tm_p->tm_mday, tm_p->tm_mon+1, tm_p->tm_year-100);
		} else if (strcmp("fr_FR", sys_locale) == 0) {
			snprintf(s_date_buffer, sizeof(s_date_buffer), "%02d.%02d.%02d", tm_p->tm_mday, tm_p->tm_mon+1, tm_p->tm_year-100);
		} else if (strcmp("es_ES", sys_locale) == 0) {
			snprintf(s_date_buffer, sizeof(s_date_buffer), "%02d.%02d.%02d", tm_p->tm_mday, tm_p->tm_mon+1, tm_p->tm_year-100);
		} else { // en_US and ch_CN
			snprintf(s_date_buffer, sizeof(s_date_buffer), "%02d/%02d/%02d", tm_p->tm_mon+1, tm_p->tm_mday, tm_p->tm_year-100);
		}
		text_layer_set_text(dateTextLayer, s_date_buffer);
	} else {
		text_layer_set_text(dateTextLayer, "");
	}

	layer_mark_dirty(text_layer_get_layer(dateTextLayer));
	layer_mark_dirty(btLayer);
}

/** Handler: the bluetooth layer draw */
static void bluetooth_layer_draw(Layer *layer, GContext *ctx) {
	if (config_display_status_bar) {
#ifdef PBL_COLOR
		if (bt_connected) {
			graphics_context_set_stroke_color(ctx, GColorFromHEX(config_foreground_color));
			gpath_draw_outline_open(ctx, btIcon);
		} else {
//			graphics_context_set_stroke_color(ctx, GColorRed);
		}
#elif PBL_BW
		if (bt_connected) {
			graphics_context_set_stroke_color(ctx, config_foreground_color == 0 ? GColorBlack : GColorWhite);
			for (unsigned int i=0; i<BT_OK_PATH_INFO.num_points-1; i++) {
				graphics_draw_line(ctx, BT_OK_PATH_INFO.points[i], BT_OK_PATH_INFO.points[i+1]);
			}
		}
#endif

	}
}

/** Handler: draw the bar layer */
static void bar_layer_draw(Layer *layer, GContext *ctx) {
#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorFromHEX(config_foreground_color));
#elif PBL_BW
	graphics_context_set_fill_color(ctx, config_foreground_color == 0 ? GColorBlack : GColorWhite);
#endif
	epoch_time = time( NULL );
	tm_p = localtime( &epoch_time );
	int hours = tm_p->tm_hour == 12 ? 13 : tm_p->tm_hour % 12;
	int min10 = tm_p->tm_min / 10;
	int min = tm_p->tm_min % 10;

	for (int i=0; i<hours; i++) gpath_draw_filled(ctx, hourBars[i]);
	for (int i=0; i<min10; i++)  gpath_draw_filled(ctx, min10Bars[i]);
	for (int i=0; i<min; i++)  gpath_draw_filled(ctx, minBars[i]);
}

/** Handler: the status layer draw */
static void battery_layer_draw(Layer *layer, GContext *ctx) {
	if (config_display_status_bar) {
#ifdef PBL_COLOR
		if (charge_percent > 20) {
			graphics_context_set_fill_color(ctx, GColorFromHEX(config_foreground_color));
			graphics_context_set_stroke_color(ctx, GColorFromHEX(config_foreground_color));
			text_layer_set_text_color(batteryTextLayer, GColorFromHEX(config_foreground_color));
		} else {
			graphics_context_set_fill_color(ctx, GColorRed);
			graphics_context_set_stroke_color(ctx, GColorRed);
			text_layer_set_text_color(batteryTextLayer, GColorRed);
		}
#elif PBL_BW
		graphics_context_set_fill_color(ctx, config_foreground_color == 0 ? GColorBlack : GColorWhite);
		graphics_context_set_stroke_color(ctx, config_foreground_color == 0 ? GColorBlack : GColorWhite);
#endif
		GRect bounds = layer_get_bounds(layer);

		// Find the width of the bar
		int width = (int)(float)(charge_percent * (bounds.size.w-6) / 100);
		//width = bounds.size.w-4;

		// Draw the rectangle around
		graphics_draw_rect(ctx, GRect(0, 0, bounds.size.w-2, bounds.size.h));
		//graphics_fill_rect(ctx, bounds, 0, GCornerNone);

		// Draw the bar
		graphics_fill_rect(ctx, GRect(2, 2, width, bounds.size.h-4), 0, GCornerNone);
		graphics_fill_rect(ctx, GRect(bounds.size.w-2, 2, 3, bounds.size.h-4), 0, GCornerNone);

		// Print the value
		static char s_battery_buffer[32];
		snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d %%", charge_percent);
		text_layer_set_text(batteryTextLayer, s_battery_buffer);

	} else {
		text_layer_set_text(batteryTextLayer, "");
	}
}

/** Easily create the bars */
static GPath* create_bar(int x, int y) {
	GPath *rc = gpath_create(&BAR_PATH_INFO);
	gpath_move_to(rc, GPoint(x, y));
	return rc;
}

static void set_background_color(int color) {
	config_background_color = color;
#ifdef PBL_COLOR
	GColor gcolor = GColorFromHEX(color);
#elif PBL_BW
	GColor gcolor = color == 0 ? GColorBlack : GColorWhite;
#endif
	window_set_background_color(mainWindow, gcolor);
}

static void set_foreground_color(int color) {
	config_foreground_color = color;
#ifdef PBL_COLOR
	text_layer_set_text_color(batteryTextLayer, GColorFromHEX(color));
	text_layer_set_text_color(dateTextLayer, GColorFromHEX(color));
#elif PBL_BW
	text_layer_set_text_color(batteryTextLayer, color == 0 ? GColorBlack : GColorWhite);
	text_layer_set_text_color(dateTextLayer, color == 0 ? GColorBlack : GColorWhite);
#endif
}

/** The battery status callback */
static void battery_handler(BatteryChargeState new_state) {
	// Write to buffer and display
	charge_percent = new_state.charge_percent;
	update_time();
}

/** Handler: Window was loaded to screen */
static void main_window_load(Window *window) {
	// Custom Font
	statusFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LUCIDA_10));

	// Create the graphics layer
	barLayer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(barLayer, bar_layer_draw);
	layer_add_child(window_get_root_layer(mainWindow), barLayer);

	// Battery Layer
	batteryLayer = layer_create(GRect(120, 2, 20, 8));
	layer_set_update_proc(batteryLayer, battery_layer_draw);
	layer_add_child(window_get_root_layer(mainWindow), batteryLayer);

	// Battery Text Layer
	batteryTextLayer = text_layer_create(GRect(85, 0, 30, 10));
	text_layer_set_background_color(batteryTextLayer, GColorClear);
	text_layer_set_font(batteryTextLayer, statusFont);
	text_layer_set_text_alignment(batteryTextLayer, GTextAlignmentRight);
	layer_add_child(window_get_root_layer(mainWindow), text_layer_get_layer(batteryTextLayer));

	// Date Layer
	dateTextLayer = text_layer_create(GRect(3, 0, 50, 10));
	text_layer_set_background_color(dateTextLayer, GColorClear);
	text_layer_set_font(dateTextLayer, statusFont);
	text_layer_set_text_alignment(dateTextLayer, GTextAlignmentLeft);
	layer_add_child(window_get_root_layer(mainWindow), text_layer_get_layer(dateTextLayer));

	// Bluetooth Layer
	btLayer = layer_create(GRect(68, 1, 8, 10));
	btIcon = gpath_create(&BT_OK_PATH_INFO);
	layer_set_update_proc(btLayer, bluetooth_layer_draw);
	layer_add_child(window_get_root_layer(mainWindow), btLayer);

	// Create the bars as resources
	for (int i=0; i<12; i++) {
		int eGaps = i/3;
		hourBars[i] = create_bar(HOUR_HPOS, VPOS-(i+1)*BAR_HEIGHT-eGaps*EXTERNAL_VGAP-i*INTERNAL_VGAP);
	}
	for (int i=0; i<5; i++) {
		int eGaps = i/3;
		min10Bars[i] = create_bar(MIN10_HPOS, VPOS-(i+1)*BAR_HEIGHT-eGaps*EXTERNAL_VGAP-i*INTERNAL_VGAP);
	}
	for (int i=0; i<9; i++) {
		int eGaps = i/3;
		minBars[i]   = create_bar(MIN_HPOS, VPOS-(i+1)*BAR_HEIGHT-eGaps*EXTERNAL_VGAP-i*INTERNAL_VGAP);
	}

	if (persist_exists(KEY_CONFIG_BACKGROUND_COLOR)) {
		int background_color = persist_read_int(KEY_CONFIG_BACKGROUND_COLOR);
		set_background_color(background_color);
	} else {
		set_background_color(0x000000);
	}

	if (persist_exists(KEY_CONFIG_FOREGROUND_COLOR)) {
		int foreground_color = persist_read_int(KEY_CONFIG_FOREGROUND_COLOR);
		set_foreground_color(foreground_color);
	} else {
		set_foreground_color(0xffffff);
	}

	if (persist_exists(KEY_CONFIG_STATUS_BAR)) {
		config_display_status_bar = persist_read_bool(KEY_CONFIG_STATUS_BAR);
	} else {
		config_display_status_bar = false;
	}

}

/** Handler: Window was removed from screen */
static void main_window_unload(Window *window) {
	// Destroy the bars
	for (int i=0; i<12; i++) gpath_destroy(hourBars[i]);
	for (int i=0; i<5; i++)  gpath_destroy(min10Bars[i]);
	for (int i=0; i<9; i++)  gpath_destroy(minBars[i]);

	// Destroy fonts
	fonts_unload_custom_font(statusFont);

	// Destroy the layers
	layer_destroy(barLayer);
	layer_destroy(batteryLayer);
	layer_destroy(btLayer);
	text_layer_destroy(batteryTextLayer);
	text_layer_destroy(dateTextLayer);
}

/** Handler: time changed */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
}

static void bt_connection_handler(bool connected) {
	bt_connected = connected;
	layer_mark_dirty(btLayer);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *background_color_t = dict_find(iter, KEY_CONFIG_BACKGROUND_COLOR);
	Tuple *foreground_color_t = dict_find(iter, KEY_CONFIG_FOREGROUND_COLOR);
	Tuple *status_bar_t = dict_find(iter, KEY_CONFIG_STATUS_BAR);

	if (background_color_t) {
		int background_color = background_color_t->value->int32;
		persist_write_int(KEY_CONFIG_BACKGROUND_COLOR, background_color);
		set_background_color(background_color);
	}

	if (foreground_color_t) {
		int foreground_color = foreground_color_t->value->int32;
		persist_write_int(KEY_CONFIG_FOREGROUND_COLOR, foreground_color);
		set_foreground_color(foreground_color);
	}

	if (status_bar_t) {
		config_display_status_bar = status_bar_t->value->int8;
		persist_write_int(KEY_CONFIG_STATUS_BAR, config_display_status_bar);
	}

	update_time();
}

/** App initialization */
static void init() {
	// Create main Window
	mainWindow = window_create();
#ifdef PBL_COLOR
	window_set_background_color(mainWindow, GColorFromHEX(config_background_color));
#elif PBL_BW
	window_set_background_color(mainWindow, config_background_color == 0 ? GColorBlack : GColorWhite);
#endif

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(mainWindow, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	// Register the time handler */
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	// Show the Window on the watch, with animated=true
	window_stack_push(mainWindow, true);

	// App communication with phone
	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

	// Battery service registration
	battery_state_service_subscribe(battery_handler);
	BatteryChargeState bs = battery_state_service_peek();
	charge_percent = bs.charge_percent;

	// Bluetooth status
	bluetooth_connection_service_subscribe(bt_connection_handler);
	bt_connected = bluetooth_connection_service_peek();

	// Make sure time is displayed right from the beginning
	update_time();
}

/** App cleanup */
static void deinit() {
	// Cleanup window
	window_destroy(mainWindow);
}

/** C main function */
int main(void) {
	init();
	app_event_loop();
	deinit();
}

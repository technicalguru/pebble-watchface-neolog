#include <pebble.h>

/** Main screen window */
static Window *mainWindow = NULL;
/** A layer on the window for the bars */
static Layer *barLayer    = NULL;
/** The bars */
static GPath* hourBars[11] = { NULL };
static GPath* min10Bars[5] = { NULL };
static GPath* minBars[9]   = { NULL };

#define BAR_WIDTH     40
#define BAR_HEIGHT     6

static const int INTERNAL_VGAP =  4;
static const int EXTERNAL_VGAP =  7; // (additional to internal)
static const int HOUR_HPOS     =  6;
static const int MIN10_HPOS    = 52;
static const int MIN_HPOS      = 98;

/** Definition of a single bar */
static const GPathInfo BAR_PATH_INFO = {
  .num_points = 4,
  .points = (GPoint []) {{0, 0}, {BAR_WIDTH, 0}, {BAR_WIDTH,BAR_HEIGHT}, {0,BAR_HEIGHT}}
};

static time_t epoch_time;
static struct tm *tm_p;

/** Handler: draw the bar layer */
static void bar_layer_draw(Layer *layer, GContext *ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
	epoch_time = time( NULL );
	tm_p = localtime( &epoch_time );
	int hours = tm_p->tm_hour == 12 ? 12 : tm_p->tm_hour % 12;
	int min10 = tm_p->tm_min / 10;
	int min = tm_p->tm_min % 10;

	for (int i=0; i<hours; i++) gpath_draw_filled(ctx, hourBars[i]);
	for (int i=0; i<min10; i++)  gpath_draw_filled(ctx, min10Bars[i]);
	for (int i=0; i<min; i++)  gpath_draw_filled(ctx, minBars[i]);
}

/** Easily create the bars */
static GPath* create_bar(int x, int y) {
	GPath *rc = gpath_create(&BAR_PATH_INFO);
	gpath_move_to(rc, GPoint(x, y));
	return rc;
}

/** Handler: Window was loaded to screen */
static void main_window_load(Window *window) {
	// Create the graphics layer
	barLayer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(barLayer, bar_layer_draw);
	layer_add_child(window_get_root_layer(mainWindow), barLayer);

	// Create the bars as resources
	for (int i=0; i<11; i++) {
		int eGaps = i/3;
		hourBars[i] = create_bar(HOUR_HPOS, 155-(i+1)*BAR_HEIGHT-eGaps*EXTERNAL_VGAP-i*INTERNAL_VGAP);
	}
	for (int i=0; i<5; i++) {
		int eGaps = i/3;
		min10Bars[i] = create_bar(MIN10_HPOS, 155-(i+1)*BAR_HEIGHT-eGaps*EXTERNAL_VGAP-i*INTERNAL_VGAP);
	}
	for (int i=0; i<9; i++) {
		int eGaps = i/3;
		minBars[i]   = create_bar(MIN_HPOS, 155-(i+1)*BAR_HEIGHT-eGaps*EXTERNAL_VGAP-i*INTERNAL_VGAP);
	}

}

/** Handler: Window was removed from screen */
static void main_window_unload(Window *window) {
	// Destroy the bars
	for (int i=0; i<11; i++) gpath_destroy(hourBars[i]);
	for (int i=0; i<5; i++)  gpath_destroy(min10Bars[i]);
	for (int i=0; i<9; i++)  gpath_destroy(minBars[i]);

	// Destroy the layers
	layer_destroy(barLayer);
}

/** Main time function */
static void update_time() {
	layer_mark_dirty(barLayer);
}

/** Handler: time changed */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
}

/** App initialization */
static void init() {

	// Create main Window
	mainWindow = window_create();
	window_set_background_color(mainWindow, GColorBlack);

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(mainWindow, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	// Register the time handler */
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	// Show the Window on the watch, with animated=true
	window_stack_push(mainWindow, true);

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

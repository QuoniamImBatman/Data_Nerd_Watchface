#include <pebble.h>

static Window *s_main_window;

//Layers:
static TextLayer *s_time_layer, *s_weather_layer, *s_month_layer, *s_date_layer, *s_GMT_layer, *s_EST_layer, *s_alert_layer, *s_test_layer;


//Fonts:
static GFont s_time_font, s_date_font, s_weather_font, s_GMT_font, s_alert_font;

//Images:
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static BitmapLayer *s_weather_icon_layer;
static GBitmap *s_weather_icon_bitmap;

static BitmapLayer *s_connection_layer;
static GBitmap  *s_connection_bitmap;

//Battery level
static int s_battery_level;
static Layer *s_battery_layer;


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  
  //Getting UTC/GMT & EST:
	time_t temp2 = mktime(tick_time);
  struct tm * utc = gmtime(&temp2);
	
  
  // Write the current hours and minutes into a buffer
  static char s_GMT_buffer[16];
  strftime(s_GMT_buffer, sizeof(s_GMT_buffer), clock_is_24h_style() ?
                                          "GMT %H:%M" : "GMT %I:%M %P", utc);

  // Display this time on the TextLayer
  text_layer_set_text(s_GMT_layer, s_GMT_buffer);
  
	utc->tm_hour = (utc->tm_hour + 20) % 24; //Convert UTC to EST
  
  static char s_EST_buffer[16];
  strftime(s_EST_buffer, sizeof(s_EST_buffer), clock_is_24h_style() ?
                                          "EST %H:%M" : "EST %I:%M %P", utc);

  // Display this time on the TextLayer
  text_layer_set_text(s_EST_layer, s_EST_buffer);
  
  
  //DATE:
  // Copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a", tick_time);

  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);
  
  
  // Copy month into buffer from tm structure
  static char month_buffer[16];
  strftime(month_buffer, sizeof(date_buffer), "%d %b", tick_time);

  // Show the month
  text_layer_set_text(s_month_layer, month_buffer);
  
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 60 minutes
  if(tick_time->tm_min % 60 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}


static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 12.0f);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
	
	// Show "Charge Me" text if low battery
  layer_set_hidden(text_layer_get_layer(s_alert_layer), s_battery_level > 25);
}


static void bluetooth_callback(bool connected) {
  // Show icon if connected
  layer_set_hidden(bitmap_layer_get_layer(s_connection_layer), !connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}


static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  
  // Update meter
  layer_mark_dirty(s_battery_layer);

}


static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  
  //STATUS BAR LAYER
  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND_IMAGE);

  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  
  //TIME LAYER
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(4, PBL_IF_ROUND_ELSE(4, 10), 70, 34));
  
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_AR_Destine_Time_26));

  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  
  //GMT LAYER
  // Create the TextLayer with specific bounds
  s_GMT_layer = text_layer_create(
      GRect(6, PBL_IF_ROUND_ELSE(4, 44), 140, 34));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_GMT_layer, GColorClear);
  text_layer_set_text_color(s_GMT_layer, GColorBlack);
  text_layer_set_text(s_GMT_layer, "00:00");
  text_layer_set_text_alignment(s_GMT_layer, GTextAlignmentLeft);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_GMT_layer));
  
  //The font for the date
  s_GMT_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_VISITOR_TEXT_18));
  
  // Apply to TextLayer
  text_layer_set_font(s_GMT_layer, s_GMT_font);
  
  
  //EST LAYER
  // Create the TextLayer with specific bounds
  s_EST_layer = text_layer_create(
      GRect(6, 62, 140, 34));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_EST_layer, GColorClear);
  text_layer_set_text_color(s_EST_layer, GColorBlack);
  text_layer_set_text(s_EST_layer, "00:00");
  text_layer_set_text_alignment(s_EST_layer, GTextAlignmentLeft);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_EST_layer));
  
  // Apply to TextLayer
  text_layer_set_font(s_EST_layer, s_GMT_font);
  
  
  //DATE LAYER
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(80, 6, 70, 20));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);

  // Add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  //The font for the date
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_VISITOR_TEXT_18));
  
  //Apply it to the text layer
  text_layer_set_font(s_date_layer, s_date_font);

  //MONTH LAYER
  // Create date TextLayer
  s_month_layer = text_layer_create(GRect(80, 22, 60, 25));
  text_layer_set_text_color(s_month_layer, GColorBlack);
  text_layer_set_background_color(s_month_layer, GColorClear);
  text_layer_set_text_alignment(s_month_layer, GTextAlignmentLeft);

  // Add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_month_layer));
  
  //Apply it to the text layer
  text_layer_set_font(s_month_layer, s_date_font);
  
  
  //WEATHER LAYER
  // Create weather Layer
  s_weather_layer = text_layer_create(
    GRect(10, 120, bounds.size.w, 25));
  
  // Create second custom font, apply it and add to Window
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_VISITOR_TEXT_18));
  text_layer_set_font(s_weather_layer, s_weather_font);
  
  // Style the text
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
  text_layer_set_text(s_weather_layer, "Loading...");
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
  
  //WEATHER ICON
  // Create GBitmap
  s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_ROUND_IMAGE);

  // Create BitmapLayer to display the GBitmap
  s_weather_icon_layer = bitmap_layer_create(
    GRect( 116, 120, 24, 24)); 

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_weather_icon_layer));
  
  
  //CONNECTION LAYER
  // Create the Bluetooth icon GBitmap
  s_connection_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CONNECTION_IMAGE);

  // Create the BitmapLayer to display the GBitmap
  s_connection_layer = bitmap_layer_create(GRect(6, 0, 12, 12));
  bitmap_layer_set_bitmap(s_connection_layer, s_connection_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_connection_layer));
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  
  //BATTERY LAYER
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(124, 3, 12, 6));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);

	
	//ALERT LAYER
	// Create alerts Layer
  s_alert_layer = text_layer_create(
    GRect(42, PBL_IF_ROUND_ELSE(125, -6), 70, 15));
  
  // Create second custom font, apply it and add to Window
  s_alert_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_VISITOR_TEXT_14));
  text_layer_set_font(s_alert_layer, s_alert_font);
  
  // Style the text
  text_layer_set_background_color(s_alert_layer, GColorClear);
  text_layer_set_text_color(s_alert_layer, GColorWhite);
  text_layer_set_text_alignment(s_alert_layer, GTextAlignmentLeft);
  text_layer_set_text(s_alert_layer, "Charge me!");
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_alert_layer));
	
	
	//TEST LAYER
	// Create alerts Layer
  s_test_layer = text_layer_create(
    GRect(22, 90, 120, 15));
  
  text_layer_set_font(s_test_layer, s_alert_font);
  
  // Style the text
  text_layer_set_background_color(s_test_layer, GColorClear);
  text_layer_set_text_color(s_test_layer, GColorBlack);
  text_layer_set_text_alignment(s_test_layer, GTextAlignmentLeft);
  text_layer_set_text(s_test_layer, clock_is_timezone_set() ?
											"Timezone is set!" : "GMT set to Local time");
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_test_layer));
}


static void main_window_unload(Window *window) {
  // Destroy GBitmap
  gbitmap_destroy(s_weather_icon_bitmap);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_weather_icon_layer);

  
  //Destroy date elements
  fonts_unload_custom_font(s_date_font);

  text_layer_destroy(s_date_layer);

  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);

  // Destroy time elements
  text_layer_destroy(s_time_layer);
  
  // Unload GFont for time
  fonts_unload_custom_font(s_time_font);
  
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Destroy connection elements
  gbitmap_destroy(s_connection_bitmap);
  bitmap_layer_destroy(s_connection_layer);
  
  //Destroy battery elements
  layer_destroy(s_battery_layer);
  
  //Destroy GMT and EST elements:
  text_layer_destroy(s_GMT_layer);
  text_layer_destroy(s_EST_layer);
  fonts_unload_custom_font(s_GMT_font);
  
	//Destroy Other Layers
	text_layer_destroy(s_alert_layer);
  text_layer_destroy(s_test_layer);
	fonts_unload_custom_font(s_alert_font);
}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
 
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
    
		
    //WEATHER ICONS
    // This is an experimental section
    
    if(strcmp(conditions_buffer, "Clear") == 0) {
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_ROUND_IMAGE);
    } else if(strcmp(conditions_buffer, "Rain") == 0){
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_ROUND_IMAGE);
    } else if(strcmp(conditions_buffer, "Thunderstorm") == 0){
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_THUNDER_ROUND_IMAGE);
    } else {
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDS_ROUND_IMAGE);
    }
    
    // Display weather icon:
    bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon_bitmap);
    
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

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);

  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
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

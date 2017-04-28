#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_battery_layer; //text layer for battery percentage
static TextLayer *s_date_layer; //Layer layer for current date 
static TextLayer *s_dow_layer; //Layer layer for current date
static TextLayer *s_disconnect_layer;
static GFont s_time_font,s_date_font,s_battery_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static GBitmap *s_border_bitmap;
static GBitmap *s_erase_1,*s_erase_2;
static BitmapLayer *s_glitch_hour,*s_glitch_min;   
static BitmapLayer *s_border_layer;
static GBitmap *s_fade_bitmap;
static BitmapLayer *s_fade_layer;
int fade_sec =0; //set time since last fade to 0
int frame=0;
int f_delay =150;
int m_g_delay=90;
int h_g_delay=90;
int g_interval =18; //glitces every 'g_interval' seconds
bool vib_hour =false;
int last_hour =24; //that's not possible!
int m_g_frame =0;
int h_g_frame =0;
bool wasDisconnected =false; //previouly disconnected? 


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  
  // push the string to the layer
  //text_layer_set_text(s_time_layer, time);
  
  //--- DATE CODE---
  // Need to be static because they're used by the system later.
  static char s_date_text[] = "Xxxx 00";
  
  strftime(s_date_text, sizeof(s_date_text), "%b %e", tick_time);
 
  text_layer_set_text(s_date_layer, s_date_text);  
  
  //update day of week!
  static char week_day[] = "Xxx";
  strftime(week_day,sizeof(week_day),"%a",tick_time);
  text_layer_set_text(s_dow_layer,week_day);
}




  static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  static char s_battery_buffer[32];  //char for the battery percent
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", new_state.charge_percent); //get he battery info, and add the approprate text
  text_layer_set_text(s_battery_layer, s_battery_buffer); //set the layer battery text (percent + text) to the battery layer
  }


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
    //Create GFont for clock| Font:Tonik BRK Font [by Blambot Comic]
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CYBER_FONT_32));
  
  //create font for date/dow  |Font: Flipside BRK Font [by Ænigma Fonts]
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_cutout_14));
  
  //create font for Battery |Font: NinePin Font  [by Digital Graphic Labs]
  s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_digital_16));

  
  //Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_bg_blue);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  // Create time Layer
  s_time_layer = text_layer_create(GRect(2, 60, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorCyan);
  text_layer_set_text(s_time_layer, "00:00");
  
  //create layer for border
  s_border_bitmap = gbitmap_create_with_resource(RESOURCE_ID_border);
  s_border_layer = bitmap_layer_create(window_bounds);
  bitmap_layer_set_bitmap(s_border_layer,s_border_bitmap);
  bitmap_layer_set_compositing_mode(s_border_layer, GCompOpSet);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_border_layer));
  
  s_battery_layer = text_layer_create(GRect(55, 22, window_bounds.size.w, window_bounds.size.h));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft); //left align the text in this layer
  text_layer_set_background_color(s_battery_layer, GColorClear); //clear background
  #ifdef PBL_PLATFORM_APLITE //if running on older pebble
    text_layer_set_text_color(s_battery_layer, GColorBlack); /// black text
   #elif PBL_PLATFORM_BASALT  //if running on pebble time
    text_layer_set_text_color(s_battery_layer, GColorCyan); //red text
  #endif
  text_layer_set_font(s_battery_layer, s_battery_font); //change the font
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));//Add battery layer to our window
 
  // Get the current battery level
  battery_handler(battery_state_service_peek());
  
  //Create Date layer:
  //s_date_layer = text_layer_create(GRect(20,49, 136, 100)); old
  s_date_layer = text_layer_create(GRect(56,97, 136, 100));
  text_layer_set_background_color(s_date_layer, GColorClear);
   #ifdef PBL_PLATFORM_APLITE //if running on older pebble
    text_layer_set_text_color(s_date_layer, GColorBlack); /// black text
   #elif PBL_PLATFORM_BASALT  //if running on pebble time
    text_layer_set_text_color(s_date_layer, GColorElectricBlue); //blueish text
  #endif
  
  text_layer_set_font(s_date_layer,  s_date_font);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  
  
  //Create 'DAY OF WEEK layer:
 // s_dow_layer = text_layer_create(GRect(83,100, 136, 100)); old
  s_dow_layer = text_layer_create(GRect(20,49, 136, 100));
  text_layer_set_background_color(s_dow_layer, GColorClear);
  #ifdef PBL_PLATFORM_APLITE //if running on older pebble
    text_layer_set_text_color(s_dow_layer, GColorBlack); /// black text
  #elif PBL_PLATFORM_BASALT  //if running on pebble time
    text_layer_set_text_color(s_dow_layer, GColorElectricBlue); //dark blue text
  #endif
  
  text_layer_set_font(s_dow_layer, s_date_font);
  layer_add_child(window_layer, text_layer_get_layer(s_dow_layer));

  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  
  //layer for bt disconect
  s_disconnect_layer = text_layer_create(GRect(15,145, 139, 50));
  text_layer_set_background_color(s_disconnect_layer, GColorClear);
  text_layer_set_text_color(s_disconnect_layer, GColorCyan);
  text_layer_set_text(s_disconnect_layer, "-Connection Lost-");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_disconnect_layer));
  layer_set_hidden( (Layer *)s_disconnect_layer,true); //hide it.
  
  
    //create layer for fade
  s_fade_bitmap = gbitmap_create_with_resource(RESOURCE_ID_fade);
  s_fade_layer = bitmap_layer_create(window_bounds);
  bitmap_layer_set_bitmap(s_fade_layer,s_fade_bitmap);
  bitmap_layer_set_compositing_mode(s_fade_layer, GCompOpSet);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_fade_layer));
  layer_set_hidden( (Layer *)s_fade_layer,true); //hide it.
  
  //load bitmaps for erase layer
   s_erase_1 = gbitmap_create_with_resource(RESOURCE_ID_glitch_min_1);
   s_erase_2 = gbitmap_create_with_resource(RESOURCE_ID_glitch_min_2);
  //set up erase layer
   s_glitch_min = bitmap_layer_create(GRect(0,0, 144, 168));
   s_glitch_hour = bitmap_layer_create(GRect(-67,0, 144, 168));  
   bitmap_layer_set_bitmap(s_glitch_min,  s_erase_1);
   bitmap_layer_set_bitmap(s_glitch_hour,  s_erase_1);
   bitmap_layer_set_compositing_mode(s_glitch_min, GCompOpSet);
   bitmap_layer_set_compositing_mode(s_glitch_hour, GCompOpSet);
   layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_glitch_min));
   layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_glitch_hour));
   layer_set_hidden( (Layer *)s_glitch_min,true); //hide it.
   layer_set_hidden( (Layer *)s_glitch_hour,true); //hide it.
  
  
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_battery_font);
  
  //Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  gbitmap_destroy(s_border_bitmap);
  gbitmap_destroy(s_fade_bitmap);
  gbitmap_destroy(s_erase_1);
  gbitmap_destroy(s_erase_2);
  //Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  bitmap_layer_destroy(s_border_layer);
  bitmap_layer_destroy(s_fade_layer);
  bitmap_layer_destroy(s_glitch_min);
  bitmap_layer_destroy(s_glitch_hour);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_dow_layer);
  text_layer_destroy(s_disconnect_layer);
}


void bg_glitch(void * data){  
    frame++;
    int last_frame = 12;  
    int delay = f_delay;
    if (frame == 1 ||frame == 3 || frame == 5 ||frame == 7 ||frame == 9 || frame == 11)
      layer_set_hidden( (Layer *)s_fade_layer,false); //show it.
    if (frame == 4 || frame == 6 ||frame == 8)
        layer_set_hidden( (Layer *)s_fade_layer,true); //hide the fade layer
    if (frame != last_frame)
      app_timer_register(delay, bg_glitch, NULL);
  
    if (frame == last_frame){ //we're done! pack it up!
      layer_set_hidden( (Layer *)s_fade_layer,true); //hide the fade layer
      frame =0; // Set frame back to zero. NOTE: THIS MUST BE THE LAS LINE IN THE FRAME COUNTER, otherwise: if (frame != last_frame) will be tripped.  
     }
}

void glitch_min_ani(void * data){  
    m_g_frame++;
    int last_frame = 12;  
    int delay = m_g_delay;
    if ( m_g_frame ==1)
      layer_set_hidden( (Layer *)s_glitch_min,false);
  
    if (m_g_frame == 1 ||m_g_frame == 3 || m_g_frame == 5 ||m_g_frame == 7 ||m_g_frame == 9 ||m_g_frame == 11)
      {
    bitmap_layer_set_bitmap(s_glitch_min,  s_erase_1);
    layer_mark_dirty(bitmap_layer_get_layer(s_glitch_min)); 
      
       }
    if (m_g_frame == 2 ||m_g_frame == 4 ||m_g_frame== 6 ||m_g_frame == 8 ||m_g_frame == 10)
      {
    bitmap_layer_set_bitmap(s_glitch_min,  s_erase_2);
    layer_mark_dirty(bitmap_layer_get_layer(s_glitch_min)); 
       }
    if (m_g_frame != last_frame)
      app_timer_register(delay, glitch_min_ani, NULL);
  
    if (m_g_frame == last_frame){ //we're done! pack it up!
      layer_set_hidden( (Layer *)s_glitch_min,true); //hide the fade layer
      m_g_frame =0; // Set frame back to zero. NOTE: THIS MUST BE THE LAS LINE IN THE FRAME COUNTER, otherwise: if (frame != last_frame) will be tripped.  
     }
}


void glitch_hour_ani(void * data){  
    h_g_frame++;
    int last_frame = 12;  
    int delay = h_g_delay;
    if ( h_g_frame ==1)
      layer_set_hidden( (Layer *)s_glitch_hour,false);
  
    if (h_g_frame == 1 ||h_g_frame == 3 || h_g_frame == 5 ||h_g_frame == 7 ||h_g_frame == 9 ||h_g_frame == 11)
      {
    bitmap_layer_set_bitmap(s_glitch_hour,  s_erase_1);
    layer_mark_dirty(bitmap_layer_get_layer(s_glitch_hour)); 
      
       }
    if (h_g_frame == 2 ||h_g_frame == 4 ||h_g_frame== 6 ||h_g_frame == 8 ||h_g_frame == 10)
      {
    bitmap_layer_set_bitmap(s_glitch_hour,  s_erase_2);
    layer_mark_dirty(bitmap_layer_get_layer(s_glitch_hour)); 
       }
    if (h_g_frame != last_frame)
      app_timer_register(delay, glitch_hour_ani, NULL);
  
    if (h_g_frame == last_frame){ //we're done! pack it up!
      layer_set_hidden( (Layer *)s_glitch_hour,true); //hide the fade layer
      h_g_frame =0; // Set frame back to zero. NOTE: THIS MUST BE THE LAS LINE IN THE FRAME COUNTER, otherwise: if (frame != last_frame) will be tripped.  
     }
}




static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    fade_sec ++;//time has passed since last blink
    int seconds = tick_time->tm_sec;
    int hours = tick_time ->tm_hour;
    int min =  tick_time ->tm_min;
  
    if (bluetooth_connection_service_peek() ==false && wasDisconnected==false){ //if we WERE connected, and now are not, then do the following
    layer_set_hidden( (Layer *) s_disconnect_layer,false);  //show it on the screen
    wasDisconnected = true; //at last check, we WERE disconnected, 
    vibes_double_pulse(); //double vibrate, so they will know it's not a normal notication.
    }
  
  if (bluetooth_connection_service_peek() ==true && wasDisconnected ==true){ //if we WERE DISconnected (last we checked), and now are connected again, then do the following
    layer_set_hidden( (Layer *) s_disconnect_layer,true);//hide the layer
    wasDisconnected = false;// at last check  (now) we WERE connected
   // layer_mark_dirty(bitmap_layer_get_layer( s_disconnected_layer)); 
  }
  
  
    if (last_hour == 24) //if at the last update the hour was 24, then we just started the watch
      last_hour=hours; //our new 'last hour' will be the current time
    
    if (hours != last_hour && vib_hour ==true) //if hour has changed
     {
        vibes_short_pulse(); //vibrate (short) 
        last_hour=hours;//our new 'last hour' will be the current time
      }
  
    if (min == 59 && seconds == 59)
      app_timer_register(500, glitch_hour_ani, NULL); //start the hour glitch

    if (seconds ==59)
     app_timer_register(500, glitch_min_ani, NULL); //start the min glitch
  
    if (seconds == 0)
      {
      update_time();
      battery_handler(battery_state_service_peek());
    }
    if( fade_sec == g_interval) 
    {
     app_timer_register(0, bg_glitch, NULL);
      fade_sec=0;//yo, we just blinked, check it!
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

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) tick_handler);
  
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
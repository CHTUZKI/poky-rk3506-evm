/*
 * LVGL Test Utility
 * 
 * This program demonstrates LVGL graphics library functionality
 * on RK3506 EVM with DSI display.
 * 
 * Note: This is a simplified test that uses LVGL's built-in display
 * driver initialization (DRM or framebuffer based on configuration).
 * 
 * Usage: lvgl-test
 *   Press Ctrl+C to exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "lvgl.h"

/* Note: LVGL driver headers are included via lvgl.h, no need for forward declarations */

static volatile int running = 1;

/* Custom tick function for LVGL */
uint32_t custom_tick_get(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void signal_handler(int sig)
{
    running = 0;
}

/* Forward declarations */
static void slider_event_cb(lv_event_t * e);

/* Button event callback */
static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    
    /* Only print important events to reduce noise */
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        printf("Button event: code=%d\n", code);
        fflush(stdout);
    }
    
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;
        
        printf("Button clicked! Count: %d\n", cnt);
        fflush(stdout);
        
        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    } else if(code == LV_EVENT_PRESSED) {
        printf("Button pressed\n");
        fflush(stdout);
    } else if(code == LV_EVENT_RELEASED) {
        printf("Button released\n");
        fflush(stdout);
    }
}

/* Global references for progress bar (for animation) */
static lv_obj_t * g_bar = NULL;
static lv_obj_t * g_bar_label = NULL;

/* Global input device for debugging */
static lv_indev_t * g_indev = NULL;

/* Touch event callback for debugging */
static void touch_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    
    if(code == LV_EVENT_PRESSED) {
        lv_point_t point;
        lv_indev_t * indev = lv_indev_get_act();
        if(indev) {
            lv_indev_get_point(indev, &point);
            printf("Touch pressed at: x=%d, y=%d\n", point.x, point.y);
            fflush(stdout);
        }
    } else if(code == LV_EVENT_RELEASED) {
        lv_point_t point;
        lv_indev_t * indev = lv_indev_get_act();
        if(indev) {
            lv_indev_get_point(indev, &point);
            printf("Touch released at: x=%d, y=%d\n", point.x, point.y);
            fflush(stdout);
        }
    }
}

/* Create a simple test UI */
static void create_test_ui(void)
{
    printf("  Getting active screen...\n");
    fflush(stdout);
    lv_obj_t * screen = lv_scr_act();
    if(!screen) {
        fprintf(stderr, "Error: No active screen available\n");
        return;
    }
    printf("  Screen obtained successfully\n");
    fflush(stdout);
    
    /* Create a label for title */
    printf("  Creating title label...\n");
    fflush(stdout);
    lv_obj_t * title_label = lv_label_create(screen);
    if(!title_label) {
        fprintf(stderr, "Error: Failed to create title label\n");
        return;
    }
    printf("  Title label created\n");
    fflush(stdout);
    
    lv_label_set_text(title_label, "LVGL Test on RK3506");
    /* Use default font instead of specific font to avoid potential issues */
    /* lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0); */
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    printf("  Title label configured\n");
    fflush(stdout);
    
    /* Create a button */
    printf("  Creating button...\n");
    fflush(stdout);
    lv_obj_t * btn = lv_btn_create(screen);
    if(!btn) {
        fprintf(stderr, "Error: Failed to create button\n");
        return;
    }
    printf("  Button created\n");
    fflush(stdout);
    
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -50);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t * btn_label = lv_label_create(btn);
    if(!btn_label) {
        fprintf(stderr, "Error: Failed to create button label\n");
        return;
    }
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);
    printf("  Button configured\n");
    fflush(stdout);
    
    /* Create a progress bar */
    printf("  Creating progress bar...\n");
    fflush(stdout);
    g_bar = lv_bar_create(screen);
    if(!g_bar) {
        fprintf(stderr, "Error: Failed to create progress bar\n");
        return;
    }
    printf("  Progress bar created\n");
    fflush(stdout);
    
    lv_obj_set_size(g_bar, 200, 20);
    lv_obj_align(g_bar, LV_ALIGN_CENTER, 0, 20);
    lv_bar_set_value(g_bar, 50, LV_ANIM_ON);
    
    /* Create a label for progress bar */
    printf("  Creating progress bar label...\n");
    fflush(stdout);
    g_bar_label = lv_label_create(screen);
    if(!g_bar_label) {
        fprintf(stderr, "Error: Failed to create progress bar label\n");
        return;
    }
    printf("  Progress bar label created\n");
    fflush(stdout);
    
    lv_label_set_text(g_bar_label, "Progress: 50%");
    lv_obj_align(g_bar_label, LV_ALIGN_CENTER, 0, 50);
    
    /* Create a slider */
    printf("  Creating slider...\n");
    fflush(stdout);
    lv_obj_t * slider = lv_slider_create(screen);
    if(!slider) {
        fprintf(stderr, "Error: Failed to create slider\n");
        return;
    }
    printf("  Slider created\n");
    fflush(stdout);
    
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 80);
    lv_slider_set_value(slider, 70, LV_ANIM_OFF);
    
    /* Create a label for slider */
    printf("  Creating slider label...\n");
    fflush(stdout);
    lv_obj_t * slider_label = lv_label_create(screen);
    if(!slider_label) {
        fprintf(stderr, "Error: Failed to create slider label\n");
        return;
    }
    printf("  Slider label created\n");
    fflush(stdout);
    
    lv_label_set_text(slider_label, "Slider: 70");
    lv_obj_align(slider_label, LV_ALIGN_CENTER, 0, 110);
    
    /* Add event callback with label as user data */
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, slider_label);
    
    printf("  All UI elements created successfully\n");
    fflush(stdout);
    
    /* Create an info label */
    lv_obj_t * info_label = lv_label_create(screen);
    if(info_label) {
        lv_label_set_text(info_label, "LVGL is working!");
        lv_obj_set_style_text_color(info_label, lv_color_hex(0x00FF00), 0);
        lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    }
}

/* Slider event callback to update label */
static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    
    /* Get user data which should be the label */
    lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);
    if(label) {
        lv_label_set_text_fmt(label, "Slider: %d", value);
    }
}

/* Update progress bar animation */
static void update_progress_bar(lv_obj_t * bar, lv_obj_t * label)
{
    static int32_t value = 0;
    static int8_t dir = 1;
    
    value += dir * 5;
    if(value >= 100) {
        value = 100;
        dir = -1;
    } else if(value <= 0) {
        value = 0;
        dir = 1;
    }
    
    lv_bar_set_value(bar, value, LV_ANIM_ON);
    lv_label_set_text_fmt(label, "Progress: %d%%", value);
}

int main(int argc, char *argv[])
{
    /* Ensure output is not buffered */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    printf("LVGL Test Utility for RK3506 EVM\n");
    printf("================================\n");
    fflush(stdout);
    
    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Initialize LVGL */
    lv_init();
    
    printf("LVGL initialized\n");
    fflush(stdout);
    
    /* Declare input device variable (used in touchscreen initialization) */
    lv_indev_t * indev = NULL;
    
    /* Initialize display driver */
#if LV_USE_LINUX_DRM
    printf("Initializing DRM display driver...\n");
    const char *videocard = "/dev/dri/card0";
    lv_display_t * disp = lv_linux_drm_create();
    if(!disp) {
        fprintf(stderr, "Failed to create DRM display\n");
        return 1;
    }
    lv_linux_drm_set_file(disp, videocard, (int64_t)-1);
    printf("DRM display initialized: %s\n", videocard);
    fflush(stdout);
    
    /* Wait for DRM to be fully ready */
    usleep(100000);  /* 100ms */
    
    /* Initialize tick before first render */
    lv_tick_inc(10);
    
    /* Wait longer for DRM to be fully ready */
    usleep(200000);  /* 200ms */
    
    /* Initialize tick before first render */
    lv_tick_inc(200);
    
    /* Do an initial render to initialize display buffers */
    printf("Initializing display buffers...\n");
    fflush(stdout);
    
    /* Get or create default screen */
    lv_obj_t * screen = lv_scr_act();
    if(!screen) {
        fprintf(stderr, "Error: Failed to get default screen\n");
        return 1;
    }
    
    /* Render empty screen first to initialize buffers */
    lv_timer_handler();
    usleep(100000);  /* 100ms */
    
    /* Render again to ensure buffers are ready */
    lv_tick_inc(100);
    lv_timer_handler();
    usleep(100000);  /* 100ms */
    
    printf("Display buffers initialized\n");
    fflush(stdout);
    
#elif LV_USE_LINUX_FBDEV
    printf("Initializing framebuffer display driver...\n");
    const char *videocard = "/dev/fb0";
    lv_display_t * disp = lv_linux_fbdev_create();
    if(!disp) {
        fprintf(stderr, "Failed to create framebuffer display\n");
        return 1;
    }
    lv_linux_fbdev_set_file(disp, videocard);
    printf("Framebuffer display initialized: %s\n", videocard);
#else
    fprintf(stderr, "Error: No display driver configured (neither DRM nor FBDEV)\n");
    return 1;
#endif
    
    /* Initialize input device (touchscreen) - for both DRM and FBDEV */
#if LV_USE_EVDEV
    printf("Initializing touchscreen input device...\n");
    fflush(stdout);
    
    /* First, try to find touchscreen by scanning /dev/input/ */
    const char *touchscreen_paths[] = {
        "/dev/input/touchscreen0",  /* Udev symlink if available */
        NULL
    };
    
    /* Try udev symlink first */
    for(int i = 0; touchscreen_paths[i] != NULL; i++) {
            if(access(touchscreen_paths[i], R_OK) == 0) {
                printf("  Trying touchscreen device: %s\n", touchscreen_paths[i]);
                fflush(stdout);
                indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, touchscreen_paths[i]);
                if(indev) {
                    printf("  ✓ Touchscreen initialized: %s\n", touchscreen_paths[i]);
                    fflush(stdout);
                    break;
                } else {
                    printf("  ✗ Failed to create evdev device for %s\n", touchscreen_paths[i]);
                    fflush(stdout);
                }
            }
    }
    
    /* If not found, scan all event devices - try event1 first (often the touchscreen) */
    if(!indev) {
        printf("  Scanning /dev/input/event* devices...\n");
        fflush(stdout);
        
        /* Try event1 first, as event0 is often keyboard or other input */
        const char *preferred_devices[] = {"/dev/input/event1", "/dev/input/event0", NULL};
        for(int p = 0; preferred_devices[p] != NULL; p++) {
            if(access(preferred_devices[p], R_OK) == 0) {
                printf("  Trying input device: %s\n", preferred_devices[p]);
                fflush(stdout);
                
                indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, preferred_devices[p]);
                if(indev) {
                    printf("  ✓ Touchscreen initialized: %s\n", preferred_devices[p]);
                    fflush(stdout);
                    break;
                } else {
                    printf("  ✗ Failed to create evdev device for %s (may not be a touchscreen)\n", preferred_devices[p]);
                    fflush(stdout);
                }
            }
        }
        
        /* If preferred devices didn't work, try all other event devices */
        if(!indev) {
            for(int i = 0; i < 10; i++) {
                char event_path[64];
                snprintf(event_path, sizeof(event_path), "/dev/input/event%d", i);
                
                /* Skip event0 and event1 as we already tried them */
                if(i <= 1) continue;
                
                if(access(event_path, R_OK) == 0) {
                    printf("  Trying input device: %s\n", event_path);
                    fflush(stdout);
                    
                    indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, event_path);
                    if(indev) {
                        printf("  ✓ Touchscreen initialized: %s\n", event_path);
                        fflush(stdout);
                        break;
                    } else {
                        printf("  ✗ Failed to create evdev device for %s (may not be a touchscreen)\n", event_path);
                        fflush(stdout);
                    }
                }
            }
        }
    }
    
    if(!indev) {
        printf("  ⚠ Warning: No touchscreen device found, continuing without touch input\n");
        printf("  Tip: Check /dev/input/event* devices and ensure LV_USE_EVDEV is enabled\n");
        fflush(stdout);
    } else {
        printf("  Touchscreen is ready for input\n");
        
        /* Store global reference for debugging */
        g_indev = indev;
        
        /* Get display size for debugging */
        lv_display_t * disp = lv_display_get_default();
        if(disp) {
            int32_t hor_res = lv_display_get_horizontal_resolution(disp);
            int32_t ver_res = lv_display_get_vertical_resolution(disp);
            printf("  Display resolution: %dx%d\n", hor_res, ver_res);
        }
        
        /* Add touch event listener to screen for debugging */
        lv_obj_t * screen = lv_scr_act();
        if(screen) {
            lv_obj_add_event_cb(screen, touch_event_cb, LV_EVENT_PRESSED, NULL);
            lv_obj_add_event_cb(screen, touch_event_cb, LV_EVENT_RELEASED, NULL);
            printf("  Touch event listeners added to screen\n");
        }
        
        fflush(stdout);
    }
#endif
    
    /* Create test UI */
    printf("Creating test UI...\n");
    fflush(stdout);
    
    /* Verify screen is still valid before creating UI */
    lv_obj_t * screen_check = lv_scr_act();
    if(!screen_check) {
        fprintf(stderr, "Error: Screen is not available before creating UI\n");
        return 1;
    }
    
    create_test_ui();
    
    /* Wait and do initial render after UI creation */
    printf("Rendering UI...\n");
    fflush(stdout);
    
    /* Wait for display to be ready */
    usleep(200000);  /* 200ms - longer delay for DRM */
    
    /* Increment tick before rendering */
    lv_tick_inc(200);
    
    /* First render with UI - this is where the crash might occur */
    printf("  Calling lv_timer_handler() for first render...\n");
    fflush(stdout);
    lv_timer_handler();  /* First render with UI */
    
    printf("  First render completed\n");
    fflush(stdout);
    
    usleep(100000);  /* 100ms */
    
    /* Render again to ensure UI is displayed */
    lv_tick_inc(100);
    printf("  Calling lv_timer_handler() for second render...\n");
    fflush(stdout);
    lv_timer_handler();
    
    printf("  Second render completed\n");
    fflush(stdout);
    usleep(50000);  /* 50ms */
    
    printf("Test UI created\n");
    printf("Display should show LVGL interface\n");
    fflush(stdout);
    
#if LV_USE_EVDEV
    if(indev) {
        printf("Touchscreen input is enabled - try tapping the screen\n");
    } else {
        printf("Touchscreen input is NOT available\n");
    }
    fflush(stdout);
#endif
    
    printf("Press Ctrl+C to exit\n\n");
    fflush(stdout);
    
    /* Initialize tick counter */
    uint32_t last_tick = custom_tick_get();
    
    printf("Entering main loop...\n");
    fflush(stdout);
    
    /* Main loop */
    static uint32_t last_touch_check = 0;
    while(running) {
        /* Calculate elapsed time and update LVGL tick */
        uint32_t current_tick = custom_tick_get();
        uint32_t elapsed = current_tick - last_tick;
        if(elapsed > 0) {
            lv_tick_inc(elapsed);
            last_tick = current_tick;
        } else {
            /* Ensure tick is always incremented, even if elapsed is 0 */
            lv_tick_inc(5);  /* Minimum 5ms per loop */
        }
        
        /* Check touch state periodically for debugging */
#if LV_USE_EVDEV
        if(g_indev && (current_tick - last_touch_check > 500)) {  /* Check every 500ms */
            lv_point_t point;
            lv_indev_get_point(g_indev, &point);
            if(point.x > 0 || point.y > 0) {
                printf("Touch point: x=%d, y=%d\n", point.x, point.y);
                fflush(stdout);
            }
            last_touch_check = current_tick;
        }
#endif
        
        /* Handle LVGL tasks - this may access DRM framebuffer */
        lv_timer_handler();
        
        /* Update progress bar if found (using global references) */
        if(g_bar && g_bar_label) {
            static uint32_t last_update = 0;
            uint32_t now = custom_tick_get();
            if(now - last_update > 100) {  /* Update every 100ms */
                update_progress_bar(g_bar, g_bar_label);
                last_update = now;
            }
        }
        
        /* Sleep for a short time */
        usleep(5000);  /* 5ms */
    }
    
    printf("\nExiting...\n");
    
    /* Cleanup */
    lv_deinit();
    
    return 0;
}


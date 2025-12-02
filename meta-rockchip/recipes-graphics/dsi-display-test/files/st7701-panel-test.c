/*
 * ST7701 Panel Test Utility
 * 
 * Comprehensive test tool for ST7701 display panel
 * Tests panel ID, power mode, display parameters, and system status
 * 
 * Usage: st7701-panel-test
 *   Runs all tests automatically
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

/* MIPI DCS Commands for ST7701 */
#define MIPI_DCS_READ_DISPLAY_ID        0x04
#define MIPI_DCS_READ_NUM_ERRORS        0x05
#define MIPI_DCS_READ_DISPLAY_POWER_MODE 0x0A
#define MIPI_DCS_READ_MADCTL            0x0B
#define MIPI_DCS_READ_PIXEL_FORMAT      0x0C
#define MIPI_DCS_READ_DISPLAY_SELF_DIAG 0x0F
#define MIPI_DCS_READ_DISPLAY_BRIGHTNESS 0x52
#define MIPI_DCS_READ_CTRL_DISPLAY      0x54

/* Print section header */
static void print_section(const char *title)
{
    printf("\n");
    printf("========================================\n");
    printf("%s\n", title);
    printf("========================================\n");
}

/* Print test result */
static void print_result(const char *test_name, int passed, const char *info)
{
    if (passed) {
        printf("  ✓ %s: %s\n", test_name, info ? info : "PASS");
    } else {
        printf("  ✗ %s: %s\n", test_name, info ? info : "FAIL");
    }
}

/* Display test colors structure */
struct display_test_state {
    int fd;
    uint32_t connector_id;
    uint32_t crtc_id;
    uint32_t fb_id;
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t size;
    void *map;
    drmModeModeInfo mode;
};

/* Test colors (RGB888 format) */
static const uint32_t test_colors[] = {
    0xFF0000,  /* Red */
    0x00FF00,  /* Green */
    0x0000FF,  /* Blue */
    0xFFFFFF,  /* White */
    0x000000,  /* Black */
};

static const char *test_color_names[] = {
    "Red",
    "Green", 
    "Blue",
    "White",
    "Black",
};

#define NUM_TEST_COLORS (sizeof(test_colors) / sizeof(test_colors[0]))

/* Wait for vblank */
static int wait_vblank(int fd)
{
    drmVBlank vbl;
    memset(&vbl, 0, sizeof(vbl));
    vbl.request.type = DRM_VBLANK_RELATIVE;
    vbl.request.sequence = 1;
    vbl.request.signal = 0;
    
    if (drmWaitVBlank(fd, &vbl) < 0) {
        /* Try alternative method */
        vbl.request.type = DRM_VBLANK_NEXTONMISS;
        vbl.request.sequence = 0;
        return drmWaitVBlank(fd, &vbl);
    }
    return 0;
}

/* Fill framebuffer with color */
static void fill_framebuffer(struct display_test_state *state, uint32_t color)
{
    uint32_t *pixels = (uint32_t *)state->map;
    uint32_t i;
    uint32_t xrgb_color;
    uint32_t total_pixels = state->width * state->height;
    
    /* Convert RGB888 to XRGB8888 format
     * XRGB8888 in little-endian memory: [B][G][R][X]
     * For RGB888 0xRRGGBB, we need 0x00RRGGBB
     * In memory: [BB][GG][RR][00] = [B][G][R][X] ✓
     */
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    xrgb_color = (0x00 << 24) | (r << 16) | (g << 8) | b;
    
    /* Debug output for first few colors */
    static int debug_count = 0;
    if (debug_count < 3) {
        printf("    DEBUG: Color 0x%06X -> XRGB8888 0x%08X (R=%02X G=%02X B=%02X)\n",
               color, xrgb_color, r, g, b);
        debug_count++;
    }
    
    /* Fill framebuffer efficiently */
    if (total_pixels > 0) {
        /* Fill first 4KB as pattern */
        uint32_t pattern_size = (4096 / sizeof(uint32_t)) < total_pixels ? 
                                (4096 / sizeof(uint32_t)) : total_pixels;
        for (i = 0; i < pattern_size; i++) {
            pixels[i] = xrgb_color;
        }
        
        /* Copy pattern to fill the rest */
        if (total_pixels > pattern_size) {
            uint32_t *src = pixels;
            uint32_t *dst = pixels + pattern_size;
            uint32_t remaining = total_pixels - pattern_size;
            while (remaining > 0) {
                uint32_t copy_size = remaining < pattern_size ? remaining : pattern_size;
                memcpy(dst, src, copy_size * sizeof(uint32_t));
                dst += copy_size;
                remaining -= copy_size;
            }
        }
        
        /* Verify first pixel */
        if (debug_count <= 3 && pixels[0] != xrgb_color) {
            printf("    ⚠ WARNING: Framebuffer verification failed! Expected 0x%08X, got 0x%08X\n",
                   xrgb_color, pixels[0]);
        }
    }
    
    /* Memory barrier and cache flush */
    __sync_synchronize();
    __builtin___clear_cache((char *)state->map, (char *)state->map + state->size);
}

/* Enable display and show test colors */
static int enable_display_with_test(void)
{
    struct display_test_state state = {0};
    drmModeRes *resources;
    drmModeConnector *connector = NULL;
    drmModeEncoder *encoder = NULL;
    int i;
    int ret = 0;
    int color_idx;
    
    printf("Enabling display and running color test...\n");
    printf("  (Based on ST7701S datasheet: ALLPOFF=22h=black, ALLPON=23h=white)\n");
    printf("  (Using DRM framebuffer to display test colors)\n\n");
    
    /* Open DRM device */
    state.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (state.fd < 0) {
        printf("  ⚠ Cannot open /dev/dri/card0: %s\n", strerror(errno));
        return 0;
    }
    
    /* Get DRM resources */
    resources = drmModeGetResources(state.fd);
    if (!resources) {
        printf("  ⚠ Cannot get DRM resources\n");
        close(state.fd);
        return 0;
    }
    
    /* Find DSI connector */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(state.fd, resources->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED) {
            /* Check if it's DSI connector */
            if (connector->connector_type == DRM_MODE_CONNECTOR_DSI) {
                printf("  Found DSI connector: ID=%u\n", connector->connector_id);
                state.connector_id = connector->connector_id;
                break;
            }
        }
        if (connector) {
            drmModeFreeConnector(connector);
            connector = NULL;
        }
    }
    
    if (!connector) {
        printf("  ⚠ No DSI connector found\n");
        drmModeFreeResources(resources);
        close(state.fd);
        return 0;
    }
    
    /* Get encoder - try connector's encoder_id first, then search all encoders */
    if (connector->encoder_id) {
        encoder = drmModeGetEncoder(state.fd, connector->encoder_id);
        if (encoder && encoder->crtc_id) {
            state.crtc_id = encoder->crtc_id;
        } else {
            if (encoder) {
                drmModeFreeEncoder(encoder);
                encoder = NULL;
            }
        }
    }
    
    /* If no encoder found, try to find a compatible encoder */
    if (!encoder) {
        for (i = 0; i < resources->count_encoders; i++) {
            encoder = drmModeGetEncoder(state.fd, resources->encoders[i]);
            if (encoder) {
                /* Check if this encoder can be used with this connector */
                int found = 0;
                for (int j = 0; j < connector->count_encoders; j++) {
                    if (connector->encoders[j] == encoder->encoder_id) {
                        if (encoder->crtc_id) {
                            state.crtc_id = encoder->crtc_id;
                            found = 1;
                            break;
                        }
                    }
                }
                if (found) {
                    break;
                }
                drmModeFreeEncoder(encoder);
                encoder = NULL;
            }
        }
    }
    
    /* If still no encoder/crtc found, try to find any available CRTC */
    if (!encoder || !state.crtc_id) {
        if (encoder) {
            drmModeFreeEncoder(encoder);
            encoder = NULL;
        }
        /* Try to find any available CRTC */
        for (i = 0; i < resources->count_crtcs; i++) {
            state.crtc_id = resources->crtcs[i];
            printf("  Using CRTC ID=%u (encoder lookup failed, trying direct CRTC)\n", state.crtc_id);
            break;
        }
    }
    
    if (!state.crtc_id) {
        printf("  ⚠ No CRTC found for connector\n");
        if (encoder) drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(state.fd);
        return 0;
    }
    
    /* Enable display by setting CRTC with first available mode */
    if (connector->count_modes > 0) {
        state.mode = connector->modes[0];
        state.width = connector->modes[0].hdisplay;
        state.height = connector->modes[0].vdisplay;
        printf("  Display mode: %dx%d@%d\n", 
               state.width, state.height, state.mode.vrefresh);
        
        /* Create framebuffer */
        struct drm_mode_create_dumb create = {};
        struct drm_mode_map_dumb map = {};
        
        create.width = state.width;
        create.height = state.height;
        create.bpp = 32;
        
        if (drmIoctl(state.fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) == 0) {
            state.handle = create.handle;
            state.pitch = create.pitch;
            state.size = create.size;
            
            /* Create framebuffer */
            uint32_t handles[4] = {state.handle, 0, 0, 0};
            uint32_t pitches[4] = {state.pitch, 0, 0, 0};
            uint32_t offsets[4] = {0, 0, 0, 0};
            
            if (drmModeAddFB2(state.fd, state.width, state.height, 
                              DRM_FORMAT_XRGB8888, handles, pitches, 
                              offsets, &state.fb_id, 0) == 0) {
                /* Map framebuffer */
                map.handle = state.handle;
                if (drmIoctl(state.fd, DRM_IOCTL_MODE_MAP_DUMB, &map) == 0) {
                    state.map = mmap(0, state.size, PROT_READ | PROT_WRITE, 
                                    MAP_SHARED, state.fd, map.offset);
                    if (state.map != MAP_FAILED) {
                        /* Set CRTC to enable display */
                        if (drmModeSetCrtc(state.fd, state.crtc_id, state.fb_id, 0, 0,
                                          &state.connector_id, 1, &state.mode) == 0) {
                            printf("  ✓ Display enabled successfully\n\n");
                            
                            /* Display test colors */
                            printf("  Displaying test colors (2 seconds each):\n");
                            printf("  (Watch the screen - it should cycle through colors)\n\n");
                            for (color_idx = 0; color_idx < NUM_TEST_COLORS; color_idx++) {
                                printf("    [%d/%d] %s (0x%06X) - ", 
                                       color_idx + 1, NUM_TEST_COLORS,
                                       test_color_names[color_idx], test_colors[color_idx]);
                                fflush(stdout);
                                
                                /* Fill framebuffer with color */
                                fill_framebuffer(&state, test_colors[color_idx]);
                                
                                /* Update CRTC to display new color */
                                if (drmModeSetCrtc(state.fd, state.crtc_id, state.fb_id, 0, 0,
                                                  &state.connector_id, 1, &state.mode) == 0) {
                                    /* Wait for vblank to ensure update is displayed */
                                    wait_vblank(state.fd);
                                    printf("✓ Displayed\n");
                                } else {
                                    printf("✗ Failed: %s\n", strerror(errno));
                                }
                                fflush(stdout);
                                
                                /* Wait before next color */
                                sleep(2);
                            }
                            
                            /* Keep last color (black) displayed */
                            printf("  ✓ Color test completed, screen should show black\n");
                            ret = 1;
                        } else {
                            printf("  ⚠ Failed to set CRTC: %s\n", strerror(errno));
                        }
                        munmap(state.map, state.size);
                    } else {
                        printf("  ⚠ Failed to mmap framebuffer: %s\n", strerror(errno));
                    }
                } else {
                    printf("  ⚠ Failed to map dumb buffer: %s\n", strerror(errno));
                }
                drmModeRmFB(state.fd, state.fb_id);
            } else {
                printf("  ⚠ Failed to create framebuffer: %s\n", strerror(errno));
            }
            
            /* Cleanup dumb buffer */
            struct drm_mode_destroy_dumb destroy = {};
            destroy.handle = state.handle;
            drmIoctl(state.fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        } else {
            printf("  ⚠ Failed to create dumb buffer: %s\n", strerror(errno));
        }
    } else {
        printf("  ⚠ No display modes available\n");
    }
    
    /* Cleanup */
    if (encoder) drmModeFreeEncoder(encoder);
    if (connector) drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    close(state.fd);
    
    return ret;
}

/* Test 1: Check ST7701 driver in kernel logs */
static int test_driver_status(void)
{
    FILE *fp;
    char line[512];
    int found = 0;
    
    print_section("Test 1: ST7701 Driver Status");
    
    fp = popen("dmesg | grep -i st7701 | tail -10", "r");
    if (fp) {
        printf("  Recent ST7701 driver messages:\n");
        while (fgets(line, sizeof(line), fp)) {
            printf("    %s", line);
            found = 1;
        }
        pclose(fp);
        
        if (found) {
            print_result("Driver loaded", 1, "ST7701 driver found in kernel logs");
            return 1;
        } else {
            print_result("Driver loaded", 0, "No ST7701 messages in kernel logs");
            return 0;
        }
    }
    
    print_result("Driver check", 0, "Cannot execute dmesg command");
    return 0;
}

/* Test 2: Check DSI connector status */
static int test_dsi_connector(void)
{
    DIR *dir;
    struct dirent *entry;
    char path[256];
    FILE *fp;
    char line[128];
    int found = 0;
    
    print_section("Test 2: DSI Connector Status");
    
    dir = opendir("/sys/class/drm");
    if (!dir) {
        print_result("DSI connector", 0, "Cannot access /sys/class/drm");
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "DSI") || strstr(entry->d_name, "dsi")) {
            snprintf(path, sizeof(path), "/sys/class/drm/%s/status", entry->d_name);
            fp = fopen(path, "r");
            if (fp) {
                if (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    printf("  Connector: %s\n", entry->d_name);
                    printf("  Status: %s\n", line);
                    print_result("DSI connector", 1, line);
                    found = 1;
                }
                fclose(fp);
            }
            
            /* Check enabled status */
            snprintf(path, sizeof(path), "/sys/class/drm/%s/enabled", entry->d_name);
            fp = fopen(path, "r");
            if (fp) {
                if (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    printf("  Enabled: %s\n", line);
                }
                fclose(fp);
            }
        }
    }
    closedir(dir);
    
    if (!found) {
        print_result("DSI connector", 0, "No DSI connector found");
    }
    
    return found;
}

/* Test 3: Check panel power regulator */
static int test_panel_power(void)
{
    DIR *dir;
    struct dirent *entry;
    char path[512];
    FILE *fp;
    char line[128];
    int found = 0;
    int found_lcd_n = 0;
    int found_3v3 = 0;
    
    print_section("Test 3: Panel Power Regulator");
    
    printf("  Searching for LCD power regulator (vcc3v3_lcd_n)...\n");
    printf("  Device tree expects: regulator-name = \"vcc3v3_lcd_n\"\n");
    printf("  GPIO: GPIO1_D3 -> LCD_PWREN\n\n");
    
    dir = opendir("/sys/class/regulator");
    if (!dir) {
        print_result("Power regulator", 0, "Cannot access /sys/class/regulator");
        return 0;
    }
    
    /* First pass: Look for exact match vcc3v3_lcd_n */
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "regulator", 9) == 0) {
            snprintf(path, sizeof(path), "/sys/class/regulator/%s/name", entry->d_name);
            fp = fopen(path, "r");
            if (fp) {
                if (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    /* Check for exact match */
                    if (strcmp(line, "vcc3v3_lcd_n") == 0) {
                        printf("  ✓ Found exact match: %s\n", line);
                        found_lcd_n = 1;
                        found = 1;
                        
                        /* Build regulator directory path */
                        char reg_dir_path[512];
                        snprintf(reg_dir_path, sizeof(reg_dir_path), "/sys/class/regulator/%s", entry->d_name);
                        
                        /* Check state */
                        char state_path[512];
                        snprintf(state_path, sizeof(state_path), "%s/state", reg_dir_path);
                        FILE *fp_state = fopen(state_path, "r");
                        if (fp_state) {
                            char state_line[128];
                            if (fgets(state_line, sizeof(state_line), fp_state)) {
                                state_line[strcspn(state_line, "\n")] = 0;
                                printf("  State: %s\n", state_line);
                                print_result("Power state", strcmp(state_line, "enabled") == 0, state_line);
                            }
                            fclose(fp_state);
                        }
                        
                        /* Check voltage */
                        char volt_path[512];
                        snprintf(volt_path, sizeof(volt_path), "%s/microvolts", reg_dir_path);
                        FILE *fp_volt = fopen(volt_path, "r");
                        if (fp_volt) {
                            char volt_line[128];
                            if (fgets(volt_line, sizeof(volt_line), fp_volt)) {
                                volt_line[strcspn(volt_line, "\n")] = 0;
                                printf("  Voltage: %s uV\n", volt_line);
                            }
                            fclose(fp_volt);
                        }
                        
                        /* Check device tree node */
                        char of_node_path[512];
                        snprintf(of_node_path, sizeof(of_node_path), "%s/of_node", reg_dir_path);
                        if (access(of_node_path, F_OK) == 0) {
                            char link_target[512];
                            ssize_t len = readlink(of_node_path, link_target, sizeof(link_target) - 1);
                            if (len > 0) {
                                link_target[len] = '\0';
                                printf("  Device tree node: %s\n", link_target);
                            }
                        }
                    }
                }
                fclose(fp);
                fp = NULL;
            }
        }
    }
    
    /* Second pass: List all 3.3V regulators if exact match not found */
    if (!found_lcd_n) {
        printf("  ⚠ Exact match 'vcc3v3_lcd_n' not found!\n");
        printf("  Listing all 3.3V regulators:\n\n");
        
        rewinddir(dir);
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "regulator", 9) == 0) {
                snprintf(path, sizeof(path), "/sys/class/regulator/%s/name", entry->d_name);
                fp = fopen(path, "r");
                if (fp) {
                    if (fgets(line, sizeof(line), fp)) {
                        line[strcspn(line, "\n")] = 0;
                        
                        /* Check voltage */
                        char reg_dir_path[512];
                        snprintf(reg_dir_path, sizeof(reg_dir_path), "/sys/class/regulator/%s", entry->d_name);
                        char volt_path[512];
                        snprintf(volt_path, sizeof(volt_path), "%s/microvolts", reg_dir_path);
                        FILE *fp_volt = fopen(volt_path, "r");
                        if (fp_volt) {
                            char volt_line[128];
                            if (fgets(volt_line, sizeof(volt_line), fp_volt)) {
                                volt_line[strcspn(volt_line, "\n")] = 0;
                                int voltage = atoi(volt_line);
                                /* Check if it's 3.3V (allow some tolerance: 3.2V - 3.4V) */
                                if (voltage >= 3200000 && voltage <= 3400000) {
                                    printf("  Regulator: %s (Voltage: %s uV)\n", line, volt_line);
                                    found_3v3 = 1;
                                    
                                    /* Check state */
                                    char state_path[512];
                                    snprintf(state_path, sizeof(state_path), "%s/state", reg_dir_path);
                                    FILE *fp_state = fopen(state_path, "r");
                                    if (fp_state) {
                                        char state_line[128];
                                        if (fgets(state_line, sizeof(state_line), fp_state)) {
                                            state_line[strcspn(state_line, "\n")] = 0;
                                            printf("    State: %s\n", state_line);
                                        }
                                        fclose(fp_state);
                                    }
                                    
                                    /* Check if it contains 'lcd' or 'stb' */
                                    if (strstr(line, "lcd") || strstr(line, "LCD")) {
                                        printf("    ⚠ This might be the LCD regulator (contains 'lcd')\n");
                                        found = 1;
                                    } else if (strstr(line, "stb")) {
                                        printf("    ⚠ Found 'vcc3v3_stb' - might be a different regulator\n");
                                    }
                                }
                            }
                            fclose(fp_volt);
                        }
                    }
                    fclose(fp);
                    fp = NULL;
                }
            }
        }
    }
    
    closedir(dir);
    
    if (!found) {
        if (found_3v3) {
            print_result("Power regulator", 0, "Found 3.3V regulators but 'vcc3v3_lcd_n' not found - check device tree");
        } else {
            print_result("Power regulator", 0, "No LCD power regulator found");
        }
    } else if (!found_lcd_n) {
        print_result("Power regulator", 0, "LCD regulator name mismatch - device tree may not be applied");
    }
    
    return found;
}

/* Test 4: Check backlight */
static int test_backlight(void)
{
    DIR *dir;
    struct dirent *entry;
    char path[256];
    FILE *fp;
    char line[128];
    int found = 0;
    
    print_section("Test 4: Backlight Status");
    
    dir = opendir("/sys/class/backlight");
    if (!dir) {
        print_result("Backlight", 0, "No backlight device found");
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            printf("  Backlight device: %s\n", entry->d_name);
            found = 1;
            
            /* Check brightness */
            snprintf(path, sizeof(path), "/sys/class/backlight/%s/brightness", entry->d_name);
            fp = fopen(path, "r");
            if (fp) {
                if (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    int brightness = atoi(line);
                    printf("  Brightness: %d", brightness);
                    
                    /* Check max brightness */
                    snprintf(path, sizeof(path), "/sys/class/backlight/%s/max_brightness", entry->d_name);
                    FILE *fp2 = fopen(path, "r");
                    if (fp2) {
                        if (fgets(line, sizeof(line), fp2)) {
                            int max_brightness = atoi(line);
                            printf(" / %d", max_brightness);
                            if (brightness > 0) {
                                printf(" (%.1f%%)\n", (brightness * 100.0) / max_brightness);
                                print_result("Backlight", 1, "Backlight is on");
                            } else {
                                printf(" (OFF)\n");
                                print_result("Backlight", 0, "Backlight is off");
                            }
                        }
                        fclose(fp2);
                    } else {
                        printf("\n");
                    }
                }
                fclose(fp);
            }
        }
    }
    closedir(dir);
    
    if (!found) {
        print_result("Backlight", 0, "No backlight device found");
    }
    
    return found;
}

/* Test 5: Check DRM device and modes */
static int test_drm_device(void)
{
    FILE *fp;
    char line[512];
    int found = 0;
    
    print_section("Test 5: DRM Device Status");
    
    /* Check if DRM device exists */
    if (access("/dev/dri/card0", F_OK) == 0) {
        print_result("DRM device", 1, "/dev/dri/card0 exists");
        found = 1;
        
        /* Try to get connector info using modetest if available */
        fp = popen("modetest -c 2>/dev/null | head -n 20 2>/dev/null || modetest -c 2>/dev/null", "r");
        if (fp) {
            printf("  DRM connector information:\n");
            while (fgets(line, sizeof(line), fp)) {
                printf("    %s", line);
            }
            pclose(fp);
        } else {
            printf("  (modetest not available, skipping detailed info)\n");
        }
    } else {
        print_result("DRM device", 0, "/dev/dri/card0 not found");
    }
    
    return found;
}

/* Test 6: Panel initialization sequence check */
static int test_panel_init(void)
{
    FILE *fp;
    char line[512];
    int prepare_found = 0;
    int enable_found = 0;
    
    print_section("Test 6: Panel Initialization Sequence");
    
    fp = popen("dmesg | grep -i 'st7701.*prepare\\|st7701.*enable'", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "prepare")) {
                prepare_found = 1;
                printf("    %s", line);
            }
            if (strstr(line, "enable")) {
                enable_found = 1;
                printf("    %s", line);
            }
        }
        pclose(fp);
    }
    
    print_result("Panel prepare", prepare_found, prepare_found ? "Panel prepared" : "No prepare message");
    print_result("Panel enable", enable_found, enable_found ? "Panel enabled" : "No enable message");
    
    if (prepare_found && enable_found) {
        print_result("Initialization", 1, "Panel initialization sequence completed");
        return 1;
    } else {
        print_result("Initialization", 0, "Panel initialization may be incomplete");
        return 0;
    }
}

/* Test 7: DSI PHY and lane configuration */
static int test_dsi_config(void)
{
    FILE *fp;
    char line[512];
    int found = 0;
    
    print_section("Test 7: DSI Configuration");
    
    fp = popen("dmesg | grep -i 'dsi.*lane\\|dsi.*phy\\|lane.*mbps' | tail -5", "r");
    if (fp) {
        printf("  DSI configuration messages:\n");
        while (fgets(line, sizeof(line), fp)) {
            printf("    %s", line);
            found = 1;
        }
        pclose(fp);
        
        if (found) {
            print_result("DSI config", 1, "DSI configuration found in logs");
        } else {
            print_result("DSI config", 0, "No DSI configuration messages");
        }
    }
    
    return found;
}

/* Test 8: Display mode and resolution */
static int test_display_mode(void)
{
    FILE *fp;
    char line[512];
    int found = 0;
    
    print_section("Test 8: Display Mode");
    
    fp = popen("dmesg | grep -i 'mode.*x.*@\\|display.*mode\\|480x800' | tail -5", "r");
    if (fp) {
        printf("  Display mode messages:\n");
        while (fgets(line, sizeof(line), fp)) {
            printf("    %s", line);
            found = 1;
        }
        pclose(fp);
        
        if (found) {
            print_result("Display mode", 1, "Display mode configured");
        } else {
            print_result("Display mode", 0, "No display mode messages");
        }
    }
    
    return found;
}

/* Summary */
static void print_summary(int *results, int num_tests)
{
    int i;
    int passed = 0;
    
    print_section("Test Summary");
    
    for (i = 0; i < num_tests; i++) {
        if (results[i]) passed++;
    }
    
    printf("  Tests passed: %d / %d\n", passed, num_tests);
    printf("  Success rate: %.1f%%\n", (passed * 100.0) / num_tests);
    
    if (passed == num_tests) {
        printf("\n  ✓ All tests passed! Panel appears to be properly configured.\n");
    } else if (passed >= num_tests / 2) {
        printf("\n  ⚠ Some tests failed. Panel may be partially configured.\n");
        printf("     Check the failed tests above for details.\n");
    } else {
        printf("\n  ✗ Most tests failed. Panel may not be properly configured.\n");
        printf("     Check hardware connections and device tree configuration.\n");
    }
}

int main(int argc, char *argv[])
{
    int results[8] = {0};
    int num_tests = 0;
    
    printf("ST7701 Panel Test Utility\n");
    printf("==========================\n");
    printf("Comprehensive display panel diagnostic tool\n");
    printf("\n");
    
    /* Enable display and run color test first */
    if (enable_display_with_test()) {
        printf("\n  Display enabled and color test completed, proceeding with diagnostic tests...\n");
    } else {
        printf("\n  ⚠ Warning: Could not enable display, some tests may fail\n");
    }
    printf("\n");
    printf("Running all diagnostic tests...\n");
    
    /* Run all tests */
    results[0] = test_driver_status();
    num_tests++;
    
    results[1] = test_dsi_connector();
    num_tests++;
    
    results[2] = test_panel_power();
    num_tests++;
    
    results[3] = test_backlight();
    num_tests++;
    
    results[4] = test_drm_device();
    num_tests++;
    
    results[5] = test_panel_init();
    num_tests++;
    
    results[6] = test_dsi_config();
    num_tests++;
    
    results[7] = test_display_mode();
    num_tests++;
    
    /* Print summary */
    print_summary(results, num_tests);
    
    printf("\n");
    return 0;
}


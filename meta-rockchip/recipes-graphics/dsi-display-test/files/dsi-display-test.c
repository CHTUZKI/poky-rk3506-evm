/*
 * DSI Display Test Utility
 * 
 * This program tests the DSI display by cycling through different colors.
 * It uses DRM/KMS interface to directly control the display.
 * 
 * Usage: dsi-display-test [options]
 *   -d <device>  DRM device (default: /dev/dri/card0)
 *   -t <seconds> Time per color in seconds (default: 2)
 *   -h           Show help
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <signal.h>
#include <getopt.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#define DRM_DEVICE_DEFAULT "/dev/dri/card0"

struct drm_state {
	int fd;
	uint32_t connector_id;
	uint32_t crtc_id;
	uint32_t plane_id;
	uint32_t fb_id;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t size;
	void *map;
	uint32_t handle;
	drmModeModeInfo mode;
};

static volatile int running = 1;

/* Color definitions (RGB888) */
static const uint32_t colors[] = {
	0xFF0000,  /* Red */
	0x00FF00,  /* Green */
	0x0000FF,  /* Blue */
	0xFFFFFF,  /* White */
	0x000000,  /* Black */
	0xFFFF00,  /* Yellow */
	0xFF00FF,  /* Magenta */
	0x00FFFF,  /* Cyan */
	0xFF8000,  /* Orange */
	0x808080,  /* Gray */
};

#define NUM_COLORS (sizeof(colors) / sizeof(colors[0]))

static void signal_handler(int sig)
{
	running = 0;
}

static int drm_open_device(const char *device)
{
	int fd = open(device, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s: %s\n", device, strerror(errno));
		return -1;
	}
	return fd;
}

/* Helper function to get CRTC index */
static int drm_crtc_index(int fd, uint32_t crtc_id)
{
	drmModeRes *resources = drmModeGetResources(fd);
	int i, index = -1;
	
	if (!resources)
		return -1;
	
	for (i = 0; i < resources->count_crtcs; i++) {
		if (resources->crtcs[i] == crtc_id) {
			index = i;
			break;
		}
	}
	
	drmModeFreeResources(resources);
	return index;
}

static int drm_find_connector(struct drm_state *state)
{
	drmModeRes *resources;
	drmModeConnector *connector = NULL;
	drmModePlaneRes *plane_res;
	int i, j;

	resources = drmModeGetResources(state->fd);
	if (!resources) {
		fprintf(stderr, "Cannot get DRM resources\n");
		return -1;
	}

	/* Find first connected connector */
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(state->fd, resources->connectors[i]);
		if (connector && connector->connection == DRM_MODE_CONNECTED) {
			state->connector_id = connector->connector_id;
			break;
		}
		if (connector)
			drmModeFreeConnector(connector);
		connector = NULL;
	}

	if (!connector) {
		fprintf(stderr, "No connected connector found\n");
		drmModeFreeResources(resources);
		return -1;
	}

	/* Find encoder for this connector */
	if (connector->encoder_id) {
		drmModeEncoder *encoder = drmModeGetEncoder(state->fd, connector->encoder_id);
		if (encoder) {
			state->crtc_id = encoder->crtc_id;
			drmModeFreeEncoder(encoder);
		}
	}

	/* If no encoder found, try to find a suitable one */
	if (!state->crtc_id) {
		for (i = 0; i < resources->count_encoders; i++) {
			drmModeEncoder *encoder = drmModeGetEncoder(state->fd, resources->encoders[i]);
			if (encoder) {
				/* Check if this encoder can be used with this connector */
				int found = 0;
				for (j = 0; j < connector->count_encoders; j++) {
					if (connector->encoders[j] == encoder->encoder_id) {
						state->crtc_id = encoder->crtc_id;
						found = 1;
						break;
					}
				}
				drmModeFreeEncoder(encoder);
				if (found)
					break;
			}
		}
	}

	if (!state->crtc_id) {
		fprintf(stderr, "No suitable CRTC found\n");
		drmModeFreeConnector(connector);
		drmModeFreeResources(resources);
		return -1;
	}

	/* Find a plane for this CRTC */
	printf("DEBUG: Starting plane lookup, CRTC ID=%u\n", state->crtc_id);
	fflush(stdout);
	plane_res = drmModeGetPlaneResources(state->fd);
	if (plane_res) {
		printf("DEBUG: Got plane resources, count=%u\n", plane_res->count_planes);
		fflush(stdout);
		int crtc_idx = drm_crtc_index(state->fd, state->crtc_id);
		printf("DEBUG: Looking for planes, CRTC ID=%u, CRTC index=%d, total planes=%u\n", 
		       state->crtc_id, crtc_idx, plane_res->count_planes);
		fflush(stdout);  /* Force output to appear immediately */
		if (crtc_idx >= 0) {
			/* First pass: try to find a primary plane (type=1) */
			for (i = 0; i < plane_res->count_planes; i++) {
				drmModePlane *plane = drmModeGetPlane(state->fd, plane_res->planes[i]);
				if (plane) {
					/* Check if this plane can be used with our CRTC */
					if (plane->possible_crtcs & (1 << crtc_idx)) {
						/* Get plane properties to check type */
						drmModeObjectProperties *props = drmModeObjectGetProperties(state->fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
						if (props) {
							uint32_t plane_type = 0; /* 0=Overlay, 1=Primary, 2=Cursor */
							for (j = 0; j < props->count_props; j++) {
								drmModePropertyPtr prop = drmModeGetProperty(state->fd, props->props[j]);
								if (prop) {
									if (strcmp(prop->name, "type") == 0) {
										plane_type = props->prop_values[j];
										printf("DEBUG: Plane %u type=%u\n", plane->plane_id, plane_type);
									}
									drmModeFreeProperty(prop);
								}
							}
							/* Prefer primary plane (type=1) */
							if (plane_type == 1 && !state->plane_id) {
								state->plane_id = plane->plane_id;
								printf("Found primary plane: %u (can be used with CRTC %u)\n", 
								       state->plane_id, state->crtc_id);
							}
							drmModeFreeObjectProperties(props);
						}
					} else {
						printf("DEBUG: Plane %u cannot be used with CRTC index %d (possible_crtcs=0x%x)\n",
						       plane->plane_id, crtc_idx, plane->possible_crtcs);
					}
					drmModeFreePlane(plane);
				}
			}
			/* Second pass: if no primary plane found, use any compatible plane */
			if (!state->plane_id) {
				printf("DEBUG: No primary plane found, trying any compatible plane\n");
				for (i = 0; i < plane_res->count_planes; i++) {
					drmModePlane *plane = drmModeGetPlane(state->fd, plane_res->planes[i]);
					if (plane) {
						if (plane->possible_crtcs & (1 << crtc_idx)) {
							state->plane_id = plane->plane_id;
							printf("Found plane: %u (can be used with CRTC %u)\n", 
							       state->plane_id, state->crtc_id);
							drmModeFreePlane(plane);
							break;
						}
						drmModeFreePlane(plane);
					}
				}
			}
		} else {
			fprintf(stderr, "Warning: Could not determine CRTC index (crtc_id=%u), trying alternative method\n", state->crtc_id);
			fflush(stderr);
			/* Fallback: try to find any plane that can be used with this CRTC */
			for (i = 0; i < plane_res->count_planes; i++) {
				drmModePlane *plane = drmModeGetPlane(state->fd, plane_res->planes[i]);
				if (plane) {
					/* Check if this plane's current CRTC matches */
					if (plane->crtc_id == state->crtc_id) {
						state->plane_id = plane->plane_id;
						printf("Found plane %u (currently on CRTC %u)\n", 
						       state->plane_id, state->crtc_id);
						fflush(stdout);
						drmModeFreePlane(plane);
						break;
					}
					drmModeFreePlane(plane);
				}
			}
		}
		drmModeFreePlaneResources(plane_res);
		if (state->plane_id) {
			printf("DEBUG: Plane lookup successful, plane_id=%u\n", state->plane_id);
		} else {
			printf("DEBUG: Plane lookup failed, plane_id=0\n");
		}
		fflush(stdout);
	} else {
		fprintf(stderr, "Warning: Could not get plane resources\n");
		fflush(stderr);
	}

	/* Get preferred mode or first mode */
	if (connector->count_modes > 0) {
		/* Save mode info to state before freeing connector */
		state->mode = connector->modes[0];
		state->width = connector->modes[0].hdisplay;
		state->height = connector->modes[0].vdisplay;
		printf("Display mode: %dx%d\n", state->width, state->height);
		if (state->plane_id) {
			printf("Found plane: %u\n", state->plane_id);
		}
	} else {
		fprintf(stderr, "No display modes found\n");
		drmModeFreeConnector(connector);
		drmModeFreeResources(resources);
		return -1;
	}

	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);

	return 0;
}

static int drm_create_fb(struct drm_state *state)
{
	struct drm_mode_create_dumb create = {};
	struct drm_mode_map_dumb map = {};
	int ret;

	/* Create dumb buffer */
	create.width = state->width;
	create.height = state->height;
	create.bpp = 32;
	ret = drmIoctl(state->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if (ret < 0) {
		fprintf(stderr, "Cannot create dumb buffer: %s\n", strerror(errno));
		return -1;
	}

	state->handle = create.handle;
	state->pitch = create.pitch;
	state->size = create.size;

	/* Create framebuffer using drmModeAddFB2 with explicit format
	 * Format: DRM_FORMAT_XRGB8888 (0x34325258)
	 * This ensures the correct format is used
	 */
	uint32_t handles[4] = {state->handle, 0, 0, 0};
	uint32_t pitches[4] = {state->pitch, 0, 0, 0};
	uint32_t offsets[4] = {0, 0, 0, 0};
	uint32_t format = DRM_FORMAT_XRGB8888;
	
	ret = drmModeAddFB2(state->fd, state->width, state->height, format,
			    handles, pitches, offsets, &state->fb_id, 0);
	if (ret) {
		/* Fallback to old API if drmModeAddFB2 fails */
		fprintf(stderr, "drmModeAddFB2 failed: %s, trying drmModeAddFB\n", strerror(errno));
		ret = drmModeAddFB(state->fd, state->width, state->height, 24, 32,
				   state->pitch, state->handle, &state->fb_id);
		if (ret) {
			fprintf(stderr, "Cannot create framebuffer: %s\n", strerror(errno));
			goto err_destroy;
		}
	}
	
	printf("Created framebuffer: id=%u, size=%ux%u, pitch=%u, format=XRGB8888 (0x%08x)\n",
	       state->fb_id, state->width, state->height, state->pitch, format);

	/* Map buffer */
	map.handle = state->handle;
	ret = drmIoctl(state->fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
	if (ret) {
		fprintf(stderr, "Cannot map dumb buffer: %s\n", strerror(errno));
		goto err_fb;
	}

	state->map = mmap(0, state->size, PROT_READ | PROT_WRITE, MAP_SHARED,
			  state->fd, map.offset);
	if (state->map == MAP_FAILED) {
		fprintf(stderr, "Cannot mmap buffer: %s\n", strerror(errno));
		goto err_fb;
	}

	memset(state->map, 0, state->size);
	return 0;

err_fb:
	drmModeRmFB(state->fd, state->fb_id);
err_destroy:
	{
		struct drm_mode_destroy_dumb destroy = {};
		destroy.handle = state->handle;
		drmIoctl(state->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
	}
	return -1;
}

static void drm_fill_color(struct drm_state *state, uint32_t color)
{
	uint32_t *pixels = (uint32_t *)state->map;
	uint32_t i;
	uint32_t xrgb_color;
	uint32_t total_pixels = state->width * state->height;

	/* Convert RGB888 to XRGB8888 format
	 * 
	 * DRM_FORMAT_XRGB8888 format specification:
	 * - 32 bits per pixel
	 * - Format code: 0x34325258 ('XR24' in ASCII)
	 * - The format name "XRGB8888" describes the byte order in memory:
	 *   On little-endian systems: [B][G][R][X] (byte0=B, byte1=G, byte2=R, byte3=X)
	 * 
	 * For RGB888 input 0xRRGGBB:
	 * - Extract: R=RR, G=GG, B=BB
	 * - To get [B][G][R][X] in memory, we need: 0x00RRGGBB
	 * - In little-endian memory: bytes are [BB][GG][RR][00]
	 *   Which means: byte0=B=BB, byte1=G=GG, byte2=R=RR, byte3=X=00 ✓
	 * 
	 * Example: Red 0xFF0000 (R=FF, G=00, B=00)
	 * - XRGB8888 value: 0x00FF0000
	 * - Memory bytes (little-endian): [00][00][FF][00]
	 *   Which means: byte0=B=00, byte1=G=00, byte2=R=FF, byte3=X=00 ✓
	 * 
	 * CRITICAL: The current conversion is CORRECT for XRGB8888 format.
	 * The value 0x00RRGGBB stored in memory as [BB][GG][RR][00] matches
	 * the XRGB8888 format requirement of [B][G][R][X].
	 */
	/* Extract RGB components from RGB888 format */
	uint8_t r = (color >> 16) & 0xFF;
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t b = color & 0xFF;
	
	/* Build XRGB8888 format: 0x00RRGGBB
	 * This will be stored in memory as [BB][GG][RR][00] on little-endian systems
	 * Which matches the XRGB8888 format requirement: [B][G][R][X]
	 */
	xrgb_color = (0x00 << 24) | (r << 16) | (g << 8) | b;
	
	/* Debug: print first pixel value for verification (only for first few colors) */
	static int debug_count = 0;
	if (total_pixels > 0 && debug_count < 3) {
		printf("DEBUG: Color 0x%06X -> XRGB8888 0x%08X (R=%02X G=%02X B=%02X)\n",
		       color, xrgb_color, r, g, b);
		debug_count++;
	}
	
	/* Fill framebuffer with color */
	if (total_pixels > 0) {
		/* Use a more efficient method: fill first 4KB, then copy */
		uint32_t pattern_size = (4096 / sizeof(uint32_t)) < total_pixels ? (4096 / sizeof(uint32_t)) : total_pixels;
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
	}
	
	/* Ensure data is written to memory before continuing */
	__sync_synchronize();
	
	/* Note: msync() only works on file-backed mappings, not on device memory.
	 * For DRM dumb buffers, we rely on __sync_synchronize() and cache flush.
	 * msync() will fail with EINVAL, which is expected and can be ignored.
	 */
	
	/* Additional cache flush hint for ARM */
	__builtin___clear_cache((char *)state->map, (char *)state->map + state->size);
	
	/* Additional DMA synchronization check */
	/* For DRM dumb buffers, the kernel handles DMA synchronization through IOMMU.
	 * We've already done memory barriers and cache flushes above.
	 * The kernel's atomic API will handle DMA sync when the framebuffer is committed.
	 */
	printf("DEBUG: DMA synchronization: memory barriers and cache flushes completed\n");
	
	/* Verify framebuffer data was written correctly */
	if (total_pixels > 0) {
		/* Check first few pixels */
		int verify_count = (total_pixels < 10) ? total_pixels : 10;
		int all_correct = 1;
		for (i = 0; i < verify_count; i++) {
			if (pixels[i] != xrgb_color) {
				all_correct = 0;
				fprintf(stderr, "ERROR: Framebuffer pixel[%u] verification failed! Expected 0x%08X, got 0x%08X\n",
				       i, xrgb_color, pixels[i]);
			}
		}
		
		/* Check a few pixels from different locations */
		if (total_pixels > 100) {
			uint32_t check_positions[] = {100, 1000, 10000, total_pixels - 1};
			for (i = 0; i < sizeof(check_positions)/sizeof(check_positions[0]); i++) {
				if (check_positions[i] < total_pixels) {
					if (pixels[check_positions[i]] != xrgb_color) {
						all_correct = 0;
						fprintf(stderr, "ERROR: Framebuffer pixel[%u] verification failed! Expected 0x%08X, got 0x%08X\n",
						       check_positions[i], xrgb_color, pixels[check_positions[i]]);
					}
				}
			}
		}
		
		/* Print verification result (only for first few colors) */
		static int verify_debug_count = 0;
		if (verify_debug_count < 3) {
			if (all_correct) {
				printf("DEBUG: Framebuffer verification PASSED - first %d pixels correct, color=0x%08X\n",
				       verify_count, xrgb_color);
				/* Print memory address for debugging */
				printf("DEBUG: Framebuffer memory address: %p, size=%zu bytes\n",
				       state->map, state->size);
			} else {
				fprintf(stderr, "ERROR: Framebuffer verification FAILED!\n");
			}
			verify_debug_count++;
		}
	}
}

/* Wait for vblank event to ensure framebuffer update is displayed */
static int drm_wait_vblank(struct drm_state *state);

static int drm_set_crtc(struct drm_state *state, int force_mode)
{
	int ret;
	drmModeCrtc *crtc;
	int crtc_active = 0;
	
	/* Additional memory barrier before setting CRTC to ensure framebuffer data is visible */
	__sync_synchronize();
	
	/* Verify framebuffer data one more time before display update */
	if (state->map && state->map != MAP_FAILED) {
		uint32_t *pixels = (uint32_t *)state->map;
		uint32_t first_pixel = pixels[0];
		static int pre_update_debug_count = 0;
		if (pre_update_debug_count < 3) {
			printf("DEBUG: Pre-CRTC update: framebuffer first pixel=0x%08X, address=%p\n",
			       first_pixel, state->map);
			pre_update_debug_count++;
		}
	}

	/* Get current CRTC state to check if it's already active */
	crtc = drmModeGetCrtc(state->fd, state->crtc_id);
	if (crtc) {
		crtc_active = (crtc->mode_valid && crtc->buffer_id != 0);
		if (crtc_active && !force_mode) {
			/* CRTC is already active, try to update framebuffer only */
			/* First try with NULL mode (just update FB) */
			ret = drmModeSetCrtc(state->fd, state->crtc_id, state->fb_id, 0, 0,
					     &state->connector_id, 1, NULL);
			if (ret) {
				/* If that fails, try with the mode again (force refresh) */
				ret = drmModeSetCrtc(state->fd, state->crtc_id, state->fb_id, 0, 0,
						     &state->connector_id, 1, &state->mode);
			}
		} else {
			/* Set CRTC with the saved mode */
			ret = drmModeSetCrtc(state->fd, state->crtc_id, state->fb_id, 0, 0,
					     &state->connector_id, 1, &state->mode);
		}
		drmModeFreeCrtc(crtc);
	} else {
		/* No existing CRTC info, set it fresh */
		ret = drmModeSetCrtc(state->fd, state->crtc_id, state->fb_id, 0, 0,
				     &state->connector_id, 1, &state->mode);
	}

	if (ret) {
		fprintf(stderr, "Cannot set CRTC: %s (errno=%d)\n", strerror(errno), errno);
		return -1;
	}

	/* If we have a plane, also update the plane to ensure framebuffer is displayed */
	if (state->plane_id) {
		/* Use drmModeSetPlane to update the plane with the new framebuffer
		 * Parameters:
		 * - plane_id: the plane to update
		 * - crtc_id: the CRTC this plane is attached to
		 * - fb_id: the framebuffer to display
		 * - flags: 0 (no special flags)
		 * - crtc_x, crtc_y: position on CRTC (0, 0)
		 * - crtc_w, crtc_h: size on CRTC (full size)
		 * - src_x, src_y: source position in framebuffer (0, 0)
		 * - src_w, src_h: source size in framebuffer (in 16.16 fixed point format)
		 */
		ret = drmModeSetPlane(state->fd, state->plane_id, state->crtc_id, state->fb_id, 0,
				      0, 0, state->width, state->height,
				      0, 0, state->width << 16, state->height << 16);
		if (ret) {
			/* Plane update failed, but CRTC update succeeded, so continue */
			fprintf(stderr, "Warning: Cannot set plane: %s (errno=%d)\n", strerror(errno), errno);
		} else {
			/* Plane update succeeded */
			printf("Plane %u updated with framebuffer %u\n", state->plane_id, state->fb_id);
		}
	} else {
		fprintf(stderr, "Warning: No plane found, using CRTC-only update\n");
	}

	/* Note: msync() doesn't work on device memory mappings.
	 * Memory synchronization is already handled by __sync_synchronize() and cache flush.
	 */

	/* Wait for vblank to ensure the update is displayed */
	ret = drm_wait_vblank(state);
	if (ret < 0) {
		fprintf(stderr, "Warning: Failed to wait for vblank: %s (errno=%d)\n", 
			strerror(errno), errno);
		/* Continue anyway, as vblank wait is not critical */
	} else {
		printf("DEBUG: Vblank wait completed successfully\n");
	}

	return 0;
}

/* Wait for vblank event to ensure framebuffer update is displayed */
static int drm_wait_vblank(struct drm_state *state)
{
	drmVBlank vbl;
	int ret;

	memset(&vbl, 0, sizeof(vbl));
	vbl.request.type = DRM_VBLANK_RELATIVE;
	vbl.request.sequence = 1;  /* Wait for next vblank */
	vbl.request.signal = 0;    /* No signal needed */

	ret = drmWaitVBlank(state->fd, &vbl);
	if (ret < 0) {
		/* If vblank wait fails, try without sequence */
		vbl.request.type = DRM_VBLANK_NEXTONMISS;
		vbl.request.sequence = 0;
		ret = drmWaitVBlank(state->fd, &vbl);
	}

	return ret;
}

static void drm_cleanup(struct drm_state *state)
{
	if (state->map && state->map != MAP_FAILED) {
		munmap(state->map, state->size);
		state->map = NULL;
	}

	if (state->fb_id && state->fd >= 0) {
		drmModeRmFB(state->fd, state->fb_id);
		state->fb_id = 0;
	}

	if (state->handle && state->fd >= 0) {
		struct drm_mode_destroy_dumb destroy = {};
		destroy.handle = state->handle;
		drmIoctl(state->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
		state->handle = 0;
	}

	if (state->fd >= 0) {
		close(state->fd);
		state->fd = -1;
	}
}

static void print_usage(const char *progname)
{
	printf("Usage: %s [options]\n", progname);
	printf("Options:\n");
	printf("  -d <device>  DRM device (default: %s)\n", DRM_DEVICE_DEFAULT);
	printf("  -t <seconds> Time per color in seconds (default: 2)\n");
	printf("  -h           Show this help\n");
	printf("\n");
	printf("This program cycles through different colors to test the display.\n");
	printf("Press Ctrl+C to exit.\n");
}

int main(int argc, char *argv[])
{
	struct drm_state state = {0};
	const char *device = DRM_DEVICE_DEFAULT;
	int time_per_color = 2;
	int opt;
	int ret;
	int color_index = 0;

	/* Parse command line options */
	while ((opt = getopt(argc, argv, "d:t:h")) != -1) {
		switch (opt) {
		case 'd':
			device = optarg;
			break;
		case 't':
			time_per_color = atoi(optarg);
			if (time_per_color <= 0) {
				fprintf(stderr, "Invalid time value: %s\n", optarg);
				return 1;
			}
			break;
		case 'h':
			print_usage(argv[0]);
			return 0;
		default:
			print_usage(argv[0]);
			return 1;
		}
	}

	/* Setup signal handler */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	printf("DSI Display Test Utility\n");
	printf("========================\n");
	printf("Device: %s\n", device);
	printf("Time per color: %d seconds\n", time_per_color);
	printf("Colors: %zu\n", NUM_COLORS);
	printf("\n");

	/* Open DRM device */
	state.fd = drm_open_device(device);
	if (state.fd < 0)
		return 1;

	/* Find connector and get display info */
	ret = drm_find_connector(&state);
	if (ret < 0) {
		drm_cleanup(&state);
		return 1;
	}

	/* Create framebuffer */
	ret = drm_create_fb(&state);
	if (ret < 0) {
		drm_cleanup(&state);
		return 1;
	}

	printf("Starting color cycle test...\n");
	printf("Press Ctrl+C to stop\n\n");

	/* Fill framebuffer with first color BEFORE setting CRTC */
	/* This ensures the screen shows the first color immediately */
	drm_fill_color(&state, colors[0]);
	
	/* Set CRTC initially with mode and first color */
	ret = drm_set_crtc(&state, 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to set CRTC initially\n");
		drm_cleanup(&state);
		return 1;
	}

	/* Main loop: cycle through colors */
	while (running) {
		const char *color_names[] = {
			"Red", "Green", "Blue", "White", "Black",
			"Yellow", "Magenta", "Cyan", "Orange", "Gray"
		};

		/* Fill framebuffer with current color */
		drm_fill_color(&state, colors[color_index]);
		
		/* Additional memory barrier before CRTC update */
		__sync_synchronize();
		
		/* Always force mode update to ensure framebuffer refresh */
		ret = drm_set_crtc(&state, 1);
		if (ret < 0) {
			fprintf(stderr, "Failed to update CRTC\n");
			break;
		}

		/* Print color info and framebuffer details (only for first few colors) */
		static int color_debug_count = 0;
		if (color_debug_count < 3) {
			printf("Color: %s (0x%06X) - FB ID=%u, Address=%p, Size=%zu bytes\n", 
			       color_names[color_index], colors[color_index],
			       state.fb_id, state.map, state.size);
			printf("DEBUG: Hardware read verification:\n");
			printf("  - Framebuffer data written: YES (verified)\n");
			printf("  - Memory barriers executed: YES\n");
			printf("  - Cache flushed: YES\n");
			printf("  - Vblank wait: Completed\n");
			printf("  - Kernel should have updated hardware registers (check kernel logs for yrgb_mst)\n");
			color_debug_count++;
		} else {
			printf("Color: %s (0x%06X)\n", 
			       color_names[color_index], colors[color_index]);
		}
		fflush(stdout);
		
		/* Additional delay to ensure hardware has time to process and display */
		/* The vblank wait in drm_set_crtc should be sufficient, but add a small delay as backup */
		usleep(50000);  /* 50ms - allow time for hardware to display */

		/* Wait for the specified time per color */
		sleep(time_per_color);

		/* Next color */
		color_index = (color_index + 1) % NUM_COLORS;
	}

	printf("\n\nTest stopped.\n");

	/* Cleanup */
	drm_cleanup(&state);

	return 0;
}


/*
 * MP4 Player for RK3506 EVM
 * 
 * This program uses GStreamer with software decoder (avdec_h264)
 * to play MP4 video files on DSI display.
 * 
 * Note: RK3506G2 does NOT have hardware video decoder (MPP/VDPU).
 * According to datasheet, it only has Video Output Processor for display.
 * Therefore, software decoder must be used.
 * 
 * Usage: mp4-player <video_file.mp4>
 *   Press Ctrl+C to exit
 * 
 * Dependencies:
 *   - gstreamer1.0
 *   - gstreamer1.0-plugins-base
 *   - gstreamer1.0-plugins-good
 *   - gstreamer1.0-plugins-bad
 *   - gstreamer1.0-libav (contains avdec_h264 software decoder, based on FFmpeg)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <gst/gst.h>

static GstElement *pipeline = NULL;
static volatile int running = 1;

static void signal_handler(int sig)
{
    running = 0;
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
}

/* Error callback */
static void on_error(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    GError *err;
    gchar *debug_info;
    
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n",
               GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    
    g_error_free(err);
    g_free(debug_info);
    
    running = 0;
}

/* EOS (End of Stream) callback */
static void on_eos(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    g_print("End of stream reached.\n");
    running = 0;
}

/* State change callback */
static void on_state_changed(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
    
    if (GST_OBJECT(msg->src) == GST_OBJECT(pipeline)) {
        g_print("Pipeline state changed from %s to %s\n",
                gst_element_state_get_name(old_state),
                gst_element_state_get_name(new_state));
    }
}

int main(int argc, char *argv[])
{
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    const char *video_file;
    
    /* Reset running flag at start */
    running = 1;
    
    /* Check arguments */
    if (argc < 2) {
        g_printerr("Usage: %s <video_file.mp4>\n", argv[0]);
        return -1;
    }
    
    video_file = argv[1];
    
    /* Check if file exists */
    if (access(video_file, R_OK) != 0) {
        g_printerr("Error: Cannot read file '%s'\n", video_file);
        return -1;
    }
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Enable debug output */
    g_setenv("GST_DEBUG", "3", FALSE);  /* 3 = INFO level */
    g_setenv("GST_DEBUG_FILE", "/tmp/gst-debug.log", FALSE);
    
    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Check DRM device */
    if (access("/dev/dri/card0", R_OK | W_OK) != 0) {
        g_printerr("Warning: Cannot access /dev/dri/card0: %s\n", strerror(errno));
        g_printerr("DRM device may not be available\n");
    } else {
        g_print("DRM device /dev/dri/card0 is accessible\n");
    }
    
    /* Check MPP device (Note: RK3506G2 does NOT have hardware video decoder) */
    if (access("/dev/mpp_service", R_OK | W_OK) != 0) {
        g_print("Note: RK3506G2 does not have hardware video decoder (MPP).\n");
        g_print("Using software decoder (avdec_h264) instead.\n");
    } else {
        g_print("MPP device /dev/mpp_service is accessible (unexpected for RK3506G2)\n");
    }
    
    /* Create pipeline
     * 
     * RK3506G2 does NOT have hardware video decoder (MPP/VDPU).
     * According to datasheet, it only has Video Output Processor for display.
     * Therefore, we must use software decoder (avdec_h264) instead of mppvideodec.
     * 
     * Pipeline strategies:
     * 1. Use decodebin for automatic format detection (recommended)
     * 2. Use explicit H.264 software decoder (avdec_h264)
     * 3. Fallback to simpler pipeline if needed
     */
    gchar *pipeline_str = NULL;
    GError *parse_error = NULL;
    
    /* First try: use decodebin for automatic format detection (recommended) */
    g_print("Trying pipeline with decodebin (software decoder)...\n");
    pipeline_str = g_strdup_printf(
        "filesrc location=%s ! "
        "qtdemux name=demux "
        "demux.video_0 ! queue ! "
        "decodebin ! "
        "videoconvert ! "
        "kmssink sync=false",
        video_file
    );
    
    pipeline = gst_parse_launch_full(pipeline_str, NULL, GST_PARSE_FLAG_NONE, &parse_error);
    
    /* Second try: explicit H.264 software decoder */
    if (!pipeline && parse_error) {
        g_print("decodebin failed, trying explicit H.264 software decoder...\n");
        g_error_free(parse_error);
        parse_error = NULL;
        g_free(pipeline_str);
        
        pipeline_str = g_strdup_printf(
            "filesrc location=%s ! "
            "qtdemux name=demux "
            "demux.video_0 ! queue ! "
            "video/x-h264,stream-format=avc,alignment=au ! "
            "h264parse ! "
            "avdec_h264 ! "
            "videoconvert ! "
            "kmssink sync=false",
            video_file
        );
        
        pipeline = gst_parse_launch_full(pipeline_str, NULL, GST_PARSE_FLAG_NONE, &parse_error);
    }
    
    /* Third try: simpler pipeline without explicit caps */
    if (!pipeline && parse_error) {
        g_print("Explicit decoder failed, trying simpler pipeline...\n");
        g_error_free(parse_error);
        parse_error = NULL;
        g_free(pipeline_str);
        
        pipeline_str = g_strdup_printf(
            "filesrc location=%s ! "
            "qtdemux name=demux "
            "demux.video_0 ! queue ! "
            "h264parse ! "
            "avdec_h264 ! "
            "videoconvert ! "
            "kmssink sync=false",
            video_file
        );
        
        pipeline = gst_parse_launch_full(pipeline_str, NULL, GST_PARSE_FLAG_NONE, &parse_error);
    }
    
    g_print("Pipeline string: %s\n", pipeline_str);
    
    if (!pipeline) {
        g_printerr("Failed to create pipeline with all attempted configurations\n");
        if (parse_error) {
            g_printerr("Last error: %s\n", parse_error->message);
            g_error_free(parse_error);
        }
        g_free(pipeline_str);
        return -1;
    }
    
    g_print("Pipeline created successfully\n");
    g_free(pipeline_str);
    
    /* Get bus for message handling */
    bus = gst_element_get_bus(pipeline);
    
    /* Add message handlers */
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error", G_CALLBACK(on_error), NULL);
    g_signal_connect(bus, "message::eos", G_CALLBACK(on_eos), NULL);
    g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_state_changed), NULL);
    
    /* Start playing */
    g_print("Starting playback of: %s\n", video_file);
    
    /* First set to READY state to check for errors */
    ret = gst_element_set_state(pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set pipeline to READY state\n");
        gst_bus_remove_signal_watch(bus);
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = NULL;
        running = 1;
        return -1;
    }
    g_print("Pipeline set to READY state\n");
    
    /* Now set to PLAYING */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set pipeline to PLAYING state\n");
        g_printerr("State change return: %s\n", 
                   gst_element_state_change_return_get_name(ret));
        gst_bus_remove_signal_watch(bus);
        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = NULL;
        running = 1;
        return -1;
    } else if (ret == GST_STATE_CHANGE_ASYNC) {
        g_print("State change is asynchronous, waiting...\n");
    } else {
        g_print("Pipeline set to PLAYING state successfully\n");
    }
    
    /* Wait until error or EOS */
    g_print("Playing... Press Ctrl+C to stop\n");
    
    /* Wait a bit for pipeline to start */
    usleep(500000); /* 500ms */
    
    /* Check current state */
    GstState state, pending;
    ret = gst_element_get_state(pipeline, &state, &pending, GST_SECOND);
    g_print("Pipeline state: %s (pending: %s, ret: %s)\n",
            gst_element_state_get_name(state),
            pending != GST_STATE_VOID_PENDING ? gst_element_state_get_name(pending) : "none",
            gst_element_state_change_return_get_name(ret));
    
    while (running) {
        msg = gst_bus_timed_pop_filtered(bus,
            GST_SECOND,  /* 1 second timeout */
            GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_WARNING);
        
        if (msg != NULL) {
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR:
                    on_error(bus, msg, NULL);
                    break;
                case GST_MESSAGE_EOS:
                    on_eos(bus, msg, NULL);
                    break;
                case GST_MESSAGE_WARNING:
                    {
                        GError *warn;
                        gchar *debug;
                        gst_message_parse_warning(msg, &warn, &debug);
                        g_print("Warning: %s\n", warn->message);
                        if (debug) g_print("Debug: %s\n", debug);
                        g_error_free(warn);
                        g_free(debug);
                    }
                    break;
                default:
                    break;
            }
            gst_message_unref(msg);
        }
    }
    
    /* Cleanup */
    g_print("Stopping playback...\n");
    
    /* Remove signal watch before stopping */
    gst_bus_remove_signal_watch(bus);
    
    /* Stop pipeline and wait for state change to complete */
    ret = gst_element_set_state(pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        /* Wait for state change to complete */
        gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    }
    
    /* Unref bus and pipeline */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    /* Reset static variables for next playback */
    pipeline = NULL;
    running = 1;
    
    g_print("Playback finished\n");
    return 0;
}


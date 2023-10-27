#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <boost/lockfree/spsc_queue.hpp> // ring buffer
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>

#include <iostream>

//#define FRAME_SIZE 6220800
#define FRAME_SIZE 921600

namespace bip = boost::interprocess;
namespace shm
{
    typedef bip::allocator<char, bip::managed_shared_memory::segment_manager> char_alloc;
    typedef bip::vector<char, char_alloc> shared_vector;
    typedef boost::lockfree::spsc_queue<shared_vector, boost::lockfree::capacity<2>> ring_buffer;
}

typedef unsigned char uchar;

typedef struct
{
    gboolean white;
    GstClockTime timestamp;
} MyContext;

//--------------------------------------------------------------------------------

bip::managed_shared_memory segment(bip::open_or_create, "MySharedMemory", FRAME_SIZE * 10);
shm::char_alloc char_alloc(segment.get_segment_manager());
shm::ring_buffer* queue = segment.find_or_construct<shm::ring_buffer>("queue")();
shm::shared_vector vector(char_alloc);

static void need_data(GstElement* appsrc, guint unused, MyContext* ctx)
{
    char* data = new char[FRAME_SIZE];
    if (queue->pop(vector)) {
        std::cout << "Processed: '" << vector.size() << "'\n";
        boost::container::copy(vector.begin(), vector.end(), data);
    }

    GstBuffer* buffer;
    GstFlowReturn ret;
    buffer = gst_buffer_new_allocate(NULL, FRAME_SIZE, NULL);
    gst_buffer_fill(buffer, 0, data, FRAME_SIZE); //&v[0] to use it as char*

    GST_BUFFER_PTS(buffer) = ctx->timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);
    ctx->timestamp += GST_BUFFER_DURATION(buffer);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
    delete data;
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data)
{
    GstElement* element, * appsrc;
    MyContext* ctx;

    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element(media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
    /* configure the caps of the video */
    g_object_set(G_OBJECT(appsrc), "caps",
        gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "BGR",
            "width", G_TYPE_INT, 640,
            "height", G_TYPE_INT, 480,
            "framerate", GST_TYPE_FRACTION, 60, 1, NULL),
        NULL);

    ctx = g_new0(MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    /* make sure ther datais freed when the media is gone */
    g_object_set_data_full(G_OBJECT(media), "my-extra-data", ctx,
        (GDestroyNotify)g_free);

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", (GCallback)need_data, ctx);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}

int main(int argc, char* argv[])
{
    GMainLoop* loop;
    GstRTSPServer* server;
    GstRTSPMountPoints* mounts;
    GstRTSPMediaFactory* factory;

    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    /* create a server instance */
    server = gst_rtsp_server_new();

    /* get the mount points for this server, every server has a default object
     * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points(server);

    /* make a media factory for a test stream. The default media factory can use
     * gst-launch syntax to create pipelines.
     * any launch line works as long as it contains elements named pay%d. Each
     * element with pay%d names will be a stream */
    factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory,
        "( appsrc name=mysrc ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 )");

    /* notify when our media is ready, This is called whenever someone asks for
     * the media and a new pipeline with our appsrc is created */
    g_signal_connect(factory, "media-configure", (GCallback)media_configure,
        NULL);

    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

    /* don't need the ref to the mounts anymore */
    g_object_unref(mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach(server, NULL);

    /* start serving */
    g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run(loop);

    return 0;
}
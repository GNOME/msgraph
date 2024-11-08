#include <config.h>
#include <glib.h>
/* #include <glib/gi18n-lib.h> */
#include <string.h>

#include "msg-uploader.h"

#define MAX_RESUMABLE_CHUNK_SIZE (512 * 1024) /* bytes = 512 KiB */

#define MSG_TYPE_UPLOADER_INPUT_STREAM (msg_uploader_input_stream_get_type ())


G_DECLARE_FINAL_TYPE (MsgUploaderInputStream, msg_uploader_input_stream, MSG, UPLOADER_INPUT_STREAM, GFilterInputStream);

typedef struct _MsgUploaderInputStream {
  GFilterInputStream parent;
} MsgUploaderInputStream;

enum {
  PROP_HEADER = 1,
  PROP_MAX_READ,
  PROP_TOTAL_WRITTEN,
};

typedef struct {
  GBytes *header;
  GInputStream *header_stream;
  gsize header_left;
  gint64 read_left;
  gsize total_written;
} MsgUploaderInputStreamPrivate;

static void msg_uploader_input_stream_pollable_init (GPollableInputStreamInterface *pollable_interface,
                                                     gpointer                       interface_data);

G_DEFINE_TYPE_WITH_CODE (MsgUploaderInputStream, msg_uploader_input_stream, G_TYPE_FILTER_INPUT_STREAM,
                         G_ADD_PRIVATE (MsgUploaderInputStream)
                         G_IMPLEMENT_INTERFACE (G_TYPE_POLLABLE_INPUT_STREAM,
                                                msg_uploader_input_stream_pollable_init))

static void
msg_uploader_input_stream_init (MsgUploaderInputStream *stream)
{
}

static void
msg_uploader_input_stream_finalize (GObject *object)
{
  MsgUploaderInputStream *stream = MSG_UPLOADER_INPUT_STREAM (object);
  MsgUploaderInputStreamPrivate *priv = msg_uploader_input_stream_get_instance_private (stream);

  if (priv->header_stream) {
    g_input_stream_close (priv->header_stream, NULL, NULL);
    g_object_unref (priv->header_stream);
  }
  g_clear_pointer (&priv->header, g_bytes_unref);

  G_OBJECT_CLASS (msg_uploader_input_stream_parent_class)->finalize (object);
}

static void
msg_uploader_input_stream_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  MsgUploaderInputStream *stream = MSG_UPLOADER_INPUT_STREAM (object);
  MsgUploaderInputStreamPrivate *priv = msg_uploader_input_stream_get_instance_private (stream);

  switch (prop_id) {
    case PROP_HEADER:
      if (!g_value_get_pointer (value)) {
        priv->header = NULL;
        priv->header_stream = NULL;
        priv->header_left = 0;
        break;
      }
      priv->header = g_bytes_ref (g_value_get_pointer (value));
      priv->header_stream = g_memory_input_stream_new_from_bytes (priv->header);
      priv->header_left = g_bytes_get_size (priv->header);
      break;
    case PROP_MAX_READ:
      priv->read_left = g_value_get_int64 (value);
      break;
    case PROP_TOTAL_WRITTEN:
      priv->total_written = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
msg_uploader_input_stream_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  MsgUploaderInputStream *stream = MSG_UPLOADER_INPUT_STREAM (object);
  MsgUploaderInputStreamPrivate *priv = msg_uploader_input_stream_get_instance_private (stream);

  switch (prop_id) {
    case PROP_HEADER:
      g_value_set_pointer (value, g_bytes_ref (priv->header));
      break;
    case PROP_MAX_READ:
      g_value_set_int64 (value, priv->read_left);
      break;
    case PROP_TOTAL_WRITTEN:
      g_value_set_uint (value, priv->total_written);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gssize
read_internal (GInputStream  *stream,
               void          *buffer,
               gsize          count,
               gboolean       blocking,
               GCancellable  *cancellable,
               GError       **error)
{
  MsgUploaderInputStream *istream = MSG_UPLOADER_INPUT_STREAM (stream);
  MsgUploaderInputStreamPrivate *priv = msg_uploader_input_stream_get_instance_private (istream);
  GError *rerror = NULL;
  gssize nread = 0;
  gssize sread;
  gssize rcount;
  char *tbuf = buffer;

  if (priv->read_left == 0)
    return 0;

  /* there are bytes left in the header stream */
  if (priv->header_left > 0) {
    nread = g_pollable_stream_read (priv->header_stream, buffer, count,
                                    blocking, cancellable, error);
    /* error: bail out */
    if (nread < 0)
      return nread;

    priv->header_left -= nread;

    /* we've read less than was left, since this is a memory stream this
     * indicates that we actually wanted less, so just return for now
     */
    if (priv->header_left > 0)
      return nread;

    g_input_stream_close (priv->header_stream, NULL, NULL);
    g_object_unref (priv->header_stream);
    priv->header_stream = NULL;
  }

  rcount = count - nread;
  if (priv->read_left > 0 && priv->read_left < rcount)
    rcount = priv->read_left;

  sread = g_pollable_stream_read (G_FILTER_INPUT_STREAM (stream)->base_stream,
                                  tbuf + nread, rcount, blocking,
                                  cancellable, (nread > 0) ? &rerror : error);

  if (sread < 0) {
    /* ensure proper behavior for partial reads */
    if (nread > 0) {
      if (g_error_matches (rerror, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
        g_cancellable_reset (cancellable);
        g_clear_error (error);
        return nread;
      } else if (!blocking && g_error_matches (rerror, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK)) {
        g_clear_error (error);
        return nread;
      }
    }
    if (nread < 0) {
      if (error)
        *error = rerror;
      else
        g_clear_error (&rerror);
    }
    return sread;
  } else {
    priv->total_written += sread;
    g_object_notify (G_OBJECT (istream), "total-written");
  }

  if (priv->read_left > 0)
    priv->read_left -= sread;

  return nread + sread;
}

static gssize
msg_uploader_input_stream_read (GInputStream  *stream,
                                void          *buffer,
                                gsize          count,
                                GCancellable  *cancellable,
                                GError       **error)
{
  return read_internal (stream, buffer, count, TRUE, cancellable, error);
}

static gssize
msg_uploader_input_stream_skip (GInputStream  *stream,
                                gsize          count,
                                GCancellable  *cancellable,
                                GError       **error)
{
  MsgUploaderInputStream *istream = MSG_UPLOADER_INPUT_STREAM (stream);
  MsgUploaderInputStreamPrivate *priv = msg_uploader_input_stream_get_instance_private (istream);
  GError *rerror = NULL;
  gssize nread = 0;
  gssize sread;
  gssize rcount;

  if (priv->read_left == 0)
    return 0;

  /* there are bytes left in the header stream */
  if (priv->header_left > 0) {
    nread = g_input_stream_skip (priv->header_stream,
                                 count, cancellable, error);
    /* error: bail out */
    if (nread < 0)
      return nread;

    priv->header_left -= nread;

    /* we've read less than was left, since this is a memory stream this
     * indicates that we actually wanted less, so just return for now
     */
    if (priv->header_left > 0)
      return nread;

    g_input_stream_close (priv->header_stream, NULL, NULL);
    g_object_unref (priv->header_stream);
    priv->header_stream = NULL;
  }

  rcount = count - nread;
  if (priv->read_left > 0 && priv->read_left < rcount)
    rcount = priv->read_left;

  /* try reading full or partial count from the actual stream */
  sread = g_input_stream_skip (G_FILTER_INPUT_STREAM (stream)->base_stream,
                               rcount, cancellable,
                               (nread > 0) ? &rerror : error);
  if (sread < 0) {
    /* ensure proper behavior for partial reads */
    if (nread > 0 && g_error_matches (rerror, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_cancellable_reset (cancellable);
      g_clear_error (error);
      return nread;
    }
    if (!nread) {
      if (error)
        *error = rerror;
      else
        g_clear_error (&rerror);
    }
    return sread;
  } else {
    priv->total_written += sread;
    g_object_notify (G_OBJECT (istream), "total-written");
  }

  if (priv->read_left > 0)
    priv->read_left -= sread;

  return nread + sread;
}

static void
msg_uploader_input_stream_class_init (MsgUploaderInputStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *input_stream_class = G_INPUT_STREAM_CLASS (klass);

  object_class->finalize = msg_uploader_input_stream_finalize;
  object_class->set_property = msg_uploader_input_stream_set_property;
  object_class->get_property = msg_uploader_input_stream_get_property;

  input_stream_class->read_fn = msg_uploader_input_stream_read;
  input_stream_class->skip = msg_uploader_input_stream_skip;

  g_object_class_install_property (
    object_class, PROP_HEADER,
    g_param_spec_pointer ("header",
                          "Header",
                          "The stream's header bytes",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (
    object_class, PROP_MAX_READ,
    g_param_spec_int64 ("max-read",
                        "Max read",
                        "Maximum amount to read",
                        -1, G_MAXINT64, -1,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (
    object_class, PROP_TOTAL_WRITTEN,
    g_param_spec_uint ("total-written",
                       "Total written",
                       "Total data bytes written",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                       G_PARAM_STATIC_STRINGS));
}

static gboolean
msg_uploader_input_stream_can_poll (GPollableInputStream *stream)
{
  GInputStream *base_stream = G_FILTER_INPUT_STREAM (stream)->base_stream;

  return G_IS_POLLABLE_INPUT_STREAM (base_stream) &&
         g_pollable_input_stream_can_poll (G_POLLABLE_INPUT_STREAM (base_stream));
}

static gboolean
msg_uploader_input_stream_is_readable (GPollableInputStream *stream)
{
  /* the memory stream is always readable, so we don't care about that one,
   * instead we check the real stream always...
   */
  return g_pollable_input_stream_is_readable (G_POLLABLE_INPUT_STREAM (G_FILTER_INPUT_STREAM (stream)->base_stream));
}

static gssize
msg_uploader_input_stream_read_nonblocking (GPollableInputStream  *stream,
                                            void                  *buffer,
                                            gsize                  count,
                                            GError               **error)
{
  return read_internal (G_INPUT_STREAM (stream),
                        buffer, count, FALSE, NULL, error);
}

static GSource *
msg_uploader_input_stream_create_source (GPollableInputStream *stream,
                                         GCancellable         *cancellable)
{
  GSource *base_source, *pollable_source;

  base_source = g_pollable_input_stream_create_source (G_POLLABLE_INPUT_STREAM (G_FILTER_INPUT_STREAM (stream)->base_stream),
                                                       cancellable);

  g_source_set_dummy_callback (base_source);
  pollable_source = g_pollable_source_new (G_OBJECT (stream));
  g_source_add_child_source (pollable_source, base_source);
  g_source_unref (base_source);

  return pollable_source;
}

static void
msg_uploader_input_stream_pollable_init (GPollableInputStreamInterface *iface,
                                         gpointer                       iface_data)
{
  iface->can_poll = msg_uploader_input_stream_can_poll;
  iface->is_readable = msg_uploader_input_stream_is_readable;
  iface->read_nonblocking = msg_uploader_input_stream_read_nonblocking;
  iface->create_source = msg_uploader_input_stream_create_source;
}

static void msg_uploader_constructed (GObject *object);
static void msg_uploader_dispose (GObject *object);
static void msg_uploader_finalize (GObject *object);
static void msg_uploader_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec);
static void msg_uploader_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);

struct _MsgUploaderPrivate {
  gchar *method;
  gchar *upload_uri;
  MsgService *service;
  gchar *slug;
  gchar *content_type;
  goffset content_length; /* -1 for non-resumable uploads; 0 or greater for resumable ones */
  SoupSession *session;
  SoupMessage *message;
  GBytes *header;
  GInputStream *body_stream;
  gsize total_written;
};

enum {
  PROP_SERVICE = 1,
  PROP_UPLOAD_URI,
  PROP_SLUG,
  PROP_CONTENT_TYPE,
  PROP_METHOD,
  PROP_CONTENT_LENGTH,
};

G_DEFINE_TYPE_WITH_PRIVATE (MsgUploader, msg_uploader, G_TYPE_OBJECT)

static void
msg_uploader_class_init (MsgUploaderClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = msg_uploader_constructed;
  gobject_class->dispose = msg_uploader_dispose;
  gobject_class->finalize = msg_uploader_finalize;
  gobject_class->get_property = msg_uploader_get_property;
  gobject_class->set_property = msg_uploader_set_property;

  /**
   * MsgUploader:service:
   *
   * The service which is used to authorize the upload, and to which the upload relates.
   *
   * Since: 0.5.0
   */
  g_object_class_install_property (gobject_class, PROP_SERVICE,
                                   g_param_spec_object ("service",
                                                        "Service", "The service which is used to authorize the upload.",
                                                        MSG_TYPE_SERVICE,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * MsgUploader:method:
   *
   * The HTTP request method to use when uploading the file.
   *
   * Since: 0.7.0
   */
  g_object_class_install_property (gobject_class, PROP_METHOD,
                                   g_param_spec_string ("method",
                                                        "Method", "The HTTP request method to use when uploading the file.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * MsgUploader:upload-uri:
   *
   * The URI to upload the data and metadata to. This must be HTTPS.
   *
   * Since: 0.5.0
   */
  g_object_class_install_property (gobject_class, PROP_UPLOAD_URI,
                                   g_param_spec_string ("upload-uri",
                                                        "Upload URI", "The URI to upload the data and metadata to.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * MsgUploader:slug:
   *
   * The slug of the file being uploaded. This is usually the display name of the file (i.e. as returned by g_file_info_get_display_name()).
   *
   * Since: 0.5.0
   */
  g_object_class_install_property (gobject_class, PROP_SLUG,
                                   g_param_spec_string ("slug",
                                                        "Slug", "The slug of the file being uploaded.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * MsgUploader:content-length:
   *
   * The content length (in bytes) of the file being uploaded (i.e. as returned by g_file_info_get_size()). Note that this does not include the
   * length of the XML serialisation of #MsgUploader:entry, if set.
   *
   * If this is <code class="literal">-1</code> the upload will be non-resumable; if it is non-negative, the upload will be resumable.
   *
   * Since: 0.13.0
   */
  g_object_class_install_property (gobject_class, PROP_CONTENT_LENGTH,
                                   g_param_spec_int64 ("content-length",
                                                       "Content length", "The content length (in bytes) of the file being uploaded.",
                                                       -1, G_MAXINT64, -1,
                                                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * MsgUploader:content-type:
   *
   * The content type of the file being uploaded (i.e. as returned by g_file_info_get_content_type()).
   *
   * Since: 0.5.0
   */
  g_object_class_install_property (gobject_class, PROP_CONTENT_TYPE,
                                   g_param_spec_string ("content-type",
                                                        "Content type", "The content type of the file being uploaded.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
msg_uploader_init (MsgUploader *self)
{
  self->priv = msg_uploader_get_instance_private (self);
}

static SoupMessage *
build_message (MsgUploader *self,
               MsgService  *service,
               const gchar *method,
               const gchar *upload_uri)
{
  SoupMessage *new_message;
  GUri *_uri, *_uri_parsed;

  /* Build the message */
  _uri_parsed = g_uri_parse (upload_uri, SOUP_HTTP_URI_FLAGS, NULL);
  _uri = soup_uri_copy (_uri_parsed, SOUP_URI_PORT,
                        msg_service_get_https_port (), SOUP_URI_NONE);
  g_uri_unref (_uri_parsed);
  new_message = msg_service_build_message (service, method, g_uri_to_string (_uri), NULL, FALSE);
  g_uri_unref (_uri);

  return new_message;
}

static void
msg_uploader_constructed (GObject *object)
{
  MsgUploaderPrivate *priv;
  MsgServiceClass *service_klass;
  GUri *uri = NULL;

  /* Chain up to the parent class */
  G_OBJECT_CLASS (msg_uploader_parent_class)->constructed (object);
  priv = MSG_UPLOADER (object)->priv;

  /* The upload URI must be HTTPS. */
  uri = g_uri_parse (priv->upload_uri, SOUP_HTTP_URI_FLAGS, NULL);
  g_assert_cmpstr (g_uri_get_scheme (uri), ==, "https");
  g_uri_unref (uri);

  /* Build the message */
  priv->message = build_message (MSG_UPLOADER (object), priv->service, priv->method, priv->upload_uri);

  if (priv->slug != NULL)
    soup_message_headers_append (soup_message_get_request_headers (priv->message), "Slug", priv->slug);

  if (priv->content_length == -1) {
    /* Non-resumable upload */
    soup_message_headers_set_encoding (soup_message_get_request_headers (priv->message), SOUP_ENCODING_CHUNKED);

    /* The Content-Type should be multipart/related if we're also uploading the metadata (entry != NULL),
     * and the given content_type otherwise. */

#if 0
    if (priv->entry != NULL) {
      gchar *first_part_header, *upload_data;
      gchar *second_part_header;
      MsgParsableClass *parsable_klass;
      GByteArray *body;

      parsable_klass = MSG_PARSABLE_GET_CLASS (priv->entry);
      g_assert (parsable_klass->get_content_type != NULL);

      soup_message_headers_set_content_type (soup_message_get_request_headers (priv->message), "multipart/related; boundary=" BOUNDARY_STRING, NULL);

      if (g_strcmp0 (parsable_klass->get_content_type (), "application/json") == 0) {
        upload_data = msg_parsable_get_json (MSG_PARSABLE (priv->entry));
      } else {
        upload_data = msg_parsable_get_xml (MSG_PARSABLE (priv->entry));
      }

      body = g_byte_array_new ();

      /* Start by writing out the entry; then the thread has something to write to the network when it's created */
      first_part_header = g_strdup_printf ("--" BOUNDARY_STRING "\n"
                                           "Content-Type: %s; charset=UTF-8\n\n",
                                           parsable_klass->get_content_type ());
      second_part_header = g_strdup_printf ("\n--" BOUNDARY_STRING "\n"
                                            "Content-Type: %s\n"
                                            "Content-Transfer-Encoding: binary\n\n",
                                            priv->content_type);

      /* Push the message parts onto the message body; we can skip the buffer, since the network thread hasn't yet been created,
       * so we're the sole thread accessing the SoupMessage. */
      g_byte_array_append (body, (const guint8 *)first_part_header, strlen (first_part_header));
      g_byte_array_append (body, (const guint8 *)upload_data, strlen (upload_data));
      g_byte_array_append (body, (const guint8 *)second_part_header, strlen (second_part_header));

      g_free (first_part_header);
      g_free (second_part_header);
      g_free (upload_data);

      priv->header = g_byte_array_free_to_bytes (body);
    } else {
#endif
    soup_message_headers_set_content_type (soup_message_get_request_headers (priv->message), priv->content_type, NULL);
    priv->header = NULL;
    /* } */
  } else {
    gchar *content_length_str;

    /* Resumable upload's initial request */
    soup_message_headers_set_encoding (soup_message_get_request_headers (priv->message), SOUP_ENCODING_CONTENT_LENGTH);
    soup_message_headers_replace (soup_message_get_request_headers (priv->message), "X-Upload-Content-Type", priv->content_type);

    content_length_str = g_strdup_printf ("%" G_GOFFSET_FORMAT, priv->content_length);
    soup_message_headers_replace (soup_message_get_request_headers (priv->message), "X-Upload-Content-Length", content_length_str);
    g_free (content_length_str);

    /*if (priv->entry != NULL) {
     *  MsgParsableClass *parsable_klass;
     *  gchar *content_type, *upload_data;
     *
     *  parsable_klass = MSG_PARSABLE_GET_CLASS (priv->entry);
     *  g_assert (parsable_klass->get_content_type != NULL);
     *
     *  if (g_strcmp0 (parsable_klass->get_content_type (), "application/json") == 0) {
     *   upload_data = msg_parsable_get_json (MSG_PARSABLE (priv->entry));
     *  } else {
     *   upload_data = msg_parsable_get_xml (MSG_PARSABLE (priv->entry));
     *  }
     *
     *  content_type = g_strdup_printf ("%s; charset=UTF-8",
     *                 parsable_klass->get_content_type ());
     *  soup_message_headers_set_content_type (soup_message_get_request_headers (priv->message),
     *                      content_type,
     *                      NULL);
     *  g_free (content_type);
     *
     *  priv->header = g_bytes_new_take (upload_data, strlen (upload_data));
     *  } else {*/
    soup_message_headers_set_content_length (soup_message_get_request_headers (priv->message), 0);
    /*} */
  }

  /* If the entry exists and has an ETag, we assume we're updating the entry, so we can set the If-Match header */
  /* if (priv->entry != NULL && msg_entry_get_etag (priv->entry) != NULL) */
  /*   soup_message_headers_append (soup_message_get_request_headers (priv->message), "If-Match", msg_entry_get_etag (priv->entry)); */

  /* Uploading doesn't actually start until the first call to write() */
}

static void
msg_uploader_dispose (GObject *object)
{
  MsgUploaderPrivate *priv = MSG_UPLOADER (object)->priv;

  g_clear_object (&priv->service);
  g_clear_object (&priv->message);
  /* g_clear_object (&priv->entry); */
  g_clear_pointer (&priv->header, g_bytes_unref);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (msg_uploader_parent_class)->dispose (object);
}

static void
msg_uploader_finalize (GObject *object)
{
  MsgUploaderPrivate *priv = MSG_UPLOADER (object)->priv;

  g_free (priv->upload_uri);
  g_free (priv->method);
  g_free (priv->slug);
  g_free (priv->content_type);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (msg_uploader_parent_class)->finalize (object);
}

static void
msg_uploader_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MsgUploaderPrivate *priv = MSG_UPLOADER (object)->priv;

  switch (property_id) {
    case PROP_SERVICE:
      g_value_set_object (value, priv->service);
      break;
    case PROP_METHOD:
      g_value_set_string (value, priv->method);
      break;
    case PROP_UPLOAD_URI:
      g_value_set_string (value, priv->upload_uri);
      break;
    case PROP_SLUG:
      g_value_set_string (value, priv->slug);
      break;
    case PROP_CONTENT_TYPE:
      g_value_set_string (value, priv->content_type);
      break;
    case PROP_CONTENT_LENGTH:
      g_value_set_int64 (value, priv->content_length);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
msg_uploader_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MsgUploaderPrivate *priv = MSG_UPLOADER (object)->priv;

  switch (property_id) {
    case PROP_SERVICE:
      priv->service = g_value_dup_object (value);
      priv->session = msg_service_get_session (priv->service);
      break;
    case PROP_METHOD:
      priv->method = g_value_dup_string (value);
      break;
    case PROP_UPLOAD_URI:
      priv->upload_uri = g_value_dup_string (value);
      break;
    case PROP_SLUG:
      priv->slug = g_value_dup_string (value);
      break;
    case PROP_CONTENT_TYPE:
      priv->content_type = g_value_dup_string (value);
      break;
    case PROP_CONTENT_LENGTH:
      priv->content_length = g_value_get_int64 (value);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

MsgUploader *
msg_uploader_new (MsgService  *service,
                  const gchar *method,
                  const gchar *upload_uri,
                  const gchar *slug,
                  const gchar *content_type)
{
  g_return_val_if_fail (MSG_IS_SERVICE (service), NULL);
  g_return_val_if_fail (method != NULL, NULL);
  g_return_val_if_fail (upload_uri != NULL, NULL);
  /* g_return_val_if_fail (entry == NULL || MSG_IS_ENTRY (entry), NULL); */
  g_return_val_if_fail (slug != NULL, NULL);
  g_return_val_if_fail (content_type != NULL, NULL);

  return MSG_UPLOADER (g_object_new (MSG_TYPE_UPLOADER,
                                     "method", method,
                                     "upload-uri", upload_uri,
                                     "service", service,
                                     "slug", slug,
                                     "content-type", content_type,
                                     NULL));
}

MsgUploader *
msg_uploader_new_resumable (MsgService  *service,
                            const gchar *upload_uri)
{
  g_return_val_if_fail (MSG_IS_SERVICE (service), NULL);
  /* g_return_val_if_fail (method != NULL, NULL); */
  g_return_val_if_fail (upload_uri != NULL, NULL);
  /* g_return_val_if_fail (slug != NULL, NULL); */
  /* g_return_val_if_fail (content_type != NULL, NULL); */
  /* g_return_val_if_fail (content_length >= 0, NULL); */

  return MSG_UPLOADER (g_object_new (MSG_TYPE_UPLOADER,
                                     "method", "PUT",
                                     "upload-uri", upload_uri,
                                     "service", service,
                                     /* "slug", slug, */
                                     /* "content-type", content_type, */
                                     /* "content-length", content_length, */
                                     NULL));
}

void
msg_uploader_set_input (MsgUploader  *self,
                        GInputStream *stream)
{
  self->priv->body_stream = g_object_ref (stream);
}

void
msg_uploader_set_input_from_bytes (MsgUploader *self,
                                   GBytes      *bytes)
{
  msg_uploader_set_input (self,
                          g_memory_input_stream_new_from_bytes (bytes));
}

static void
wrote_body_data_cb (SoupMessage *message,
                    guint        chunk_size,
                    gsize       *total_written)
{
  *total_written += chunk_size;
}

static GError *
upload_status_error (MsgUploaderPrivate *priv,
                     SoupStatus          status,
                     GBytes             *response)
{
  GError *error = NULL;
  MsgServiceClass *klass = MSG_SERVICE_GET_CLASS (priv->service);
  g_warning ("FIXME");
  /* klass->parse_error_response (priv->service, MSG_OPERATION_UPLOAD, */
  /*                status, */
  /*                soup_message_get_reason_phrase (priv->message), */
  /*                g_bytes_get_data (response, NULL), */
  /*                g_bytes_get_size (response), */
  /*                &error); */
  return error;
}

GBytes *
msg_uploader_send (MsgUploader   *self,
                   GCancellable  *cancellable,
                   gsize         *total_written,
                   GError       **error)
{
  MsgUploaderPrivate *priv = self->priv;
  MsgAuthorizer *authorizer;
  GInputStream *mbody;
  g_autoptr (GBytes) response = NULL;
  SoupStatus status;

  priv->total_written = 0;

  if (total_written)
    *total_written = 0;

  /* FIXME: Refresh authorization before sending message in order to prevent authorization errors during transfer.
   * See: https://gitlab.gnome.org/GNOME/libmsg/issues/23 */
  authorizer = msg_service_get_authorizer (priv->service);
  if (authorizer) {
    g_autoptr (GError) error = NULL;

    msg_authorizer_refresh_authorization (authorizer, cancellable,
                                          &error);
    if (error != NULL)
      g_debug ("Error returned when refreshing authorization: %s",
               error->message);
    else
      msg_authorizer_process_request (authorizer, priv->message);
  }

  if (priv->content_length == -1) {
    GBytes *ret;

    /* do a non-resumable upload */
    if (priv->header) {
      /* metadata plus content */
      mbody = g_object_new (MSG_TYPE_UPLOADER_INPUT_STREAM,
                            "base-stream", priv->body_stream,
                            "header", priv->header,
                            NULL);
      soup_message_set_request_body (priv->message, NULL,
                                     mbody, -1);
      g_object_unref (mbody);
    } else {
      /* content-only upload */
      soup_message_set_request_body (priv->message, NULL,
                                     priv->body_stream, -1);
      if (total_written)
        g_signal_connect (priv->message, "wrote-body-data",
                          G_CALLBACK (wrote_body_data_cb),
                          total_written);
    }

    ret = soup_session_send_and_read (priv->session,
                                      priv->message,
                                      cancellable,
                                      error);
    if (ret && priv->header && total_written)
      *total_written = g_bytes_get_size (ret);

    return ret;
  }

  /* resumable upload needs multiple requests */
  if (priv->header) {
    soup_message_set_request_body_from_bytes (priv->message, NULL,
                                              priv->header);
  }

  /* send initial request */
  response = soup_session_send_and_read (priv->session, priv->message,
                                         cancellable, error);
  if (!response)
    return NULL;

  status = soup_message_get_status (priv->message);

  /* unsuccessful upload */
  if (!SOUP_STATUS_IS_SUCCESSFUL (status)) {
    if (error)
      *error = upload_status_error (priv, status, response);
    return NULL;
  }

  /* metadata-only upload */
  if (priv->content_length == 0 && status == SOUP_STATUS_CREATED)
    return g_bytes_ref (response);

  for (;;) {
    MsgServiceClass *klass;
    gchar *new_uri;
    SoupMessage *new_message;
    gsize next_chunk_length;

    next_chunk_length = MIN (priv->content_length - priv->total_written,
                             MAX_RESUMABLE_CHUNK_SIZE);

    new_uri = g_strdup (soup_message_headers_get_one (soup_message_get_response_headers (priv->message), "Location"));
    if (new_uri == NULL) {
      new_uri = g_uri_to_string_partial (soup_message_get_uri (priv->message), G_URI_HIDE_PASSWORD);
    }

    new_message = build_message (self, priv->service,
                                 SOUP_METHOD_PUT, new_uri);

    g_signal_connect (new_message, "wrote-body-data",
                      G_CALLBACK (wrote_body_data_cb), &priv->total_written);

    g_free (new_uri);

    mbody = g_object_new (MSG_TYPE_UPLOADER_INPUT_STREAM,
                          "base-stream", priv->body_stream,
                          "close-base-stream", FALSE,
                          "max-read", (gint64)next_chunk_length, NULL);

    soup_message_set_request_body (new_message, priv->content_type,
                                   mbody, next_chunk_length);

    g_object_unref (mbody);

    soup_message_headers_set_content_range (soup_message_get_request_headers (new_message),
                                            priv->total_written,
                                            priv->total_written + next_chunk_length - 1,
                                            priv->content_length);

    /* Make sure the headers are set. HACK: This should actually be in build_message(), but we have to work around
     * http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=3033 in MsgDocumentsService's append_query_headers(). */
    klass = MSG_SERVICE_GET_CLASS (priv->service);

    g_object_unref (priv->message);
    priv->message = new_message;

    g_bytes_unref (response);
    response = soup_session_send_and_read (priv->session,
                                           priv->message,
                                           cancellable,
                                           error);
    if (!response) {
      /* error */
      return NULL;
    }

    status = soup_message_get_status (priv->message);

    if (status == 308) {
      /* continuation */
      continue;
    }

    /* completed */
    if (SOUP_STATUS_IS_SUCCESSFUL (status)) {
      if (total_written)
        *total_written = priv->total_written;
      return g_bytes_ref (response);
    }

    /* error */
    if (error)
      *error = upload_status_error (priv, status, response);
    return NULL;
  }
}

static void
non_resumable_cb (GObject      *object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  SoupSession *session = SOUP_SESSION (object);
  g_autoptr (GTask) task = user_data;
  GError *error = NULL;
  GBytes *body;

  body = soup_session_send_and_read_finish (session, result, &error);
  if (body)
    g_task_return_pointer (task, body, (GDestroyNotify)g_bytes_unref);
  else
    g_task_return_error (task, error);
}

static void try_send_chunk (GTask *task);

static void
send_chunk_cb (GObject      *object,
               GAsyncResult *result,
               gpointer      user_data)
{
  SoupSession *session = SOUP_SESSION (object);
  g_autoptr (GTask) task = user_data;
  MsgUploader *self = g_task_get_task_data (task);
  MsgUploaderPrivate *priv = self->priv;
  GError *error = NULL;
  guint status;
  g_autoptr (GBytes) body = NULL;

  body = soup_session_send_and_read_finish (session, result, &error);
  if (!body) {
    /* error */
    g_task_return_error (task, error);
    return;
  }

  status = soup_message_get_status (soup_session_get_async_result_message (session, result));

  if (status == 308) {
    /* continuation */
    try_send_chunk (g_object_ref (task));
    return;
  }

  /* completed */
  if (SOUP_STATUS_IS_SUCCESSFUL (status)) {
    g_task_return_pointer (task, body, (GDestroyNotify)g_bytes_unref);
    return;
  }

  /* error */
  error = upload_status_error (priv, status, body);
  g_task_return_error (task, error);
}

static void
try_send_chunk (GTask *task)
{
  MsgUploader *self = g_task_get_task_data (task);
  MsgUploaderPrivate *priv = self->priv;
  MsgServiceClass *klass;
  GInputStream *mbody;
  gchar *new_uri;
  SoupMessage *new_message;
  gsize next_chunk_length;

  next_chunk_length = MIN (priv->content_length - priv->total_written,
                           MAX_RESUMABLE_CHUNK_SIZE);

  new_uri = g_strdup (soup_message_headers_get_one (soup_message_get_response_headers (priv->message), "Location"));
  if (new_uri == NULL) {
    new_uri = g_uri_to_string_partial (soup_message_get_uri (priv->message), G_URI_HIDE_PASSWORD);
  }

  new_message = build_message (self, priv->service,
                               SOUP_METHOD_PUT, new_uri);

  g_signal_connect (new_message, "wrote-body-data",
                    G_CALLBACK (wrote_body_data_cb), &priv->total_written);

  g_free (new_uri);

  mbody = g_object_new (MSG_TYPE_UPLOADER_INPUT_STREAM,
                        "base-stream", priv->body_stream,
                        "close-base-stream", FALSE,
                        "max-read", (gint64)next_chunk_length, NULL);

  soup_message_set_request_body (new_message, priv->content_type,
                                 mbody, next_chunk_length);

  g_object_unref (mbody);

  soup_message_headers_set_content_range (soup_message_get_request_headers (new_message),
                                          priv->total_written,
                                          priv->total_written + next_chunk_length - 1,
                                          priv->content_length);

  /* Make sure the headers are set. HACK: This should actually be in build_message(), but we have to work around
   * http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=3033 in MsgDocumentsService's append_query_headers(). */
  klass = MSG_SERVICE_GET_CLASS (priv->service);

  g_object_unref (priv->message);
  priv->message = new_message;

  soup_session_send_and_read_async (priv->session, priv->message,
                                    G_PRIORITY_DEFAULT,
                                    g_task_get_cancellable (task),
                                    send_chunk_cb,
                                    task);
}

static void
resumable_cb (GObject      *object,
              GAsyncResult *result,
              gpointer      user_data)
{
  SoupSession *session = SOUP_SESSION (object);
  g_autoptr (GTask) task = user_data;
  MsgUploader *self = g_task_get_task_data (task);
  MsgUploaderPrivate *priv = self->priv;
  GError *error = NULL;
  g_autoptr (GBytes) body = NULL;
  guint status;

  body = soup_session_send_and_read_finish (session, result, &error);
  if (!body) {
    g_task_return_error (task, error);
    return;
  }

  status = soup_message_get_status (priv->message);

  /* unsuccessful upload */
  if (!SOUP_STATUS_IS_SUCCESSFUL (status)) {
    error = upload_status_error (priv, status, body);
    g_task_return_error (task, error);
    return;
  }

  /* metadata-only upload */
  if (priv->content_length == 0 && status == SOUP_STATUS_CREATED) {
    g_task_return_pointer (task, body, (GDestroyNotify)g_bytes_unref);
  }

  /* otherwise proceed */
  try_send_chunk (g_object_ref (task));
}

static void
send_async_cb (GTask *task)
{
  MsgUploader *self = g_task_get_task_data (task);
  MsgUploaderPrivate *priv = self->priv;

  if (priv->content_length == -1) {
    /* do a non-resumable upload */
    if (priv->header) {
      g_autoptr (GInputStream) actual_body = NULL;
      /* metadata plus content */
      actual_body = g_object_new (MSG_TYPE_UPLOADER_INPUT_STREAM,
                                  "base-stream", priv->body_stream,
                                  "header", priv->header,
                                  NULL);
      soup_message_set_request_body (priv->message, NULL,
                                     actual_body, -1);
    } else {
      /* content-only upload */
      soup_message_set_request_body (priv->message, NULL,
                                     priv->body_stream, -1);
      g_signal_connect (priv->message, "wrote-body-data",
                        G_CALLBACK (wrote_body_data_cb),
                        &priv->total_written);
    }

    soup_session_send_and_read_async (priv->session,
                                      priv->message,
                                      G_PRIORITY_DEFAULT,
                                      g_task_get_cancellable (task),
                                      non_resumable_cb,
                                      task);
    return;
  }

  /* resumable upload needs multiple requests */
  if (priv->header) {
    soup_message_set_request_body_from_bytes (priv->message, NULL,
                                              priv->header);
  }

  soup_session_send_and_read_async (priv->session,
                                    priv->message,
                                    G_PRIORITY_DEFAULT,
                                    g_task_get_cancellable (task),
                                    resumable_cb,
                                    task);
}

static void
refresh_auth_cb (GObject      *object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  MsgAuthorizer *authorizer = MSG_AUTHORIZER (object);
  GError *error = NULL;
  GTask *task = user_data;
  MsgUploader *self = g_task_get_task_data (task);
  MsgUploaderPrivate *priv = self->priv;

  msg_authorizer_refresh_authorization_finish (authorizer, result, &error);
  if (error) {
    g_debug ("Error returned when refreshing authorization: %s",
             error->message);
    g_error_free (error);
  } else
    msg_authorizer_process_request (authorizer, priv->message);

  send_async_cb (task);
}

void
msg_uploader_send_async (MsgUploader         *self,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  MsgAuthorizer *authorizer;
  GTask *task;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, msg_uploader_send_async);
  g_task_set_task_data (task, g_object_ref (self), g_object_unref);

  self->priv->total_written = 0;

  authorizer = msg_service_get_authorizer (self->priv->service);
  if (authorizer) {
    g_autoptr (GError) error = NULL;

    msg_authorizer_refresh_authorization_async (authorizer, g_task_get_cancellable (task), refresh_auth_cb, task);
    return;
  }

  send_async_cb (task);
}

GBytes *
msg_uploader_send_finish (MsgUploader   *self,
                          GAsyncResult  *result,
                          gsize         *total_written,
                          GError       **error)
{
  GBytes *ret;

  g_return_val_if_fail (MSG_IS_UPLOADER (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_async_result_is_tagged (result, msg_uploader_send_async), NULL);

  ret = g_task_propagate_pointer (G_TASK (result), error);
  if (total_written)
    *total_written = ret ? g_bytes_get_size (ret) : 0;

  return ret;
}

/**
 * msg_uploader_get_service:
 * @self: a #MsgUploader
 *
 * Gets the service used to authorize the upload, as passed to msg_uploader_new().
 *
 * Return value: (transfer none): the #MsgService used to authorize the upload
 *
 * Since: 0.5.0
 */
MsgService *
msg_uploader_get_service (MsgUploader *self)
{
  g_return_val_if_fail (MSG_IS_UPLOADER (self), NULL);
  return self->priv->service;
}

/**
 * msg_uploader_get_method:
 * @self: a #MsgUploader
 *
 * Gets the HTTP request method being used to upload the file, as passed to msg_uploader_new().
 *
 * Return value: the HTTP request method in use
 *
 * Since: 0.7.0
 */
const gchar *
msg_uploader_get_method (MsgUploader *self)
{
  g_return_val_if_fail (MSG_IS_UPLOADER (self), NULL);
  return self->priv->method;
}

/**
 * msg_uploader_get_upload_uri:
 * @self: a #MsgUploader
 *
 * Gets the URI the file is being uploaded to, as passed to msg_uploader_new().
 *
 * Return value: the URI which the file is being uploaded to
 *
 * Since: 0.5.0
 */
const gchar *
msg_uploader_get_upload_uri (MsgUploader *self)
{
  g_return_val_if_fail (MSG_IS_UPLOADER (self), NULL);
  return self->priv->upload_uri;
}

/**
 * msg_uploader_get_slug:
 * @self: a #MsgUploader
 *
 * Gets the slug (filename) of the file being uploaded.
 *
 * Return value: the slug of the file being uploaded
 *
 * Since: 0.5.0
 */
const gchar *
msg_uploader_get_slug (MsgUploader *self)
{
  g_return_val_if_fail (MSG_IS_UPLOADER (self), NULL);
  return self->priv->slug;
}

/**
 * msg_uploader_get_content_type:
 * @self: a #MsgUploader
 *
 * Gets the content type of the file being uploaded.
 *
 * Return value: the content type of the file being uploaded
 *
 * Since: 0.5.0
 */
const gchar *
msg_uploader_get_content_type (MsgUploader *self)
{
  g_return_val_if_fail (MSG_IS_UPLOADER (self), NULL);
  return self->priv->content_type;
}

/**
 * msg_uploader_get_content_length:
 * @self: a #MsgUploader
 *
 * Gets the size (in bytes) of the file being uploaded. This will be <code class="literal">-1</code> for a non-resumable upload, and zero or greater
 * for a resumable upload.
 *
 * Return value: the size of the file being uploaded
 *
 * Since: 0.13.0
 */
goffset
msg_uploader_get_content_length (MsgUploader *self)
{
  g_return_val_if_fail (MSG_IS_UPLOADER (self), -1);
  return self->priv->content_length;
}

/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#undef  G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"SfiGlueCodec"
#include "sfigluecodec.h"
#include "sfiglueproxy.h"
#include "sfistore.h"

#include <string.h>


typedef struct {
  SfiGlueContext       context;
  SfiGlueCodecHandleIO io_handler;
  gpointer             user_data;
  GDestroyNotify       destroy;
  GScanner            *scanner;
} CodecContext;

typedef struct {
  SfiGlueCodec *codec;
  gulong        proxy;
  gchar        *signal;
  gulong        id;
} CodecSignal;


/* --- prototypes --- */
static SfiGlueIFace*  encode_describe_iface		(SfiGlueContext *context,
							 const gchar    *iface);
static SfiGlueProc*   encode_describe_proc		(SfiGlueContext *context,
							 const gchar    *proc_name);
static gchar**	      encode_list_proc_names		(SfiGlueContext *context);
static gchar**	      encode_list_method_names		(SfiGlueContext *context,
							 const gchar    *iface_name);
static gchar*	      encode_base_iface			(SfiGlueContext *context);
static gchar**	      encode_iface_children		(SfiGlueContext *context,
							 const gchar    *iface_name);
static GValue*	      encode_exec_proc			(SfiGlueContext *context,
							 const gchar    *proc_name,
							 SfiSeq         *params);
static gchar*	      encode_proxy_iface		(SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean	      encode_proxy_is_a			(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *iface);
static gchar**	      encode_proxy_list_properties	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *first_ancestor,
							 const gchar    *last_ancestor);
static GParamSpec*    encode_proxy_get_pspec		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop_name);
static SfiSCategory   encode_proxy_get_pspec_scategory	(SfiGlueContext *context,
							  SfiProxy        proxy,
							  const gchar    *prop_name);
static void	      encode_proxy_set_property		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop,
							 const GValue   *value);
static GValue*	      encode_proxy_get_property		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop);
static gboolean	      encode_proxy_watch_release	(SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean	      encode_proxy_notify		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *signal,
							 gboolean        enable_notify);
static GValue*	      encode_client_msg			(SfiGlueContext *context,
							 const gchar    *msg,
							 GValue         *value);


/* --- functions --- */
SfiGlueCodec*
sfi_glue_codec_new (SfiGlueContext       *context,
		    SfiGlueCodecSendEvent send_event,
		    SfiGlueCodecClientMsg client_msg)
{
  SfiGlueCodec *c;
  
  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (send_event != NULL, NULL);
  
  c = g_new0 (SfiGlueCodec, 1);
  c->user_data = NULL;
  c->context = context;
  c->destroy = NULL;
  c->send_event = send_event;
  c->client_msg = client_msg;
  // FIXME: c->scanner = g_scanner_new (sfi_rstore_scanner_config);
  // c->scanner->input_name = "SerializedCommand";
  
  return c;
}

void
sfi_glue_codec_set_user_data (SfiGlueCodec  *codec,
			      gpointer       user_data,
			      GDestroyNotify destroy)
{
  GDestroyNotify odestroy;
  gpointer ouser_data;
  
  g_return_if_fail (codec != NULL);
  
  odestroy = codec->destroy;
  ouser_data = codec->user_data;
  codec->user_data = user_data;
  codec->destroy = destroy;
  if (odestroy)
    odestroy (ouser_data);
}

void
sfi_glue_codec_destroy (SfiGlueCodec *codec)
{
  GDestroyNotify destroy;
  gpointer user_data;
  
  g_return_if_fail (codec != NULL);
  
  destroy = codec->destroy;
  user_data = codec->user_data;
  sfi_glue_context_push (codec->context);
  while (codec->signals)
    {
      CodecSignal *sig = codec->signals->data;
      // guint id = sig->id;
      
      sig->id = 0;
      // FIXME: sfi_glue_signal_disconnect (sig->signal, sig->proxy, id);
    }
  sfi_glue_context_pop ();
  // FIXME: g_scanner_destroy (codec->scanner);
  g_free (codec);
  
  if (destroy)
    destroy (user_data);
}

SfiGlueContext*
sfi_glue_codec_context (SfiGlueCodecHandleIO handle_io,
			gpointer             user_data,
			GDestroyNotify       destroy)
{
  static const SfiGlueContextTable encoder_vtable = {
    encode_describe_iface,
    encode_describe_proc,
    encode_list_proc_names,
    encode_list_method_names,
    encode_base_iface,
    encode_iface_children,
    encode_exec_proc,
    encode_proxy_iface,
    encode_proxy_is_a,
    encode_proxy_list_properties,
    encode_proxy_get_pspec,
    encode_proxy_get_pspec_scategory,
    encode_proxy_set_property,
    encode_proxy_get_property,
    encode_proxy_watch_release,
    encode_proxy_notify,
    encode_client_msg,
  };
  CodecContext *c;
  
  g_return_val_if_fail (handle_io != NULL, NULL);
  
  c = g_new0 (CodecContext, 1);
  sfi_glue_context_common_init (&c->context, &encoder_vtable);
  c->user_data = user_data;
  c->destroy = destroy;
  c->io_handler = handle_io;
  // FIXME: c->scanner = g_scanner_new (sfi_rstore_scanner_config);
  c->scanner->input_name = "SerializedResponse";
  
  return &c->context;
}

static SfiSeq*
request_exec_remote (SfiGlueContext *context,
		     SfiSeq         *seq)
{
#if 0
  gchar *response;
  
  response = ((CodecContext*) context)->io_handler (((CodecContext*) context)->user_data, gstring->str);
  g_string_free (gstring, TRUE);
  
  if (response)
    {
      /* check response */
      if (strncmp (response, ";sfi-glue-return\n", 17) != 0)
        {
          g_message ("received bad glue response: %s", response);
          g_free (response);
          response = NULL;
        }
      else
        g_memmove (response, response + 17, strlen (response) - 17 + 1);
    }
  else
    g_message ("received NULL glue response");
  
#endif
  return seq;
}

static SfiGlueIFace*
encode_describe_iface (SfiGlueContext *context,
		       const gchar    *iface_name)
{
  SfiGlueIFace *iface = NULL;
  SfiRec *rec;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_DESCRIBE_IFACE);
  sfi_seq_append_string (seq, iface_name);

  seq = request_exec_remote (context, seq);

  rec = sfi_seq_get_rec (seq, 0);
  if (rec)
    {
      iface = _sfi_glue_iface_new (sfi_rec_get_string (rec, "type_name"));
      iface->ifaces = sfi_seq_to_strv (sfi_rec_get_seq (rec, "ifaces"));
      iface->n_ifaces = g_strlenv (iface->ifaces);
      iface->props = sfi_seq_to_strv (sfi_rec_get_seq (rec, "props"));
      iface->n_props = g_strlenv (iface->props);
    }
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return iface;
}

static GValue*
decode_describe_iface (SfiGlueCodec *codec,
		       SfiSeq       *seq)
{
  SfiGlueIFace *iface = sfi_glue_describe_iface (sfi_seq_get_string (seq, 1));
  GValue *rvalue = NULL;
  SfiRec *rec = NULL;
  if (iface)
    {
      SfiSeq *seq;
      rec = sfi_rec_new ();
      sfi_rec_set_string (rec, "type_name", iface->type_name);
      seq = sfi_seq_from_strv (iface->ifaces);
      sfi_rec_set_seq (rec, "ifaces", seq);
      sfi_seq_unref (seq);
      seq = sfi_seq_from_strv (iface->props);
      sfi_rec_set_seq (rec, "props", seq);
      sfi_seq_unref (seq);
    }
  rvalue = sfi_value_rec (rec);
  sfi_glue_gc_free_now (iface, sfi_glue_iface_unref);
  return rvalue;
}

static SfiGlueProc*
encode_describe_proc (SfiGlueContext *context,
		      const gchar    *proc_name)
{
  SfiGlueProc *proc = NULL;
  SfiRec *rec;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_DESCRIBE_PROC);
  sfi_seq_append_string (seq, proc_name);

  seq = request_exec_remote (context, seq);

  rec = sfi_seq_get_rec (seq, 0);
  if (rec)
    {
      SfiSeq *pseq;
      GParamSpec *pspec;
      proc = _sfi_glue_proc_new ();
      proc->proc_name = g_strdup (sfi_rec_get_string (rec, "proc_name"));
      pseq = sfi_rec_get_seq (rec, "params");
      if (pseq)
	{
	  guint i;
	  for (i = 0; i < pseq->n_elements; i++)
	    _sfi_glue_proc_add_param (proc, sfi_seq_get_pspec (pseq, i));
	}
      pspec = sfi_rec_get_pspec (rec, "ret_param");
      if (pspec)
	_sfi_glue_proc_add_ret_param (proc, pspec);
    }
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return proc;
}

static GValue*
decode_describe_proc (SfiGlueCodec *codec,
		      SfiSeq       *seq)
{
  SfiGlueProc *proc = sfi_glue_describe_proc (sfi_seq_get_string (seq, 1));
  GValue *rvalue = NULL;
  SfiRec *rec = NULL;
  if (proc)
    {
      rec = sfi_rec_new ();
      sfi_rec_set_string (rec, "proc_name", proc->proc_name);
      if (proc->ret_param)
	sfi_rec_set_pspec (rec, "ret_param", proc->ret_param);
      if (proc->params)
	{
	  SfiSeq *seq = sfi_seq_new ();
	  guint i;
	  for (i = 0; i < proc->n_params; i++)
	    sfi_seq_append_pspec (seq, proc->params[i]);
	  sfi_rec_set_seq (rec, "params", seq);
	  sfi_seq_unref (seq);
	}
    }
  rvalue = sfi_value_rec (rec);
  sfi_glue_gc_free_now (proc, sfi_glue_proc_unref);
  return rvalue;
}

static gchar**
encode_list_proc_names (SfiGlueContext *context)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_LIST_PROC_NAMES);

  seq = request_exec_remote (context, seq);

  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return strv;
}

static GValue*
decode_list_proc_names (SfiGlueCodec *codec,
			SfiSeq       *seq)
{
  gchar **names = sfi_glue_list_proc_names ();
  GValue *rvalue = NULL;
  seq = sfi_seq_from_strv (names);
  rvalue = sfi_value_seq (seq);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static gchar**
encode_list_method_names (SfiGlueContext *context,
			  const gchar    *iface_name)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_LIST_METHOD_NAMES);
  sfi_seq_append_string (seq, iface_name);
  
  seq = request_exec_remote (context, seq);

  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return strv;
}

static GValue*
decode_list_method_names (SfiGlueCodec *codec,
			  SfiSeq       *seq)
{
  gchar **names = sfi_glue_list_method_names (sfi_seq_get_string (seq, 1));
  GValue *rvalue = NULL;
  seq = sfi_seq_from_strv (names);
  rvalue = sfi_value_seq (seq);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static gchar*
encode_base_iface (SfiGlueContext *context)
{
  gchar *string = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_BASE_IFACE);

  seq = request_exec_remote (context, seq);

  string = g_strdup (sfi_seq_get_string (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return string;
}

static GValue*
decode_base_iface (SfiGlueCodec *codec,
		   SfiSeq       *seq)
{
  gchar *name = sfi_glue_base_iface ();
  GValue *rvalue = NULL;
  rvalue = sfi_value_string (name);
  sfi_glue_gc_free_now (name, g_free);
  return rvalue;
}

static gchar**
encode_iface_children (SfiGlueContext *context,
		       const gchar    *iface_name)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_IFACE_CHILDREN);
  sfi_seq_append_string (seq, iface_name);
  
  seq = request_exec_remote (context, seq);

  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return strv;
}

static GValue*
decode_iface_children (SfiGlueCodec *codec,
		       SfiSeq       *seq)
{
  gchar **names = sfi_glue_iface_children (sfi_seq_get_string (seq, 1));
  GValue *rvalue = NULL;
  seq = sfi_seq_from_strv (names);
  rvalue = sfi_value_seq (seq);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static GValue*
encode_exec_proc (SfiGlueContext *context,
		  const gchar    *proc_name,
		  SfiSeq         *params)
{
  GValue *rvalue = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_EXEC_PROC);
  sfi_seq_append_string (seq, proc_name);
  sfi_seq_append_seq (seq, params);

  seq = request_exec_remote (context, seq);

  if (seq->n_elements)
    rvalue = sfi_value_clone_shallow (sfi_seq_get (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return rvalue;
}

static GValue*
decode_exec_proc (SfiGlueCodec *codec,
		  SfiSeq       *seq)
{
  GValue *pvalue = sfi_glue_call_seq (sfi_seq_get_string (seq, 1),
				      sfi_seq_get_seq (seq, 2));
  if (pvalue)
    sfi_glue_gc_remove (pvalue, sfi_value_free);
  return pvalue;
}

static gchar*
encode_proxy_iface (SfiGlueContext *context,
		    SfiProxy        proxy)
{
  gchar *string = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_IFACE);
  sfi_seq_append_proxy (seq, proxy);
  
  seq = request_exec_remote (context, seq);

  string = g_strdup (sfi_seq_get_string (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return string;
}

static GValue*
decode_proxy_iface (SfiGlueCodec *codec,
		    SfiSeq       *seq)
{
  const gchar *name = sfi_glue_proxy_iface (sfi_seq_get_proxy (seq, 1));
  GValue *rvalue = NULL;
  rvalue = sfi_value_string (name);
  sfi_glue_gc_free_now ((char*) name, g_free);
  return rvalue;
}

static gboolean
encode_proxy_is_a (SfiGlueContext *context,
		   SfiProxy        proxy,
		   const gchar    *iface)
{
  gboolean vbool;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_IS_A);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, iface);

  seq = request_exec_remote (context, seq);

  vbool = sfi_seq_get_bool (seq, 0);
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return vbool;
}

static GValue*
decode_proxy_is_a (SfiGlueCodec *codec,
		   SfiSeq       *seq)
{
  gboolean vbool = sfi_glue_proxy_is_a (sfi_seq_get_proxy (seq, 1),
					sfi_seq_get_string (seq, 2));
  return sfi_value_bool (vbool);
}

static gchar**
encode_proxy_list_properties (SfiGlueContext *context,
			      SfiProxy        proxy,
			      const gchar    *first_ancestor,
			      const gchar    *last_ancestor)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_LIST_PROPERTIES);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, first_ancestor);
  sfi_seq_append_string (seq, last_ancestor);

  seq = request_exec_remote (context, seq);

  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return strv;
}

static GValue*
decode_proxy_list_properties (SfiGlueCodec *codec,
			      SfiSeq       *seq)
{
  const gchar **names = sfi_glue_proxy_list_properties (sfi_seq_get_proxy (seq, 1),
							sfi_seq_get_string (seq, 2),
							sfi_seq_get_string (seq, 3),
							NULL);
  GValue *rvalue = NULL;
  seq = sfi_seq_from_strv ((gchar**) names);
  rvalue = sfi_value_seq (seq);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static GParamSpec*
encode_proxy_get_pspec (SfiGlueContext *context,
			SfiProxy        proxy,
			const gchar    *prop_name)
{
  GParamSpec *pspec = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_GET_PSPEC);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop_name);

  seq = request_exec_remote (context, seq);

  pspec = sfi_seq_get_pspec (seq, 0);
  if (pspec)
    g_param_spec_ref (pspec);
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return pspec;
}

static GValue*
decode_proxy_get_pspec (SfiGlueCodec *codec,
			SfiSeq       *seq)
{
  GParamSpec *pspec = sfi_glue_proxy_get_pspec (sfi_seq_get_proxy (seq, 1),
						sfi_seq_get_string (seq, 2));
  GValue *rvalue = NULL;
  rvalue = sfi_value_pspec (pspec);
  sfi_glue_gc_free_now (pspec, g_param_spec_unref);
  return rvalue;
}

static SfiSCategory
encode_proxy_get_pspec_scategory (SfiGlueContext *context,
				  SfiProxy        proxy,
				  const gchar    *prop_name)
{
  SfiSCategory scat;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_GET_PSPEC_SCATEGORY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop_name);

  seq = request_exec_remote (context, seq);

  scat = sfi_seq_get_int (seq, 0);
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return scat;
}

static GValue*
decode_proxy_get_pspec_scategory (SfiGlueCodec *codec,
				  SfiSeq       *seq)
{
  SfiSCategory scat = sfi_glue_proxy_get_pspec_scategory (sfi_seq_get_proxy (seq, 1),
							  sfi_seq_get_string (seq, 2));
  return sfi_value_int (scat);
}

static void
encode_proxy_set_property (SfiGlueContext *context,
			   SfiProxy        proxy,
			   const gchar    *prop,
			   const GValue   *value)
{
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_SET_PROPERTY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop);
  sfi_seq_append (seq, value);

  seq = request_exec_remote (context, seq);
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
}

static GValue*
decode_proxy_set_property (SfiGlueCodec *codec,
			   SfiSeq       *seq)
{
  if (seq->n_elements >= 3)
    sfi_glue_proxy_set_property (sfi_seq_get_proxy (seq, 1),
				 sfi_seq_get_string (seq, 2),
				 sfi_seq_get (seq, 3));
  return NULL;
}

static GValue*
encode_proxy_get_property (SfiGlueContext *context,
			   SfiProxy        proxy,
			   const gchar    *prop)
{
  GValue *rvalue = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_GET_PROPERTY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop);

  seq = request_exec_remote (context, seq);

  if (seq->n_elements)
    rvalue = sfi_value_clone_shallow (sfi_seq_get (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return rvalue;
}

static GValue*
decode_proxy_get_property (SfiGlueCodec *codec,
			   SfiSeq       *seq)
{
  GValue *pvalue = (GValue*) sfi_glue_proxy_get_property (sfi_seq_get_proxy (seq, 1),
							  sfi_seq_get_string (seq, 1));
  if (pvalue)
    sfi_glue_gc_remove (pvalue, sfi_value_free);
  return pvalue;
}

static gboolean
encode_proxy_watch_release (SfiGlueContext *context,
			    SfiProxy        proxy)
{
  gboolean vbool;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_WATCH_RELEASE);
  sfi_seq_append_proxy (seq, proxy);

  seq = request_exec_remote (context, seq);

  vbool = sfi_seq_get_bool (seq, 0);
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return vbool;
}

static GValue*
decode_proxy_watch_release (SfiGlueCodec *codec,
			    SfiSeq       *seq)
{
  gboolean vbool = _sfi_glue_proxy_watch_release (sfi_seq_get_proxy (seq, 1));
  return sfi_value_bool (vbool);
}

static gboolean
encode_proxy_notify (SfiGlueContext *context,
		     SfiProxy        proxy,
		     const gchar    *signal,
		     gboolean        enable_notify)
{
  gboolean vbool;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_NOTIFY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, signal);
  sfi_seq_append_bool (seq, enable_notify);

  seq = request_exec_remote (context, seq);

  vbool = sfi_seq_get_bool (seq, 0);
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return vbool;
}

static GValue*
decode_proxy_notify (SfiGlueCodec *codec,
		     SfiSeq       *seq)
{
  gboolean vbool = _sfi_glue_proxy_notify (sfi_seq_get_proxy (seq, 1),
					   sfi_seq_get_string (seq, 2),
					   sfi_seq_get_bool (seq, 3));
  return sfi_value_bool (vbool);
}

static GValue*
encode_client_msg (SfiGlueContext *context,
		   const gchar    *msg,
		   GValue         *value)
{
  GValue *rvalue = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_CLIENT_MSG);
  sfi_seq_append_string (seq, msg);
  sfi_seq_append (seq, value);
  
  seq = request_exec_remote (context, seq);

  if (seq->n_elements)
    rvalue = sfi_value_clone_shallow (sfi_seq_get (seq, 0));
  sfi_glue_gc_free_now (seq, sfi_seq_unref);
  return rvalue;
}

static GValue*
decode_client_msg (SfiGlueCodec *codec,
		   SfiSeq       *seq)
{
  const gchar *msg = sfi_seq_get_string (seq, 1);
  GValue *cvalue = sfi_seq_get (seq, 2);
  GValue *rvalue = NULL;
  gboolean handled = FALSE;
  
  /* interception handler */
  if (codec->client_msg)
    {
      rvalue = codec->client_msg (codec, codec->user_data, msg, cvalue, &handled);
      if (!handled && rvalue)
	sfi_value_free (rvalue);
    }
  
  /* regular handling */
  if (!handled)
    rvalue = sfi_glue_client_msg (msg, cvalue);
  
  return rvalue;
}

static GValue*
decode_process_command (SfiGlueCodec *codec,
			SfiSeq       *seq)
{
  SfiInt cmd;
  if (!seq || seq->n_elements < 1)
    return NULL;

  cmd = sfi_seq_get_int (seq, 0);

  switch (cmd)
    {
    case SFI_GLUE_CODEC_DESCRIBE_IFACE:
      return decode_describe_iface (codec, seq);
    case SFI_GLUE_CODEC_DESCRIBE_PROC:
      return decode_describe_proc (codec, seq);
    case SFI_GLUE_CODEC_LIST_PROC_NAMES:
      return decode_list_proc_names (codec, seq);
    case SFI_GLUE_CODEC_LIST_METHOD_NAMES:
      return decode_list_method_names (codec, seq);
    case SFI_GLUE_CODEC_BASE_IFACE:
      return decode_base_iface (codec, seq);
    case SFI_GLUE_CODEC_IFACE_CHILDREN:
      return decode_iface_children (codec, seq);
    case SFI_GLUE_CODEC_EXEC_PROC:
      return decode_exec_proc (codec, seq);
    case SFI_GLUE_CODEC_PROXY_IFACE:
      return decode_proxy_iface (codec, seq);
    case SFI_GLUE_CODEC_PROXY_IS_A:
      return decode_proxy_is_a (codec, seq);
    case SFI_GLUE_CODEC_PROXY_LIST_PROPERTIES:
      return decode_proxy_list_properties (codec, seq);
    case SFI_GLUE_CODEC_PROXY_GET_PSPEC:
      return decode_proxy_get_pspec (codec, seq);
    case SFI_GLUE_CODEC_PROXY_GET_PSPEC_SCATEGORY:
      return decode_proxy_get_pspec_scategory (codec, seq);
    case SFI_GLUE_CODEC_PROXY_SET_PROPERTY:
      return decode_proxy_set_property (codec, seq);
    case SFI_GLUE_CODEC_PROXY_GET_PROPERTY:
      return decode_proxy_get_property (codec, seq);
    case SFI_GLUE_CODEC_PROXY_WATCH_RELEASE:
      return decode_proxy_watch_release (codec, seq);
    case SFI_GLUE_CODEC_PROXY_NOTIFY:
      return decode_proxy_notify (codec, seq);
    case SFI_GLUE_CODEC_CLIENT_MSG:
      return decode_client_msg (codec, seq);
    }
  sfi_warn ("invalid request ID: %d", cmd);
  return NULL;
}


gchar*
sfi_glue_codec_process (SfiGlueCodec *codec,
                        const gchar  *message)
{
#if 0
  static GScanner *scanner = NULL;
  GString *gstring;

  /* here, we are processing incoming requests from remote.
   * after decoding the request, we invoke actual glue layer functions
   * and encode the various return values to pass them back on to
   * the remote client
   */
  
  g_return_val_if_fail (codec != NULL, NULL);
  g_return_val_if_fail (message != NULL, NULL);
  
  sfi_glue_context_push (codec->context);
  scanner = codec->scanner;
  scanner->user_data = NULL;
  g_scanner_input_text (scanner, message, strlen (message));
  
  gstring = g_string_new (";sfi-glue-return\n");
  decode_process_command (codec, gstring);
  
  if (scanner->user_data)
    {
      g_scanner_unexp_token (scanner, (GTokenType) scanner->user_data, NULL, NULL, NULL, "aborting...", TRUE);
      g_string_assign (gstring, ";sfi-glue-parse-error");
    }
  g_scanner_input_text (scanner, NULL, 0);
  sfi_glue_context_pop ();
  
  return g_string_free (gstring, FALSE);
#endif
  return NULL;
}


#if 0

static void
gstring_add_signal (GString     *gstring,
		    const gchar *signal,
		    gulong       proxy,
		    SfiGlueSeq  *args,
		    gboolean     connected)
{
  SfiGlueValue *value;
  
  g_string_printfa (gstring, "(%u ", SFI_GLUE_CODEC_EVENT_SIGNAL);
  g_string_add_escstr (gstring, signal);
  g_string_printfa (gstring, " %lu ", proxy);
  
  value = sfi_glue_value_seq (args);
  g_string_add_value (gstring, value);
  sfi_glue_gc_collect_value (value);
  
  g_string_printfa (gstring, " %u)", connected);
}

static gboolean       /* returns whether there was an event */
decode_event (SfiGlueContext *context,
	      gchar         **signal,
	      gulong	     *proxy_p,
	      SfiGlueSeq    **args,
	      gboolean       *connected)
{
  GScanner *scanner = ((CodecContext*) context)->scanner;
  SfiGlueValue *value;
  gulong proxy;
  
  parse_or_return (scanner, '(', FALSE);
  parse_or_return (scanner, G_TOKEN_INT, FALSE);
  if (scanner->value.v_int != SFI_GLUE_CODEC_EVENT_SIGNAL)
    return FALSE;
  
  parse_or_return (scanner, G_TOKEN_STRING, TRUE);
  *signal = g_strdup (scanner->value.v_string);
  parse_or_return (scanner, G_TOKEN_INT, TRUE);
  proxy = scanner->value.v_int;
  value = parse_value (scanner);
  checkok_or_return (scanner, TRUE);
  if (value->glue_type != SFI_GLUE_TYPE_SEQ)
    {
      sfi_glue_gc_collect_value (value);
      return TRUE;
    }
  else
    {
      *args = sfi_glue_seq_ref (value->value.v_seq);
      sfi_glue_gc_collect_value (value);
    }
  parse_or_return (scanner, G_TOKEN_INT, TRUE);
  *connected = scanner->value.v_int;
  parse_or_return (scanner, ')', TRUE);
  
  *proxy_p = proxy;	/* success */
  return TRUE;
}

gboolean	/* returns whether message contains an event */
sfi_glue_codec_enqueue_event (SfiGlueContext *context,
			      const gchar    *message)
{
  GScanner *scanner;
  gchar *signal = NULL;
  gulong proxy = 0;
  SfiGlueSeq *args = NULL;
  gboolean seen_event, connected = FALSE;
  
  g_return_val_if_fail (context != NULL, FALSE);
  g_return_val_if_fail (message != NULL, FALSE);
  
  scanner = encoder_setup_scanner (context, message);
  seen_event = decode_event (context, &signal, &proxy, &args, &connected);
  scanner_cleanup (scanner, NULL, G_STRLOC);
  
  if (proxy)
    sfi_glue_enqueue_signal_event (signal, args, !connected);
  g_free (signal);
  if (args)
    sfi_glue_seq_unref (args);
  
  return seen_event;
}
#endif

/* vim:set ts=8 sts=2 sw=2: */

/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bswproxy.h"

#include <bse/bse.h>
#include <gobject/gvaluecollector.h>


/* --- value type glue code --- */
static inline void
bsw_value_glue2bsw (const GValue *value,
		    GValue       *dest)
{
  if (G_VALUE_HOLDS_OBJECT (value))
    {
      GObject *object = g_value_get_object (value);

      if (value == dest)
	g_value_unset (dest);
      g_value_init (dest, SFI_TYPE_PROXY);
      g_value_set_pointer (dest, (gpointer) (BSE_IS_OBJECT (object) ? BSE_OBJECT_ID (object) : 0));
    }
  else if (value != dest)
    {
      g_value_init (dest, G_VALUE_TYPE (value));
      g_value_copy (value, dest);
    }
}

static inline void
bsw_value_glue2bse (const GValue *value,
		    GValue       *dest,
		    GType         object_type)
{
  if (G_VALUE_TYPE (value) == SFI_TYPE_PROXY)
    {
      guint object_id = sfi_value_get_proxy (value);

      if (value == dest)
	g_value_unset (dest);
      g_value_init (dest, object_type);
      g_value_set_object (dest, bse_object_from_id (object_id));
    }
  else if (value != dest)
    {
      g_value_init (dest, G_VALUE_TYPE (value));
      g_value_copy (value, dest);
    }
}

static inline GType
bsw_property_type_from_bse (GType bse_type)
{
  if (G_TYPE_IS_OBJECT (bse_type))
    return SFI_TYPE_PROXY;
  else
    return bse_type;
}


/* --- functions --- */
void
bsw_init (gint               *argc,
	  gchar             **argv[],
	  const BswLockFuncs *lock_funcs)
{
  static gboolean initialized = 0;
  BseLockFuncs lfuncs;

  g_return_if_fail (initialized++ == 0);

  if (lock_funcs)
    {
      g_return_if_fail (lock_funcs->lock != NULL);
      g_return_if_fail (lock_funcs->unlock != NULL);

      lfuncs.lock_data = lock_funcs->lock_data;
      lfuncs.lock = lock_funcs->lock;
      lfuncs.unlock = lock_funcs->unlock;
    }
  
  bse_init (argc, argv, lock_funcs ? &lfuncs : NULL);

  sfi_glue_context_push (bse_glue_context ());
}

SfiProxy
bsw_proxy_get_server (void)
{
  return BSE_OBJECT_ID (bse_server_get ());
}

void
bsw_proxy_set (SfiProxy     proxy,
	       const gchar *prop,
	       ...)
{
  va_list var_args;
  GObject *object = bse_object_from_id (proxy);

  g_return_if_fail (BSE_IS_ITEM (object));

  g_object_freeze_notify (object);
  va_start (var_args, prop);
  while (prop)
    {
      GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop);
      GValue value = { 0, };
      gchar *error = NULL;
      gboolean mask_proxy = FALSE;

      if (pspec)
	switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
	  {
	  case G_TYPE_CHAR: case G_TYPE_UCHAR: case G_TYPE_BOOLEAN: case G_TYPE_INT:
	  case G_TYPE_UINT: case G_TYPE_LONG: case G_TYPE_ULONG: case G_TYPE_ENUM:
	  case G_TYPE_FLAGS: case G_TYPE_FLOAT: case G_TYPE_DOUBLE: case G_TYPE_STRING:
	    // case BSE_TYPE_DOTS:
	    // case BSE_TYPE_TIME:
	    break;
	  default:
	    if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), BSE_TYPE_OBJECT))
	      mask_proxy = TRUE;
	    else
	      pspec = NULL;
	    break;
	  }
      if (!pspec)
	{
	  g_warning (G_STRLOC ": invalid property name `%s'", prop);
	  break;
	}
      g_value_init (&value, bsw_property_type_from_bse (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: failed to collect `%s': %s", G_STRLOC, pspec->name, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      bsw_value_glue2bse (&value, &value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_object_set_property (object, pspec->name, &value);
      g_value_unset (&value);
      prop = va_arg (var_args, gchar*);
    }
  va_end (var_args);
  g_object_thaw_notify (object);
}

GParamSpec*
bsw_proxy_get_pspec (SfiProxy     proxy,
		     const gchar *name)
{
  GObject *object;

  object = bse_object_from_id (proxy);

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  return g_object_class_find_property (G_OBJECT_GET_CLASS (object), name);
}

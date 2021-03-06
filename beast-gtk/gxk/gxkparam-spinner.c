/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */

/* --- spinnbutton editors --- */
enum {
  PARAM_SPINNER_LINEAR,
  PARAM_SPINNER_LOGARITHMIC,
};

static GtkWidget*
param_spinner_create (GxkParam    *param,
                      const gchar *tooltip,
                      guint        variant)
{
  GtkAdjustment *adjustment = NULL;
  /* figure normal/log/db value scaled adjustment */
  if (variant == PARAM_SPINNER_LOGARITHMIC)
    adjustment = gxk_param_get_log_adjustment (param);
  if (variant == PARAM_SPINNER_LINEAR &&
      (g_param_spec_check_option (param->pspec, "db-volume") ||
       g_param_spec_check_option (param->pspec, "db-range")) &&
      !g_param_spec_check_option (param->pspec, "db-value"))
    adjustment = gxk_param_get_decibel_adjustment (param);
  if (!adjustment)
    adjustment = gxk_param_get_adjustment (param);
  /* figure chars & digits by type */
  const GxkParamEditorSizes *esize = gxk_param_get_editor_sizes (param);
  guint chars = 1, digits = 0, fracts = 0;
  switch (G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (&param->value)))
    {
    case G_TYPE_CHAR:   chars = esize->char_chars;   digits = esize->char_digits;   break;
    case G_TYPE_UCHAR:  chars = esize->uchar_chars;  digits = esize->uchar_digits;  break;
    case G_TYPE_INT:    chars = esize->int_chars;    digits = esize->int_digits;    break;
    case G_TYPE_UINT:   chars = esize->uint_chars;   digits = esize->uint_digits;   break;
    case G_TYPE_LONG:   chars = esize->long_chars;   digits = esize->long_digits;   break;
    case G_TYPE_ULONG:  chars = esize->ulong_chars;  digits = esize->ulong_digits;  break;
    case G_TYPE_INT64:  chars = esize->int64_chars;  digits = esize->int64_digits;  break;
    case G_TYPE_UINT64: chars = esize->uint64_chars; digits = esize->uint64_digits; break;
    case G_TYPE_FLOAT:  chars = esize->float_chars;  digits = esize->float_digits;  fracts = 7;  break;
    case G_TYPE_DOUBLE: chars = esize->double_chars; digits = esize->double_digits; fracts = 17; break;
    }
  /* adjust digits by sign */
  if (esize->may_resize &&      /* may chars be modified? */
      adjustment->lower >= 0 && adjustment->upper >= 0 && chars >= 1)
    chars -= 1; /* no sign */
  /* shrink fractions by semantic information */
  if (TRUE &&                   /* fractions may always be modified */
      (g_param_spec_check_option (param->pspec, "db-volume") ||
       g_param_spec_check_option (param->pspec, "db-range") ||
       g_param_spec_check_option (param->pspec, "db-value")))
    fracts = MIN (fracts, 6);   /* fractional digits don't make too much sense for decibel values */
  /* shrink digits by range */
  if (esize->may_resize)        /* may digits be modified? */
    {
      double maxvalue = MAX (ABS (adjustment->lower), ABS (adjustment->upper));
      guint vdigits = gxk_param_get_digits (maxvalue, 10);
      digits = MIN (digits, vdigits);
    }
  /* creation & setup */
  GtkWidget *widget = g_object_new (GTK_TYPE_SPIN_BUTTON,
                                    "visible", TRUE,
                                    "activates_default", TRUE,
                                    "adjustment", adjustment,
                                    "digits", fracts,
                                    "width_chars", 0,
                                    NULL);
  gxk_param_add_grab_widget (param, widget);
  gxk_widget_add_font_requisition (widget, chars, digits + (esize->request_fractions ? fracts : 0));
  gxk_param_entry_connect_handlers (param, widget, NULL);
  gxk_widget_set_tooltip (widget, tooltip);
  return widget;
}

static void
param_spinner_update (GxkParam  *param,
                      GtkWidget *widget)
{
  /* contents are updated through the adjustment */
  gtk_editable_set_editable (GTK_EDITABLE (widget), param->editable);
}

static GxkParamEditor param_spinner1 = {
  { "spinner",          N_("Spin Button"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +9,   TRUE, },        /* options, rating, editing */
  param_spinner_create, param_spinner_update, PARAM_SPINNER_LINEAR,
};
static GxkParamEditor param_spinner2 = {
  { "spinner-log",      N_("Spin Button (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +4,   TRUE, },        /* options, rating, editing */
  param_spinner_create, param_spinner_update, PARAM_SPINNER_LOGARITHMIC,
};

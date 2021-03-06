/* BEAST - Better Audio System
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


/* --- choice parameters --- */
#define CHOICE_PARAM_OPTION_MENU        GXK_MENU_BUTTON_POPUP_MODE

static void
param_choice_change_value (GtkWidget *widget,
			   GxkParam  *param)
{
  if (!param->updating)
    {
      GtkWidget *item;
      if (GXK_IS_MENU_BUTTON (widget))
        item = GXK_MENU_BUTTON (widget)->menu_item;
      else
        item = GTK_OPTION_MENU (widget)->menu_item;
      if (item)
	{
          SfiChoiceValue *cv = g_object_get_qdata (G_OBJECT (item), quark_param_choice_values);
	  sfi_value_set_choice (&param->value, cv->choice_ident);
	}
      gxk_param_apply_value (param);
    }
}

static void
param_choice_item_activated (GtkWidget *menu_item,
                             GtkWidget *widget)
{
  GtkMenuShell *mshell = GTK_MENU_SHELL (menu_item->parent);
  if (mshell)
    gxk_menu_set_active (GTK_MENU (mshell), menu_item);
}

static GtkWidget*
param_choice_create (GxkParam    *param,
                     const gchar *tooltip,
                     guint        variant)
{
  SfiChoiceValues cvalues = sfi_pspec_get_choice_values (param->pspec);
  GtkWidget *widget;
  GtkContainer *menu;
  gchar *str;
  guint i;

  if (variant == CHOICE_PARAM_OPTION_MENU)
    widget = g_object_new (GTK_TYPE_OPTION_MENU,
                           "visible", TRUE,
                           NULL);
  else
    widget = g_object_new (GXK_TYPE_MENU_BUTTON,
                           "visible", TRUE,
                           "mode", variant,
                           "can-focus", variant == GXK_MENU_BUTTON_COMBO_MODE,
                           NULL);
  gxk_widget_set_tooltip (widget, tooltip);

  menu = g_object_new (GTK_TYPE_MENU, NULL);
  for (i = 0; i < cvalues.n_values; i++)
    {
      GtkWidget *item = gtk_menu_item_new_with_label (cvalues.values[i].choice_label);
      if (cvalues.values[i].choice_blurb && cvalues.values[i].choice_blurb[0])
        gxk_widget_set_tooltip (item, cvalues.values[i].choice_blurb);
      gtk_widget_show (item);
      g_object_connect (item, "signal::activate", param_choice_item_activated, widget, NULL);
      g_object_set_qdata (G_OBJECT (item), quark_param_choice_values, (gpointer) &cvalues.values[i]);
      gtk_container_add (menu, item);
    }

  if (GXK_IS_MENU_BUTTON (widget))
    {
      SfiProxy proxy = bst_param_get_proxy (param);
      g_object_set (widget, "menu", menu, NULL);
      str = g_strdup_printf ("<BEAST-ParamChoice>/%s(%s::%llx)",
                             param->pspec->name,
                             proxy ? bse_item_get_type (proxy) : "0",
                             sfi_pspec_get_choice_hash (param->pspec));
      gtk_menu_set_accel_path (GTK_MENU (menu), str);
      g_free (str);
    }
  else
    {
      gxk_option_menu_set_menu (GTK_OPTION_MENU (widget), GTK_MENU (menu));
      /* GtkOptionMenu doesn't display accelerators correctly */
    }
  g_object_connect (widget,
		    "signal::button_press_event", gxk_param_ensure_focus, NULL,
		    "signal::changed", param_choice_change_value, param,
		    NULL);
  return widget;
}

static void
param_choice_update (GxkParam  *param,
		     GtkWidget *widget)
{
  const gchar *string = sfi_value_get_choice (&param->value);
  GtkWidget *menu;
  if (GXK_IS_MENU_BUTTON (widget))
    menu = GTK_WIDGET (GXK_MENU_BUTTON (widget)->menu);
  else
    menu = GTK_OPTION_MENU (widget)->menu;
  if (menu && string)
    {
      GList *list;
      for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
	{
	  GtkWidget *item = list->data;
	  SfiChoiceValue *cv = g_object_get_qdata (G_OBJECT (item), quark_param_choice_values);
	  if (sfi_choice_match (cv->choice_ident, string))
            {
              param_choice_item_activated (item, widget);
              break;
	    }
	}
    }
  if (GXK_IS_MENU_BUTTON (widget))
    {
      /* force a menu button update even if param_choice_item_activated()
       * didn't change anything, to ensure correctly set tooltips.
       */
      gxk_menu_button_update (GXK_MENU_BUTTON (widget));
    }
}

static GxkParamEditor param_choice1 = {
  { "combo-button",     N_("Drop Down Combo"), },
  { G_TYPE_STRING,      "SfiChoice", },
  { NULL,       +8,     TRUE, },        /* options, rating, editing */
  param_choice_create,  param_choice_update,    GXK_MENU_BUTTON_COMBO_MODE
};
static GxkParamEditor param_choice2 = {
  { "tool-button",      N_("Drop Down Button"), },
  { G_TYPE_STRING,      "SfiChoice", },
  { NULL,       +7,     TRUE, },        /* options, rating, editing */
  param_choice_create,  param_choice_update,    GXK_MENU_BUTTON_TOOL_MODE
};
static GxkParamEditor param_choice3 = {
  { "choice-button",    N_("Popup Options"), },
  { G_TYPE_STRING,      "SfiChoice", },
  { NULL,       +6,     TRUE, },        /* options, rating, editing */
  param_choice_create,  param_choice_update,    GXK_MENU_BUTTON_OPTION_MODE
};
static GxkParamEditor param_choice4 = {
  { "choice-menu",      N_("Standard Option Menu"), },
  { G_TYPE_STRING,      "SfiChoice", },
  { NULL,       +5,     TRUE, },        /* options, rating, editing */
  param_choice_create,  param_choice_update,    CHOICE_PARAM_OPTION_MENU
};
static const gchar *param_choice_aliases1[] = {
  "choice",
  "choice-menu", "choice-button",
  NULL,
};

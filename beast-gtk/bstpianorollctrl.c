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
#include "bstpianorollctrl.h"
#include "bsteventrollctrl.h"


#define NOTE_LENGTH(self)       ((self)->note_rtools->action_id)
#define QUANTIZATION(self)      ((self)->quant_rtools->action_id)
#define HAVE_OBJECT             (1 << 31)


/* --- prototypes --- */
static gboolean bst_piano_roll_controller_check_action  (BstPianoRollController *self,
                                                         gulong                  action_id,
                                                         guint64                 action_stamp);
static void     bst_piano_roll_controller_exec_action   (BstPianoRollController *self,
                                                         gulong                  action_id);
static void	controller_canvas_drag		        (BstPianoRollController	*self,
                                                         BstPianoRollDrag	*drag);
static void	controller_piano_drag		        (BstPianoRollController	*self,
                                                         BstPianoRollDrag	*drag);
static void	controller_update_canvas_cursor	        (BstPianoRollController *self,
                                                         BstCommonRollTool	 tool);


/* --- variables --- */
static BsePartNoteSeq *clipboard_pseq = NULL;

/* --- actions --- */
enum {
  ACTION_NONE           = BST_COMMON_ROLL_TOOL_LAST,
  ACTION_SELECT_ALL,
  ACTION_SELECT_NONE,
  ACTION_SELECT_INVERT,
};

/* --- functions --- */
GxkActionList*
bst_piano_roll_controller_select_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create ();
  static const GxkStockAction actions[] = {
    { N_("All"),                "<ctrl>A",              N_("Select all notes"),
      ACTION_SELECT_ALL,        BST_STOCK_SELECT_ALL },
    { N_("None"),               "<shift><ctrl>A",       N_("Unselect all notes"),
      ACTION_SELECT_NONE,       BST_STOCK_SELECT_NONE },
    { N_("Invert"),             "<ctrl>I",              N_("Invert the current selection"),
      ACTION_SELECT_INVERT,     BST_STOCK_SELECT_INVERT },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/,
                               (GxkActionCheck) bst_piano_roll_controller_check_action,
                               (GxkActionExec) bst_piano_roll_controller_exec_action,
                               self);
  return alist;
}

GxkActionList*
bst_piano_roll_controller_canvas_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->canvas_rtools);
  static const GxkStockAction actions[] = {
    { N_("Insert"),           "I",    N_("Insert/resize/move notes (mouse button 1 and 2)"),
      BST_COMMON_ROLL_TOOL_INSERT,   BST_STOCK_PART_TOOL },
    { N_("Delete"),           "D",    N_("Delete note (mouse button 1)"),
      BST_COMMON_ROLL_TOOL_DELETE,   BST_STOCK_TRASHCAN },
    { N_("Align Events"),     "A",    N_("Draw a line to align events to"),
      BST_COMMON_ROLL_TOOL_ALIGN,    BST_STOCK_EVENT_CONTROL },
    { N_("Select"),           "S",    N_("Rectangle select notes"),
      BST_COMMON_ROLL_TOOL_SELECT,   BST_STOCK_RECT_SELECT },
    { N_("Vertical Select"),  "V",    N_("Select tick range vertically"),
      BST_COMMON_ROLL_TOOL_VSELECT,  BST_STOCK_VERT_SELECT },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

GxkActionList*
bst_piano_roll_controller_note_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->note_rtools);
  static const GxkStockAction actions[] = {
    { N_("1\\/1"),              "1",    N_("Insert whole notes"),
      1,                        BST_STOCK_NOTE_1, },
    { N_("1\\/2"),              "2",    N_("Insert half notes"),
      2,                        BST_STOCK_NOTE_2, },
    { N_("1\\/4"),              "4",    N_("Insert quarter notes"),
      4,                        BST_STOCK_NOTE_4, },
    { N_("1\\/8"),              "8",    N_("Insert eighths note"),
      8,                        BST_STOCK_NOTE_8, },
    { N_("1\\/16"),             "6",    N_("Insert sixteenth note"),
      16,                       BST_STOCK_NOTE_16, },
    { N_("1\\/32"),             "3",    N_("Insert thirty-second note"),
      32,                       BST_STOCK_NOTE_32, },
    { N_("1\\/64"),             "5",    N_("Insert sixty-fourth note"),
      64,                       BST_STOCK_NOTE_64, },
    { N_("1\\/128"),            "7",    N_("Insert hundred twenty-eighth note"),
      128,                      BST_STOCK_NOTE_128, },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

GxkActionList*
bst_piano_roll_controller_quant_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->quant_rtools);
  static const GxkStockAction actions[] = {
      { N_("Q: Tact"),          "<ctrl>T",      N_("Quantize to tact boundaries"),
        BST_QUANTIZE_TACT,      BST_STOCK_QTACT, },
      { N_("Q: None"),          "<ctrl>0",      N_("No quantization selected"),
        BST_QUANTIZE_NONE,      BST_STOCK_QNOTE_NONE, },
      { N_("Q: 1\\/1"),         "<ctrl>1",      N_("Quantize to whole note boundaries"),
        BST_QUANTIZE_NOTE_1,    BST_STOCK_QNOTE_1, },
      { N_("Q: 1\\/2"),         "<ctrl>2",      N_("Quantize to half note boundaries"),
        BST_QUANTIZE_NOTE_2,    BST_STOCK_QNOTE_2, },
      { N_("Q: 1\\/4"),         "<ctrl>4",      N_("Quantize to quarter note boundaries"),
        BST_QUANTIZE_NOTE_4,    BST_STOCK_QNOTE_4, },
      { N_("Q: 1\\/8"),         "<ctrl>8",      N_("Quantize to eighths note boundaries"),
        BST_QUANTIZE_NOTE_8,    BST_STOCK_QNOTE_8, },
      { N_("Q: 1\\/16"),        "<ctrl>6",      N_("Quantize to sixteenth note boundaries"),
        BST_QUANTIZE_NOTE_16,   BST_STOCK_QNOTE_16, },
      { N_("Q: 1\\/32"),        "<ctrl>3",      N_("Quantize to thirty-second note boundaries"),
        BST_QUANTIZE_NOTE_32,   BST_STOCK_QNOTE_32, },
      { N_("Q: 1\\/64"),        "<ctrl>5",      N_("Quantize to sixty-fourth note boundaries"),
        BST_QUANTIZE_NOTE_64,   BST_STOCK_QNOTE_64, },
      { N_("Q: 1\\/128"),       "<ctrl>7",      N_("Quantize to hundred twenty-eighth note boundaries"),
        BST_QUANTIZE_NOTE_128,  BST_STOCK_QNOTE_128, },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

void
bst_piano_roll_controller_set_clipboard (BsePartNoteSeq *pseq)
{
  if (clipboard_pseq)
    bse_part_note_seq_free (clipboard_pseq);
  clipboard_pseq = pseq && pseq->n_pnotes ? bse_part_note_seq_copy_shallow (pseq) : NULL;
  if (clipboard_pseq)
    bst_event_roll_controller_set_clipboard (NULL);
}

BsePartNoteSeq*
bst_piano_roll_controller_get_clipboard (void)
{
  return clipboard_pseq;
}

static void
controller_reset_canvas_cursor (BstPianoRollController *self)
{
  controller_update_canvas_cursor (self, self->canvas_rtools->action_id);
}

BstPianoRollController*
bst_piano_roll_controller_new (BstPianoRoll *proll)
{
  BstPianoRollController *self;

  g_return_val_if_fail (BST_IS_PIANO_ROLL (proll), NULL);

  self = g_new0 (BstPianoRollController, 1);
  self->proll = proll;
  self->ref_count = 1;

  self->ref_count++;
  g_signal_connect_data (proll, "canvas-drag",
			 G_CALLBACK (controller_canvas_drag),
			 self, (GClosureNotify) bst_piano_roll_controller_unref,
			 G_CONNECT_SWAPPED);
  g_signal_connect_data (proll, "piano-drag",
			 G_CALLBACK (controller_piano_drag),
			 self, NULL,
			 G_CONNECT_SWAPPED);
  /* canvas tools */
  self->canvas_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->canvas_rtools, BST_COMMON_ROLL_TOOL_INSERT);
  /* note length selection */
  self->note_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->note_rtools, 4);
  /* quantization selection */
  self->quant_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->quant_rtools, BST_QUANTIZE_NOTE_8);
  /* update from action group */
  g_signal_connect_swapped (self->canvas_rtools, "changed",
                            G_CALLBACK (controller_reset_canvas_cursor), self);
  controller_reset_canvas_cursor (self);
  return self;
}

BstPianoRollController*
bst_piano_roll_controller_ref (BstPianoRollController *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_piano_roll_controller_unref (BstPianoRollController *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  self->ref_count--;
  if (!self->ref_count)
    {
      gxk_action_group_dispose (self->canvas_rtools);
      g_object_unref (self->canvas_rtools);
      gxk_action_group_dispose (self->note_rtools);
      g_object_unref (self->note_rtools);
      gxk_action_group_dispose (self->quant_rtools);
      g_object_unref (self->quant_rtools);
      g_free (self);
    }
}

static gboolean
bst_piano_roll_controller_check_action (BstPianoRollController *self,
                                        gulong                  action_id,
                                        guint64                 action_stamp)
{
  switch (action_id)
    {
    case ACTION_SELECT_ALL:
      return TRUE;
    case ACTION_SELECT_NONE:
    case ACTION_SELECT_INVERT:
      return bst_piano_roll_controller_has_selection (self, action_stamp);
    }
  return FALSE;
}

static void
bst_piano_roll_controller_exec_action (BstPianoRollController *self,
                                       gulong                  action_id)
{
  SfiProxy part = self->proll->proxy;
  switch (action_id)
    {
      BsePartNoteSeq *pseq;
      guint i;
    case ACTION_SELECT_ALL:
      bse_part_select_notes (part, 0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      break;
    case ACTION_SELECT_NONE:
      bse_part_deselect_notes (part, 0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      break;
    case ACTION_SELECT_INVERT:
      pseq = bse_part_list_selected_notes (part);
      bse_part_select_notes (part, 0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      for (i = 0; i < pseq->n_pnotes; i++)
        {
          BsePartNote *pnote = pseq->pnotes[i];
          bse_part_deselect_event (part, pnote->id);
        }
      break;
    }
  gxk_widget_update_actions_downwards (self->proll);
}

static BstCommonRollTool
piano_canvas_button_tool (BstPianoRollController *self,
                          guint                   button,
                          guint                   have_object)
{
  switch (self->canvas_rtools->action_id | /* user selected tool */
          (have_object ? HAVE_OBJECT : 0))
    {
    case BST_COMMON_ROLL_TOOL_INSERT: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_INSERT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_INSERT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_RESIZE;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_DELETE: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_DELETE;       /* user error */
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_DELETE | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_DELETE;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_ALIGN: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_ALIGN;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_ALIGN | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_ALIGN;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_SELECT: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_SELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_SELECT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_SELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_VSELECT: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_VSELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_VSELECT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_VSELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    }
  return BST_COMMON_ROLL_TOOL_NONE;
}

void
bst_piano_roll_controller_clear (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;
  guint i;

  g_return_if_fail (self != NULL);

  proxy = self->proll->proxy;
  pseq = bse_part_list_selected_notes (proxy);
  bse_item_group_undo (proxy, "Clear Selection");
  for (i = 0; i < pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = pseq->pnotes[i];
      bse_part_delete_event (proxy, pnote->id);
    }
  bse_item_ungroup_undo (proxy);
}

void
bst_piano_roll_controller_cut (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;
  guint i;

  g_return_if_fail (self != NULL);

  proxy = self->proll->proxy;
  pseq = bse_part_list_selected_notes (proxy);
  bse_item_group_undo (proxy, "Cut Selection");
  for (i = 0; i < pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = pseq->pnotes[i];
      bse_part_delete_event (proxy, pnote->id);
    }
  bst_piano_roll_controller_set_clipboard (pseq);
  bse_item_ungroup_undo (proxy);
}

gboolean
bst_piano_roll_controller_copy (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;

  g_return_val_if_fail (self != NULL, FALSE);

  proxy = self->proll->proxy;
  pseq = bse_part_list_selected_notes (proxy);
  bst_piano_roll_controller_set_clipboard (pseq);
  return pseq && pseq->n_pnotes;
}

void
bst_piano_roll_controller_paste (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq;
  SfiProxy proxy;

  g_return_if_fail (self != NULL);

  proxy = self->proll->proxy;
  pseq = bst_piano_roll_controller_get_clipboard ();
  if (pseq)
    {
      guint i, paste_tick, ctick = self->proll->max_ticks;
      gint cnote = 0;
      gint paste_note;
      bse_item_group_undo (proxy, "Paste Clipboard");
      bse_part_deselect_notes (proxy, 0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      bst_piano_roll_get_paste_pos (self->proll, &paste_tick, &paste_note);
      paste_tick = bst_piano_roll_controller_quantize (self, paste_tick);
      for (i = 0; i < pseq->n_pnotes; i++)
	{
	  BsePartNote *pnote = pseq->pnotes[i];
	  ctick = MIN (ctick, pnote->tick);
	  cnote = MAX (cnote, pnote->note);
	}
      cnote = paste_note - cnote;
      for (i = 0; i < pseq->n_pnotes; i++)
	{
	  BsePartNote *pnote = pseq->pnotes[i];
	  guint id;
	  gint note;
	  note = pnote->note + cnote;
	  if (note >= 0)
	    {
	      id = bse_part_insert_note_auto (proxy,
                                              pnote->tick - ctick + paste_tick,
                                              pnote->duration,
                                              note,
                                              pnote->fine_tune,
                                              pnote->velocity);
              bse_part_select_event (proxy, id);
            }
	}
      bse_item_ungroup_undo (proxy);
    }
}

gboolean
bst_piano_roll_controller_clipboard_full (BstPianoRollController *self)
{
  BsePartNoteSeq *pseq = bst_piano_roll_controller_get_clipboard ();
  return pseq && pseq->n_pnotes;
}

gboolean
bst_piano_roll_controller_has_selection (BstPianoRollController *self,
                                         guint64                 action_stamp)
{
  if (self->cached_stamp != action_stamp)
    {
      SfiProxy part = self->proll->proxy;
      if (part)
        {
          self->cached_stamp = action_stamp;
          BsePartNoteSeq *pseq = bse_part_list_selected_notes (part);
          self->cached_n_notes = pseq->n_pnotes;
        }
      else
        self->cached_n_notes = 0;
    }
  return self->cached_n_notes > 0;
}

guint
bst_piano_roll_controller_quantize (BstPianoRollController *self,
                                    guint                   fine_tick)
{
  BseSongTiming *timing;
  guint quant, tick, qtick;
  g_return_val_if_fail (self != NULL, fine_tick);

  timing = bse_part_get_timing (self->proll->proxy, fine_tick);
  if (QUANTIZATION (self) == BST_QUANTIZE_NONE)
    quant = 1;
  else if (QUANTIZATION (self) == BST_QUANTIZE_TACT)
    quant = timing->tpt;
  else
    quant = timing->tpqn * 4 / QUANTIZATION (self);
  tick = fine_tick - timing->tick;
  qtick = tick / quant;
  qtick *= quant;
  if (tick - qtick > quant / 2)
    qtick += quant;
  tick = timing->tick + qtick;
  return tick;
}

static void
controller_update_canvas_cursor (BstPianoRollController *self,
                                 BstCommonRollTool      tool)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self->proll);
  switch (tool)
    {
    case BST_COMMON_ROLL_TOOL_INSERT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_PENCIL);
      break;
    case BST_COMMON_ROLL_TOOL_RESIZE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_SB_H_DOUBLE_ARROW);
      break;
    case BST_COMMON_ROLL_TOOL_MOVE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_FLEUR);
      break;
    case BST_COMMON_ROLL_TOOL_DELETE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_TARGET);
      break;
    case BST_COMMON_ROLL_TOOL_SELECT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_CROSSHAIR);
      break;
    case BST_COMMON_ROLL_TOOL_VSELECT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_LEFT_SIDE);
      break;
    default:
      gxk_scroll_canvas_set_canvas_cursor (scc, GXK_DEFAULT_CURSOR);
      break;
    }
}

static gboolean
check_hoverlap (SfiProxy part,
		guint    tick,
		guint    duration,
		gint     note,
		guint    except_tick,
		guint    except_duration)
{
  if (duration)
    {
      BsePartNoteSeq *pseq = bse_part_check_overlap (part, tick, duration, note);
      BsePartNote *pnote;
      
      if (pseq->n_pnotes == 0)
	return FALSE;     /* no overlap */
      if (pseq->n_pnotes > 1)
	return TRUE;      /* definite overlap */
      pnote = pseq->pnotes[0];
      if (pnote->tick == except_tick &&
	  pnote->duration == except_duration)
	return FALSE;     /* overlaps with exception */
    }
  return TRUE;
}

static void
move_start (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  if (self->obj_id)	/* got note to move */
    {
      self->xoffset = drag->start_tick - self->obj_tick;	/* drag offset */
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_WAIT, _("Move Note"), NULL);
      drag->state = GXK_DRAG_CONTINUE;
      if (bse_part_is_event_selected (part, self->obj_id))
	self->sel_pseq = bse_part_note_seq_copy_shallow (bse_part_list_selected_notes (part));
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Move Note"), _("No target"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
move_group_motion (BstPianoRollController *self,
		   BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  gint i, new_tick, old_note, new_note, delta_tick, delta_note;

  new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_piano_roll_controller_quantize (self, new_tick);
  old_note = self->obj_note;
  new_note = drag->current_note;
  delta_tick = self->obj_tick;
  delta_note = old_note;
  delta_tick -= new_tick;
  delta_note -= new_note;
  bse_item_group_undo (part, "Move Selection");
  for (i = 0; i < self->sel_pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = self->sel_pseq->pnotes[i];
      gint tick = pnote->tick;
      gint note = pnote->note;
      note -= delta_note;
      bse_part_change_note (part, pnote->id,
			    MAX (tick - delta_tick, 0),
			    pnote->duration,
			    SFI_NOTE_CLAMP (note),
			    pnote->fine_tune,
			    pnote->velocity);
    }
  if (drag->type == GXK_DRAG_DONE)
    {
      bse_part_note_seq_free (self->sel_pseq);
      self->sel_pseq = NULL;
    }
  bse_item_ungroup_undo (part);
}

static void
move_motion (BstPianoRollController *self,
	     BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  gint new_tick;
  gboolean note_changed;

  if (self->sel_pseq)
    {
      move_group_motion (self, drag);
      return;
    }

  new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_piano_roll_controller_quantize (self, new_tick);
  note_changed = self->obj_note != drag->current_note;
  if ((new_tick != self->obj_tick || note_changed) &&
      !check_hoverlap (part, new_tick, self->obj_duration, drag->current_note,
		       self->obj_tick, note_changed ? 0 : self->obj_duration))
    {
      bse_item_group_undo (part, "Move Note");
      if (bse_part_delete_event (part, self->obj_id) != BSE_ERROR_NONE)
        drag->state = GXK_DRAG_ERROR;
      else
	{
	  self->obj_id = bse_part_insert_note_auto (part, new_tick, self->obj_duration,
                                                    drag->current_note, self->obj_fine_tune, self->obj_velocity);
	  self->obj_tick = new_tick;
	  self->obj_note = drag->current_note;
	  if (!self->obj_id)
	    drag->state = GXK_DRAG_ERROR;
	}
      bse_item_ungroup_undo (part);
    }
}

static void
move_abort (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  if (self->sel_pseq)
    {
      bse_part_note_seq_free (self->sel_pseq);
      self->sel_pseq = NULL;
    }
  gxk_status_set (GXK_STATUS_ERROR, _("Move Note"), _("Lost Note"));
}

static void
resize_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  if (self->obj_id)	/* got note for resize */
    {
      guint bound = self->obj_tick + self->obj_duration + 1;

      /* set the fix-point (either note start or note end) */
      if (drag->start_tick - self->obj_tick <= bound - drag->start_tick)
	self->tick_bound = bound;
      else
	self->tick_bound = self->obj_tick;
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_RESIZE);
      gxk_status_set (GXK_STATUS_WAIT, _("Resize Note"), NULL);
      drag->state = GXK_DRAG_CONTINUE;
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Resize Note"), _("No target"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
resize_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  guint new_bound, new_tick, new_duration;

  /* calc new note around fix-point */
  new_tick = bst_piano_roll_controller_quantize (self, drag->current_tick);
  new_bound = MAX (new_tick, self->tick_bound);
  new_tick = MIN (new_tick, self->tick_bound);
  new_duration = new_bound - new_tick;
  new_duration = MAX (new_duration, 1) - 1;

  /* apply new note size */
  if ((self->obj_tick != new_tick || new_duration != self->obj_duration) &&
      !check_hoverlap (part, new_tick, new_duration, self->obj_note,
		       self->obj_tick, self->obj_duration))
    {
      bse_item_group_undo (part, "Resize Note");
      if (self->obj_id)
	{
	  BseErrorType error = bse_part_delete_event (part, self->obj_id);
	  if (error)
	    drag->state = GXK_DRAG_ERROR;
	  self->obj_id = 0;
	}
      if (new_duration && drag->state != GXK_DRAG_ERROR)
	{
	  self->obj_id = bse_part_insert_note_auto (part, new_tick, new_duration,
                                                    self->obj_note, self->obj_fine_tune, self->obj_velocity);
	  self->obj_tick = new_tick;
	  self->obj_duration = new_duration;
	  if (!self->obj_id)
	    drag->state = GXK_DRAG_ERROR;
	}
      bse_item_ungroup_undo (part);
    }
}

static void
resize_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Resize Note"), _("Lost Note"));
}

static void
delete_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  if (self->obj_id)	/* got note to delete */
    {
      BseErrorType error = bse_part_delete_event (part, self->obj_id);
      bst_status_eprintf (error, _("Delete Note"));
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, _("Delete Note"), _("No target"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
insert_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  BseErrorType error = BSE_ERROR_NO_TARGET;
  if (drag->start_valid)
    {
      guint qtick = bst_piano_roll_controller_quantize (self, drag->start_tick);
      guint duration = drag->proll->ppqn * 4 / NOTE_LENGTH (self);
      if (check_hoverlap (part, qtick, duration, drag->start_note, 0, 0))
	error = BSE_ERROR_INVALID_OVERLAP;
      else
	{
	  bse_part_insert_note_auto (part, qtick, duration, drag->start_note, 0, 1.0);
	  error = BSE_ERROR_NONE;
	}
    }
  bst_status_eprintf (error, _("Insert Note"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
select_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  drag->start_tick = bst_piano_roll_controller_quantize (self, drag->start_tick);
  bst_piano_roll_set_view_selection (drag->proll, drag->start_tick, 0, 0, 0);
  gxk_status_set (GXK_STATUS_WAIT, _("Select Region"), NULL);
  drag->state = GXK_DRAG_CONTINUE;
}

static void
select_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);
  gint min_note = MIN (drag->start_note, drag->current_note);
  gint max_note = MAX (drag->start_note, drag->current_note);

  bst_piano_roll_set_view_selection (drag->proll, start_tick, end_tick - start_tick, min_note, max_note);
  if (drag->type == GXK_DRAG_DONE)
    {
      bse_part_select_notes_exclusive (part, start_tick, end_tick - start_tick, min_note, max_note);
      bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
    }
}

static void
select_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Select Region"), _("Aborted"));
  bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
}

static void
vselect_start (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  drag->start_tick = bst_piano_roll_controller_quantize (self, drag->start_tick);
  bst_piano_roll_set_view_selection (drag->proll, drag->start_tick, 0, drag->proll->min_note, drag->proll->max_note);
  gxk_status_set (GXK_STATUS_WAIT, _("Vertical Select"), NULL);
  drag->state = GXK_DRAG_CONTINUE;
}

static void
vselect_motion (BstPianoRollController *self,
		BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);

  bst_piano_roll_set_view_selection (drag->proll, start_tick, end_tick - start_tick,
				     drag->proll->min_note, drag->proll->max_note);
  if (drag->type == GXK_DRAG_DONE)
    {
      bse_part_select_notes_exclusive (part, start_tick, end_tick - start_tick,
                                       drag->proll->min_note, drag->proll->max_note);
      bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
    }
}

static void
vselect_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Vertical Region"), _("Aborted"));
  bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
}

#if 0
static void
generic_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Abortion"), NULL);
}
#endif

typedef void (*DragFunc) (BstPianoRollController *,
			  BstPianoRollDrag       *);

void
controller_canvas_drag (BstPianoRollController *self,
			BstPianoRollDrag       *drag)
{
  static struct {
    BstCommonRollTool tool;
    DragFunc start, motion, abort;
  } tool_table[] = {
    { BST_COMMON_ROLL_TOOL_INSERT,	insert_start,	NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_ALIGN,	insert_start,	NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_RESIZE,	resize_start,	resize_motion,	resize_abort,	},
    { BST_COMMON_ROLL_TOOL_MOVE,	move_start,	move_motion,	move_abort,	},
    { BST_COMMON_ROLL_TOOL_DELETE,	delete_start,	NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_SELECT,	select_start,	select_motion,	select_abort,	},
    { BST_COMMON_ROLL_TOOL_VSELECT,	vselect_start,	vselect_motion,	vselect_abort,	},
  };
  guint i;

  if (drag->type == GXK_DRAG_START)
    {
      BstCommonRollTool tool = BST_COMMON_ROLL_TOOL_NONE;
      BsePartNoteSeq *pseq;

      /* setup drag data */
      pseq = bse_part_get_notes (drag->proll->proxy, drag->start_tick, drag->start_note);
      if (pseq->n_pnotes)
	{
	  BsePartNote *pnote = pseq->pnotes[0];
	  self->obj_id = pnote->id;
	  self->obj_tick = pnote->tick;
	  self->obj_duration = pnote->duration;
	  self->obj_note = pnote->note;
	  self->obj_fine_tune = pnote->fine_tune;
	  self->obj_velocity = pnote->velocity;
	}
      else
	{
	  self->obj_id = 0;
	  self->obj_tick = 0;
	  self->obj_duration = 0;
	  self->obj_note = 0;
	  self->obj_fine_tune = 0;
	  self->obj_velocity = 0;
	}
      if (self->sel_pseq)
	g_warning ("leaking old drag selection (%p)", self->sel_pseq);
      self->sel_pseq = NULL;
      self->xoffset = 0;
      self->tick_bound = 0;

      /* find drag tool */
      tool = piano_canvas_button_tool (self, drag->button, self->obj_id > 0);
      for (i = 0; i < G_N_ELEMENTS (tool_table); i++)
	if (tool_table[i].tool == tool)
	  break;
      self->tool_index = i;
      if (i >= G_N_ELEMENTS (tool_table))
	return;		/* unhandled */
    }
  i = self->tool_index;
  g_return_if_fail (i < G_N_ELEMENTS (tool_table));
  switch (drag->type)
    {
    case GXK_DRAG_START:
      if (tool_table[i].start)
	tool_table[i].start (self, drag);
      break;
    case GXK_DRAG_MOTION:
    case GXK_DRAG_DONE:
      if (tool_table[i].motion)
	tool_table[i].motion (self, drag);
      break;
    case GXK_DRAG_ABORT:
      if (tool_table[i].abort)
	tool_table[i].abort (self, drag);
      break;
    }
  if (drag->type == GXK_DRAG_DONE ||
      drag->type == GXK_DRAG_ABORT)
    controller_reset_canvas_cursor (self);
}

void
controller_piano_drag (BstPianoRollController *self,
		       BstPianoRollDrag       *drag)
{
  SfiProxy part = self->proll->proxy;
  SfiProxy song = bse_item_get_parent (part);
  SfiProxy project = song ? bse_item_get_parent (song) : 0;
  SfiProxy track = song ? bse_song_find_track_for_part (song, part) : 0;

  // g_printerr ("piano drag event, note=%d (valid=%d)", drag->current_note, drag->current_valid);

  if (project && track)
    {
      if (drag->type == GXK_DRAG_START ||
	  (drag->type == GXK_DRAG_MOTION &&
	   self->obj_note != drag->current_note))
	{
	  BseErrorType error;
	  bse_project_auto_deactivate (project, 5 * 1000);
	  error = bse_project_activate (project);
	  self->obj_note = drag->current_note;
	  if (error == BSE_ERROR_NONE)
	    bse_song_synthesize_note (song, track, 384 * 4, self->obj_note, 0, 1.0);
	  bst_status_eprintf (error, _("Play note"));
	  drag->state = GXK_DRAG_CONTINUE;
	}
    }

  if (drag->type == GXK_DRAG_START ||
      drag->type == GXK_DRAG_MOTION)
    drag->state = GXK_DRAG_CONTINUE;
}

/* BseMult - BSE Multiplier
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
#include "bsemult.h"

#include <bse/bseengine.h>

#include <string.h>

/* --- prototypes --- */
static void	 bse_mult_init			(BseMult	*mult);
static void	 bse_mult_class_init		(BseMultClass	*class);
static void	 bse_mult_context_create	(BseSource	*source,
						 guint		 context_handle,
						 BseTrans	*trans);


/* --- Export to BSE --- */
#include "./icons/multiply.c"
BSE_REGISTER_OBJECT (BseMult, BseSource, "/Modules/Routing/Multiply", "",
                     "Mult is a channel multiplier for ring-modulating incoming signals",
                     multiply_icon,
                     bse_mult_class_init, NULL, bse_mult_init);
BSE_DEFINE_EXPORTS ();


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
static void
bse_mult_class_init (BseMultClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ichannel, ochannel;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  source_class->context_create = bse_mult_context_create;
  
  ichannel = bse_source_class_add_ichannel (source_class, "audio-in1", _("Audio In1"), _("Audio Input 1"));
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO1);
  ichannel = bse_source_class_add_ichannel (source_class, "audio-in2", _("Audio In2"), _("Audio Input 2"));
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO2);
  ichannel = bse_source_class_add_ichannel (source_class, "audio-in3", _("Audio In3"), _("Audio Input 3"));
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO3);
  ichannel = bse_source_class_add_ichannel (source_class, "audio-in4", _("Audio In4"), _("Audio Input 4"));
  g_assert (ichannel == BSE_MULT_ICHANNEL_MONO4);
  ochannel = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("Audio Output"));
  g_assert (ochannel == BSE_MULT_OCHANNEL_MONO);
}

static void
bse_mult_init (BseMult *mult)
{
}

static void
multiply_process (BseModule *module,
		  guint      n_values)
{
  // = module->user_data;
  gfloat *wave_out = BSE_MODULE_OBUFFER (module, 0);
  gfloat *bound = wave_out + n_values;
  guint i;
  
  if (!module->ostreams[0].connected)
    return;	/* nothing to process */
  for (i = 0; i < BSE_MODULE_N_ISTREAMS (module); i++)
    if (module->istreams[i].connected)
      {
	/* found first channel */
	memcpy (wave_out, BSE_MODULE_IBUFFER (module, i), n_values * sizeof (wave_out[0]));
	break;
      }
  if (i >= BSE_MODULE_N_ISTREAMS (module))
    {
      /* no input, FIXME: should set static-0 here */
      memset (wave_out, 0, n_values * sizeof (wave_out[0]));
    }
  for (i += 1; i < BSE_MODULE_N_ISTREAMS (module); i++)
    if (module->istreams[i].connected)
      {
	const gfloat *in = BSE_MODULE_IBUFFER (module, i);
	gfloat *out = wave_out;
	
	/* found 1+nth channel to multiply with */
	do
	  *out++ *= *in++;
	while (out < bound);
      }
}

static void
bse_mult_context_create (BseSource *source,
			 guint      context_handle,
			 BseTrans  *trans)
{
  static const BseModuleClass multiply_class = {
    4,                          /* n_istreams */
    0,                          /* n_jstreams */
    1,                          /* n_ostreams */
    multiply_process,           /* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    NULL,                       /* free */
    BSE_COST_CHEAP,             /* cost */
  };
  // BseMult *mult = BSE_MULT (source);
  BseModule *module;
  
  module = bse_module_new (&multiply_class, NULL);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

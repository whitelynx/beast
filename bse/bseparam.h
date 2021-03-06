/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#ifndef __BSE_PARAM_H__
#define __BSE_PARAM_H__

#include        <bse/bsetype.h>
#include        <bse/bseutils.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object param specs --- */
#define	    BSE_TYPE_PARAM_OBJECT		(G_TYPE_PARAM_OBJECT)
#define	    BSE_IS_PARAM_SPEC_OBJECT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_OBJECT))
#define	    BSE_PARAM_SPEC_OBJECT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_OBJECT, BseParamSpecObject))
typedef	    GParamSpecObject			 BseParamSpecObject;
GParamSpec* bse_param_spec_object		(const gchar	*name,
						 const gchar	*nick,
						 const gchar	*blurb,
						 GType		 object_type,
						 const gchar	*hints);

#define	    BSE_VALUE_HOLDS_OBJECT(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), BSE_TYPE_OBJECT))
#define	    bse_value_get_object		 g_value_get_object
#define	    bse_value_set_object		 g_value_set_object
#define	    bse_value_take_object		 g_value_take_object
GValue*	    bse_value_object			(gpointer	 vobject);


/* --- boxed parameters --- */
typedef GParamSpecBoxed			 BseParamSpecBoxed;
#define BSE_TYPE_PARAM_BOXED		(G_TYPE_PARAM_BOXED)
#define BSE_IS_PARAM_SPEC_BOXED(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_BOXED))
#define BSE_PARAM_SPEC_BOXED(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_BOXED, BseParamSpecBoxed))
#define BSE_VALUE_HOLDS_BOXED(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_BOXED))
GParamSpec* bse_param_spec_boxed	(const gchar  *name,
					 const gchar  *nick,
					 const gchar  *blurb,
					 GType	       boxed_type,
					 const gchar  *hints);
#define     bse_value_get_boxed          g_value_get_boxed
#define     bse_value_set_boxed          g_value_set_boxed
#define     bse_value_dup_boxed          g_value_dup_boxed
#define     bse_value_take_boxed         g_value_take_boxed


/* --- convenience pspec constructors --- */
GParamSpec* bse_param_spec_freq         (const gchar  *name,
					 const gchar  *nick,
					 const gchar  *blurb,
					 SfiReal       default_freq,
                                         SfiReal       min_freq,
                                         SfiReal       max_freq,
					 const gchar  *hints);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PARAM_H__ */

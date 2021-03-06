/* Splines - Spline creation and evaluation routines
 * Copyright (C) 2004-2006 Tim Janik
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
#ifndef __GXK_SPLINE_H__
#define __GXK_SPLINE_H__

#include        <gxk/gxkglobals.h>

G_BEGIN_DECLS

typedef struct _GxkSpline GxkSpline;
typedef struct {
  double x, y;
} GxkSplinePoint;
typedef struct {
  double x, y, yd2;
  double ymin, ymax;
  double ex1, ex2;
} GxkSplineSegment;
struct _GxkSpline {
  guint             n_segs;     /* == n_points */
  GxkSplineSegment  segs[0];    /* last segment always has NAN extrema */
};
GxkSpline*      gxk_spline_new_natural  (guint                   n_points,
                                         const GxkSplinePoint   *points);
GxkSpline*      gxk_spline_new          (guint                   n_points,
                                         const GxkSplinePoint   *points,
                                         double                  dy_start,
                                         double                  dy_end);
double          gxk_spline_eval         (const GxkSpline        *spline,
                                         double                  x,
                                         double                 *yd1);
double          gxk_spline_y            (const GxkSpline        *spline,
                                         double                  x);
double          gxk_spline_findx        (const GxkSpline        *spline,
                                         double                  y);
void            gxk_spline_dump         (GxkSpline              *spline);
GxkSpline*      gxk_spline_copy         (GxkSpline              *spline);
void            gxk_spline_free         (GxkSpline              *spline);



G_END_DECLS

#endif /* __GXK_SPLINE_H__ */

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
#include "sficomport.h"
#include "sfiprimitives.h"
#include "sfilog.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>


#define	DEBUG	sfi_nodebug


/* --- functions --- */
static void
nonblock_fd (gint fd)
{
  if (fd >= 0)
    {
      glong r, d_long;
      do
	d_long = fcntl (fd, F_GETFL);
      while (d_long < 0 && errno == EINTR);
      
      d_long |= O_NONBLOCK;
      
      do
	r = fcntl (fd, F_SETFL, d_long);
      while (r < 0 && errno == EINTR);
    }
}

SfiComPort*
sfi_com_port_from_child (const gchar *ident,
			 gint         remote_input,
			 gint         remote_output,
			 gint         standard_input,
			 gint         standard_output,
			 gint         standard_error,
			 gint         remote_pid)
{
  SfiComPort *port;
  guint i;

  g_return_val_if_fail (ident != NULL, NULL);

  port = g_new0 (SfiComPort, 1);
  port->ref_count = 1;
  if (remote_pid > 1)
    port->ident = g_strdup_printf ("%s[%u]", ident, remote_pid);
  else
    port->ident = g_strdup (ident);
  port->n_pfds = ((remote_input >= 0) +
		  (remote_output >= 0) +
		  (standard_input >= 0) +
		  (standard_output >= 0) +
		  (standard_error >= 0));
  port->pfds = g_new0 (GPollFD, port->n_pfds);
  port->buffer = g_malloc0 (sizeof (port->buffer[0]) * port->n_pfds);

  i = 0;
  if (remote_input >= 0)
    {
      port->pfds[i].fd = remote_input;
      port->rinp = i++;
    }
  else
    port->rinp = port->n_pfds;
  if (remote_output >= 0)
    {
      port->pfds[i].fd = remote_output;
      port->rout = i++;
    }
  else
    port->rout = port->n_pfds;
  if (standard_input >= 0)
    {
      port->pfds[i].fd = standard_input;
      port->sinp = i++;
    }
  else
    port->sinp = port->n_pfds;
  if (standard_output >= 0)
    {
      port->pfds[i].fd = standard_output;
      port->sout = i++;
    }
  else
    port->sout = port->n_pfds;
  if (standard_error >= 0)
    {
      port->pfds[i].fd = standard_error;
      port->serr = i++;
    }
  else
    port->serr = port->n_pfds;
  port->remote_pid = remote_pid > 1 ? remote_pid : -1;

  for (i = 0; i < port->n_pfds; i++)
    nonblock_fd (port->pfds[i].fd);
  port->connected = port->n_pfds > 0;
  
  return port;
}

SfiComPort*
sfi_com_port_from_pipe (const gchar *ident,
			gint         remote_input,
			gint         remote_output)
{
  g_return_val_if_fail (ident != NULL, NULL);
  
  return sfi_com_port_from_child (ident,
				  remote_input,
				  remote_output,
				  -1, -1, -1, -1);
}

void
sfi_com_port_create_linked (const gchar *ident1,
			    SfiThread   *thread1,
			    SfiComPort **port1,
			    const gchar *ident2,
			    SfiThread   *thread2,
			    SfiComPort **port2)
{
  SfiComPortLink *link;

  g_return_if_fail (thread1 && ident1);
  g_return_if_fail (thread2 && ident2);
  g_return_if_fail (port1 && port2);

  link = g_new0 (SfiComPortLink, 1);
  sfi_mutex_init (&link->mutex);
  link->port1 = sfi_com_port_from_child (ident1, -1, -1, -1, -1, -1, -1);
  link->thread1 = thread1;
  link->port2 = sfi_com_port_from_child (ident2, -1, -1, -1, -1, -1, -1);
  link->thread2 = thread2;
  link->ref_count = 2;
  link->port1->link = link;
  link->port1->connected = TRUE;
  link->port2->link = link;
  link->port2->connected = TRUE;
  *port1 = link->port1;
  *port2 = link->port2;
  sfi_cond_init (&link->wcond);
}

static void
sfi_com_port_link_destroy (SfiComPortLink *link)
{
  g_return_if_fail (link->ref_count == 0);
  g_return_if_fail (link->port1== NULL && link->port2 == NULL);

  while (link->p1queue)
    sfi_value_free (sfi_ring_pop_head (&link->p1queue));
  while (link->p2queue)
    sfi_value_free (sfi_ring_pop_head (&link->p2queue));
  sfi_mutex_destroy (&link->mutex);
  sfi_cond_destroy (&link->wcond);
  g_free (link);
}

SfiComPort*
sfi_com_port_ref (SfiComPort *port)
{
  g_return_val_if_fail (port != NULL, NULL);
  g_return_val_if_fail (port->ref_count > 0, NULL);

  port->ref_count++;
  return port;
}

void
sfi_com_port_close_remote (SfiComPort *port,
			   gboolean    terminate)
{
  guint i;

  g_return_if_fail (port != NULL);

  port->connected = FALSE;
  for (i = 0; i < port->n_pfds; i++)
    {
      close (port->pfds[i].fd);
      port->pfds[i].fd = -1;
      port->pfds[i].events = 0;
      g_free (port->buffer[i].data);
      port->buffer[i].data = NULL;
      port->buffer[i].allocated = 0;
      port->buffer[i].n = 0;
    }
  if (port->remote_pid > 1 && terminate)
    kill (port->remote_pid, SIGTERM);
  port->remote_pid = -1;
  if (port->link)
    {
      SfiComPortLink *link = port->link;
      gboolean need_destroy;
      SFI_SPIN_LOCK (&link->mutex);
      if (port == link->port1)
	{
	  link->port1 = NULL;
	  link->thread1 = NULL;
	}
      else
	{
	  link->port2 = NULL;
	  link->thread2 = NULL;
	}
      link->ref_count--;
      need_destroy = link->ref_count == 0;
      SFI_SPIN_UNLOCK (&link->mutex);
      port->link = NULL;
      if (need_destroy)
	sfi_com_port_link_destroy (port->link);
    }
}

static void
sfi_com_port_destroy (SfiComPort *port)
{
  g_return_if_fail (port != NULL);
  
  sfi_com_port_close_remote (port, FALSE);
  g_free (port->ident);
  g_free (port->pfds);
  g_free (port->buffer);
  g_free (port);
}

void
sfi_com_port_unref (SfiComPort *port)
{
  g_return_if_fail (port != NULL);
  g_return_if_fail (port->ref_count > 0);

  port->ref_count--;
  if (!port->ref_count)
    sfi_com_port_destroy (port);
}

static void
com_port_grow_buffer (SfiComPort *port,
		      guint       i,
		      guint       l)
{
  if (port->buffer[i].n + l < port->buffer[i].allocated)
    {
      port->buffer[i].allocated = port->buffer[i].n + l;
      port->buffer[i].data = g_renew (guint8, port->buffer[i].data, port->buffer[i].allocated);
    }
}

static void
com_port_write_queued (SfiComPort *port,
		       guint       i)
{
  if (port->buffer[i].n)
    {
      gint l;
      do
	l = write (port->pfds[i].fd, port->buffer[i].data, port->buffer[i].n);
      while (l < 0 && errno == EINTR);
      if (l > 0)
	{
	  port->buffer[i].n -= l;
	  g_memmove (port->buffer[i].data, port->buffer[i].data + l, port->buffer[i].n);
	}
    }
}

static void
com_port_write (SfiComPort   *port,
		guint         i,
		guint         n_bytes,
		const guint8 *bytes)
{
  com_port_write_queued (port, i);
  if (!port->buffer[i].n)
    {
      gint l;
      do
	l = write (port->pfds[i].fd, bytes, n_bytes);
      while (l < 0 && errno == EINTR);
      l = CLAMP (l, 0, n_bytes);
      n_bytes -= l;
      bytes += l;
    }
  if (n_bytes)
    {
      /* append to queued data */
      com_port_grow_buffer (port, i, n_bytes);
      memcpy (port->buffer[i].data, bytes, n_bytes);
      port->buffer[i].n += n_bytes;
    }
}

void
sfi_com_port_write_stdin (SfiComPort *port,
			  guint       n_chars,
			  guint8     *data)
{
  g_return_if_fail (port != NULL);
  g_return_if_fail (port->sinp < port->n_pfds);

  if (n_chars)
    com_port_write (port, port->sinp, n_chars, data);
}

void
sfi_com_port_send (SfiComPort   *port,
		   const GValue *value)
{

  g_return_if_fail (port != NULL);
  g_return_if_fail (port->rout < port->n_pfds || link);
  g_return_if_fail (value != NULL);

  if (link)
    {
      SfiComPortLink *link = port->link;
      gboolean first = port == link->port1;
      SfiThread *thread = NULL;
      /* guard caller against receiver messing with value */
      GValue *dvalue = sfi_value_clone_deep (value);

      SFI_SPIN_LOCK (&link->mutex);
      if (first)
	link->p1queue = sfi_ring_append (link->p1queue, dvalue);
      else
	link->p2queue = sfi_ring_append (link->p2queue, dvalue);
      if (link->waiting)
	sfi_cond_signal (&link->wcond);
      else
	thread = first ? link->thread2 : link->thread1;
      SFI_SPIN_UNLOCK (&link->mutex);
      DEBUG ("[%s: sent ((GValue*)%p)]", port->ident, dvalue);
      if (thread)
	sfi_thread_wakeup (thread);
    }
  else
    g_assert_not_reached ();
}

static GValue*
sfi_com_port_recv_intern (SfiComPort *port,
			  gboolean    blocking)
{
  GValue *value;
  
  DEBUG ("[%s: START receiving]", port->ident);
  if (link)
    {
      SfiComPortLink *link = port->link;
      
      SFI_SPIN_LOCK (&link->mutex);
    refetch:
      if (port == link->port1)
	value = sfi_ring_pop_head (&link->p2queue);
      else
	value = sfi_ring_pop_head (&link->p1queue);
      if (!value && blocking)
	{
	  link->waiting = TRUE;
	  sfi_cond_wait (&link->wcond, &link->mutex);
	  link->waiting = FALSE;
	  goto refetch;
	}
      SFI_SPIN_UNLOCK (&link->mutex);
    }
  else
    {
      value = NULL;
      g_assert_not_reached ();
    }
  DEBUG ("[%s: DONE receiving: ((GValue*)%p) ]", port->ident, value);
  return value;
}

GValue*
sfi_com_port_recv (SfiComPort *port)
{
  g_return_val_if_fail (port != NULL, NULL);
  g_return_val_if_fail (port->rinp < port->n_pfds || port->link, NULL);

  return sfi_com_port_recv_intern (port, FALSE);
}

GValue*
sfi_com_port_recv_blocking (SfiComPort *port)
{
  g_return_val_if_fail (port != NULL, NULL);
  g_return_val_if_fail (port->rinp < port->n_pfds || port->link, NULL);

  return sfi_com_port_recv_intern (port, TRUE);
}

GPollFD*
sfi_com_port_get_remote_pfd (SfiComPort *port)
{
  g_return_val_if_fail (port != NULL, NULL);
  g_return_val_if_fail (port->rinp < port->n_pfds || port->link, NULL);

  return port->rinp < port->n_pfds ? port->pfds + port->rinp : NULL;
}

gboolean
sfi_com_port_io_pending (SfiComPort *port)
{
  guint i;

  g_return_val_if_fail (port != NULL, FALSE);

  if (port->link &&
      ((port == port->link->port1 && port->link->p2queue) ||
       (port == port->link->port2 && port->link->p1queue)))
    return TRUE;
  for (i = 0; i < port->n_pfds; i++)
    if (port->buffer[i].n)
      return TRUE;
  return FALSE;
}

void
sfi_com_port_process_io (SfiComPort *port)
{
  g_return_if_fail (port != NULL);

  /* read from input streams */
  /* flush output streams... */
}

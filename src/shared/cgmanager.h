/*
 * lxc: linux Container library
 *
 * (C) Copyright Canonical Ltd. 2013-2014
 *
 * Authors:
 * Serge Hallyn <serge.hallyn@canonical.com>
 * Stéphane Graber <stephane.graber@canonical.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CGMANAGER

#include <stdbool.h>
#include <nih-dbus/dbus_connection.h>
#include <cgmanager/cgmanager-client.h>
#include <nih/alloc.h>
#include <nih/error.h>
#include <nih/string.h>

bool cgm_dbus_connect(void);
void cgm_dbus_disconnect(void);
bool cgm_create(const char *controller, const char *cgroup_path, int32_t *existed);
bool cgm_remove(const char *controller, const char *cgroup_path, int recursive);
char *cgm_get(const char *controller, const char *path, const char *key);
bool cgm_chmod(const char *controller, const char *cgroup_path, int mode);
bool cgm_chown(const char *controller, const char *cgroup_path, int uid, int gid);
bool cgm_list_children(const char *controller, const char *cgroup_path, char ***children);
bool cgm_controller_exists(const char *controller);
bool cgm_enter(const char *controller, const char *cgroup_path, pid_t pid);
#endif

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

#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "cgmanager.h"
#include "log.h"

static NihDBusProxy *cgroup_manager = NULL;

#define CGMANAGER_DBUS_SOCK "unix:path=/sys/fs/cgroup/cgmanager/sock"
bool cgm_dbus_connect(void)
{
	DBusError dbus_error;
	DBusConnection *connection;
	dbus_error_init(&dbus_error);

	connection = dbus_connection_open_private(CGMANAGER_DBUS_SOCK, &dbus_error);
	if (!connection) {
		dbus_error_free(&dbus_error);
		return false;
	}

	dbus_connection_set_exit_on_disconnect(connection, FALSE);
	dbus_error_free(&dbus_error);
	cgroup_manager = nih_dbus_proxy_new(NULL, connection,
				NULL /* p2p */,
				"/org/linuxcontainers/cgmanager", NULL, NULL);
	dbus_connection_unref(connection);
	if (!cgroup_manager) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: Error opening proxy: %s", nerr->message);
		nih_free(nerr);
		return false;
	}

	// force fd passing negotiation
	if (cgmanager_ping_sync(NULL, cgroup_manager, 0) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: Error pinging manager: %s", nerr->message);
		nih_free(nerr);
		nih_free(cgroup_manager);
		cgroup_manager = NULL;
		return false;
	}
	return true;
}

void cgm_dbus_disconnect(void)
{
	if (cgroup_manager) {
		dbus_connection_flush(cgroup_manager->connection);
		dbus_connection_close(cgroup_manager->connection);
		nih_free(cgroup_manager);
		cgroup_manager = NULL;
	}
}

bool cgm_create(const char *controller, const char *cgroup_path, int32_t *existed)
{
	if (cgroup_path[0] == '/')
		cgroup_path++;

	if ( cgmanager_create_sync(NULL, cgroup_manager, controller,
				       cgroup_path, existed) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: cgm_create for controller=%s, cgroup_path=%s failed: %s",
		          controller, cgroup_path, nerr->message);
		nih_free(nerr);
		return false;
	}

	return true;
}

bool cgm_remove(const char *controller, const char *cgroup_path, int recursive)
{
	int existed;

	if (cgroup_path[0] == '/')
		cgroup_path++;

	if ( cgmanager_remove_sync(NULL, cgroup_manager, controller,
				   cgroup_path, recursive, &existed) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: cgm_remove for controller=%s, cgroup_path=%s, recursive=%d failed: %s",
		          controller, cgroup_path, recursive, nerr->message);
		nih_free(nerr);
		return false;
	}

	if (existed == -1) {
		log_error("cgmanager: cgm_remove failed: %s:%s did not exist", controller, cgroup_path);
		return false;
	}
	return true;
}

char *cgm_get(const char *controller, const char *cgroup_path, const char *key)
{
	char *result = NULL;
	char *ret = NULL;

	if (cgroup_path[0] == '/')
		cgroup_path++;

	if (cgmanager_get_value_sync(NULL, cgroup_manager, controller, cgroup_path, key, &result) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: cgm_get for controller=%s, cgroup_path=%s failed: %s",
		          controller, cgroup_path, nerr->message);
		nih_free(nerr);
		nih_free(result);
		return NULL;
	}

	if (result) {
		ret = strdup(result);
		nih_free(result);
	}

	return ret;
}

bool cgm_chmod(const char *controller, const char *cgroup_path, int mode)
{
	nih_local char *path_dirname = NULL;
	nih_local char *path_basename = NULL;

	if (cgroup_path[0] == '/')
		cgroup_path++;

	path_dirname = NIH_MUST(nih_strdup(NULL, cgroup_path));
	path_basename = NIH_MUST(nih_strdup(NULL, cgroup_path));

	if (cgmanager_chmod_sync(NULL, cgroup_manager, controller,
			dirname(path_dirname), basename(path_basename), mode) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: cgm_chmod for controller=%s, cgroup_path=%s, mode=%d failed: %s",
		          controller, cgroup_path, mode, nerr->message);
		nih_free(nerr);
		return false;
	}

	return true;
}

bool cgm_chown(const char *controller, const char *cgroup_path, int uid, int gid)
{
	if (cgroup_path[0] == '/')
		cgroup_path++;

	if (cgmanager_chown_sync(NULL, cgroup_manager, controller,
			cgroup_path, uid, gid) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: cgm_chown for controller=%s, cgroup_path=%s, uid=%d, gid=%d failed: %s",
		          controller, cgroup_path, uid, gid, nerr->message);
		nih_free(nerr);
		return false;
	}

	return true;
}

bool cgm_list_children(const char *controller, const char *cgroup_path, char ***children)
{
	if (cgroup_path[0] == '/')
		cgroup_path++;

	if ( cgmanager_list_children_sync(NULL, cgroup_manager, controller,
				       cgroup_path, children) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: cgm_list_children for controller=%s, cgroup_path=%s failed: %s",
		          controller, cgroup_path, nerr->message);
		nih_free(nerr);
		return false;
	}

	return true;
}

bool cgm_controller_exists(const char *controller)
{
	char *cgroup_name = NULL;

	if ( cgmanager_get_pid_cgroup_sync(NULL, cgroup_manager, controller,
				        getpid(), &cgroup_name) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		nih_free(nerr);
		return false;
	}

	nih_free(cgroup_name);
	return true;
}

bool cgm_enter(const char *controller, const char *cgroup_path, pid_t pid)
{
	if (cgroup_path[0] == '/')
		cgroup_path++;

	if (cgmanager_move_pid_sync(NULL, cgroup_manager, controller,
			cgroup_path, pid) != 0) {
		NihError *nerr;
		nerr = nih_error_get();
		log_error("cgmanager: cgm_enter for controller=%s, cgroup_path=%s, pid=%d failed: %s",
		          controller, cgroup_path, pid, nerr->message);
		nih_free(nerr);
		return false;
	}
	return true;
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include "erl_nif.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

typedef struct {
	ErlNifThreadOpts	*opts;
	ErlNifTid		thread_;
	ErlNifPid	  	pid_;
	char*			dir_;
	char*			mask_;
} thread_ref_t;

char* mask_to_string(ulong mask) {
	if (mask & IN_CLOSE) 		return "close";
	if (mask & IN_ACCESS) 		return "access";
	if (mask & IN_ATTRIB) 		return "attrib";
	if (mask & IN_CLOSE_WRITE) 	return "close_write";
	if (mask & IN_CLOSE_NOWRITE) 	return "close_no_write";
	if (mask & IN_CREATE) 		return "create";
	if (mask & IN_DELETE) 		return "delete";
	if (mask & IN_DELETE_SELF)	return "delete_self";
	if (mask & IN_MODIFY) 		return "modify";
	if (mask & IN_MOVE_SELF) 	return "move_self";
	if (mask & IN_MOVED_FROM) 	return "moved_from";
	if (mask & IN_MOVED_TO) 	return "moved_to";
	if (mask & IN_OPEN) 		return "open";
	if (mask & IN_IGNORED) 		return "ignored";
	if (mask & IN_ISDIR) 		return "is_dir";
	if (mask & IN_Q_OVERFLOW) 	return "q_overflow";
	if (mask & IN_UNMOUNT) 		return "unmount";
  }

uint32_t string_to_mask(char* mask) {
	if (0 == strcmp("all", mask)) 			return IN_ALL_EVENTS;
	if (0 == strcmp("close", mask)) 		return IN_CLOSE;
	if (0 == strcmp("access", mask)) 		return IN_ACCESS;
	if (0 == strcmp("attrib", mask)) 		return IN_ATTRIB;
	if (0 == strcmp("close_write", mask)) 		return IN_CLOSE_WRITE;
	if (0 == strcmp("close_no_write", mask))	return IN_CLOSE_NOWRITE;
	if (0 == strcmp("create", mask)) 		return IN_CREATE;
	if (0 == strcmp("delete", mask)) 		return IN_DELETE;
	if (0 == strcmp("delete_self", mask)) 		return IN_DELETE_SELF;
	if (0 == strcmp("modify", mask)) 		return IN_MODIFY;
	if (0 == strcmp("move_self", mask)) 		return IN_MOVE_SELF;
	if (0 == strcmp("moved_from", mask)) 		return IN_MOVED_FROM;
	if (0 == strcmp("moved_to", mask))		return IN_MOVED_TO;
	if (0 == strcmp("open", mask)) 			return IN_OPEN;
	if (0 == strcmp("ignored", mask)) 		return IN_IGNORED;
	if (0 == strcmp("is_dir", mask)) 		return IN_ISDIR;
	if (0 == strcmp("q_overflow", mask)) 		return IN_Q_OVERFLOW;
	if (0 == strcmp("unmount", mask)) 		return IN_UNMOUNT;
}

int handle_monitor(int fd, thread_ref_t *thread_ref, int flag) {
	char buf[BUF_LEN];
	size_t i = 0, len;
	ErlNifEnv *env = enif_alloc_env();
	len = read(fd, buf, BUF_LEN);
	if (len < 0) { 
		enif_send(NULL, &(thread_ref->pid_), env, enif_make_atom(env, "event_error"));
		enif_clear_env(env);
	}
	while (i < len && flag == 0) {
		struct inotify_event *event = (struct inotify_event *) &buf[i];
		ERL_NIF_TERM mask = enif_make_atom(env,  mask_to_string(event->mask));
		ERL_NIF_TERM path = enif_make_string(env, event->name, ERL_NIF_LATIN1);
		ERL_NIF_TERM tuple = enif_make_tuple2(env, mask, path); 
		enif_send(NULL, &(thread_ref->pid_), env, tuple);
		enif_clear_env(env);
		if (0 == strcmp("close", event->name)) {
			flag++;
			break;
		}
		i += EVENT_SIZE + event->len;
	}
	return flag;
}

int handle_io(int fd, fd_set rfds) {
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, NULL);
}

void monitor_loop(int fd, thread_ref_t *thread_ref) {
	int flag = 0, ret;
	fd_set rfds;
	ErlNifEnv *env = enif_alloc_env();
	while (flag == 0) {
		ret = handle_io(fd, rfds);
		FD_CLR(fd, &rfds);
		if (ret < 0) {
			enif_send(NULL, &(thread_ref->pid_), env, enif_make_atom(env, "io_error"));
			enif_clear_env(env);
		}
		else if (!ret) {
			enif_send(NULL, &(thread_ref->pid_), env, enif_make_atom(env, "io_timed_out"));
			enif_clear_env(env);
		}
		else 
			flag = handle_monitor(fd, thread_ref, flag);
	}
}

static void* monitor(void *obj) {
	int fd, wd;
	ErlNifEnv *env = enif_alloc_env();
	thread_ref_t *thread_ref = (thread_ref_t *) obj;
	fd = inotify_init();
	if (fd < 0) {
		enif_send(NULL, &(thread_ref->pid_), env, enif_make_atom(env, "watcher_not_started"));
		enif_clear_env(env);
	}
	wd = inotify_add_watch(fd, (char*) &(thread_ref->dir_), string_to_mask((char*) &(thread_ref->mask_)));
	if (wd < 0) { 
		enif_send(NULL, &(thread_ref->pid_), env, enif_make_atom(env, "not_found"));
		enif_clear_env(env);
	}
	else {
		monitor_loop(fd, thread_ref);
		inotify_rm_watch(fd, wd);
	}
	close(fd);
	return NULL;
}

static ERL_NIF_TERM _watch_(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
	ErlNifBinary dir_bin;
	char* dir;
	thread_ref_t *thread_ref = (thread_ref_t *) enif_alloc(sizeof(thread_ref_t));
	if (thread_ref == NULL)
		 return enif_make_badarg(env);
	if (enif_inspect_iolist_as_binary(env, argv[0], &dir_bin)) {
		dir = malloc(dir_bin.size + 1);
		memcpy(dir, dir_bin.data, dir_bin.size);
		dir[dir_bin.size] = '\0';
		memcpy((char*) &(thread_ref->dir_), dir, dir_bin.size + 1);
	}
	else
		perror("directory string");
	thread_ref->opts = enif_thread_opts_create("thr_opts");
	enif_get_atom(env, argv[1], (char*) &(thread_ref->mask_), 20, ERL_NIF_LATIN1);
    	enif_self(env, &(thread_ref->pid_));
      	if (enif_thread_create("", &(thread_ref->thread_), monitor, thread_ref, thread_ref->opts) != 0)
		return enif_make_badarg(env);
	return enif_make_atom(env, "ok");
}

static ErlNifFunc nif_funcs[] = {{"_watch_", 2, _watch_}};
ERL_NIF_INIT(Elixir.Cassius, nif_funcs, NULL, NULL, NULL, NULL);

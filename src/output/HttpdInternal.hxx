/*
 * Copyright (C) 2003-2011 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file
 *
 * Internal declarations for the "httpd" audio output plugin.
 */

#ifndef MPD_OUTPUT_HTTPD_INTERNAL_H
#define MPD_OUTPUT_HTTPD_INTERNAL_H

#include "HttpdClient.hxx"
#include "output_internal.h"
#include "timer.h"
#include "thread/Mutex.hxx"

#include <glib.h>

#include <forward_list>

#include <stdbool.h>

class ServerSocket;
class HttpdClient;

struct HttpdOutput {
	struct audio_output base;

	/**
	 * True if the audio output is open and accepts client
	 * connections.
	 */
	bool open;

	/**
	 * The configured encoder plugin.
	 */
	struct encoder *encoder;

	/**
	 * Number of bytes which were fed into the encoder, without
	 * ever receiving new output.  This is used to estimate
	 * whether MPD should manually flush the encoder, to avoid
	 * buffer underruns in the client.
	 */
	size_t unflushed_input;

	/**
	 * The MIME type produced by the #encoder.
	 */
	const char *content_type;

	/**
	 * This mutex protects the listener socket and the client
	 * list.
	 */
	mutable Mutex mutex;

	/**
	 * A #timer object to synchronize this output with the
	 * wallclock.
	 */
	struct timer *timer;

	/**
	 * The listener socket.
	 */
	ServerSocket *server_socket;

	/**
	 * The header page, which is sent to every client on connect.
	 */
	Page *header;

	/**
	 * The metadata, which is sent to every client.
	 */
	Page *metadata;

	/**
	 * The configured name.
	 */
	char const *name;
	/**
	 * The configured genre.
	 */
	char const *genre;
	/**
	 * The configured website address.
	 */
	char const *website;

	/**
	 * A linked list containing all clients which are currently
	 * connected.
	 */
	std::forward_list<HttpdClient> clients;

	/**
	 * A temporary buffer for the httpd_output_read_page()
	 * function.
	 */
	char buffer[32768];

	/**
	 * The maximum and current number of clients connected
	 * at the same time.
	 */
	guint clients_max, clients_cnt;

	bool Bind(GError **error_r);
	void Unbind();

	/**
	 * Caller must lock the mutex.
	 */
	bool OpenEncoder(struct audio_format *audio_format,
			 GError **error_r);

	/**
	 * Caller must lock the mutex.
	 */
	bool Open(struct audio_format *audio_format, GError **error_r);

	/**
	 * Caller must lock the mutex.
	 */
	void Close();

	/**
	 * Check whether there is at least one client.
	 *
	 * Caller must lock the mutex.
	 */
	gcc_pure
	bool HasClients() const {
		return !clients.empty();
	}

	/**
	 * Check whether there is at least one client.
	 */
	gcc_pure
	bool LockHasClients() const {
		const ScopeLock protect(mutex);
		return HasClients();
	}

	void AddClient(int fd);

	/**
	 * Removes a client from the httpd_output.clients linked list.
	 */
	void RemoveClient(HttpdClient &client);

	/**
	 * Sends the encoder header to the client.  This is called
	 * right after the response headers have been sent.
	 */
	void SendHeader(HttpdClient &client) const;

	/**
	 * Reads data from the encoder (as much as available) and
	 * returns it as a new #page object.
	 */
	Page *ReadPage();

	/**
	 * Broadcasts a page struct to all clients.
	 *
	 * Mutext must not be locked.
	 */
	void BroadcastPage(Page *page);

	/**
	 * Broadcasts data from the encoder to all clients.
	 */
	void BroadcastFromEncoder();

	bool EncodeAndPlay(const void *chunk, size_t size, GError **error_r);

	void SendTag(const struct tag *tag);
};

#endif

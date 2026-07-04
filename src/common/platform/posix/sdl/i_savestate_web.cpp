/*
** i_savestate_web.cpp
** Emscripten-only: back up / restore the on-disk save games as a single zip
** archive so the JavaScript front-end can persist them to IndexedDB (local) or
** to a cloud server.
**
** Two functions are exported to JS (via EMSCRIPTEN_KEEPALIVE):
**
**   neil_serialize()    - zip everything under /Save into /savestate.gz, then
**                         call back window.myApp.SaveStateEvent(success). The
**                         JS side then stores /savestate.gz locally or uploads
**                         it.
**   neil_unserialize()  - read /savestate.gz (which JS has written into MEMFS),
**                         delete the current /Save tree, then extract the
**                         archive back into /Save (recreating it). JS calls this
**                         after fetching the archive from IndexedDB / the server.
**
** The archive is a plain ZIP (miniz), holding every regular file under /Save
** with paths relative to /Save (e.g. "doom.id.doom2.commercial/save00.zds").
**---------------------------------------------------------------------------
*/

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Match how miniz.c is compiled for the web build (Makefile: -DMINIZ_NO_STDIO)
// so the struct layouts line up. We only use the in-memory (heap) archive APIs;
// all real file I/O below is done with plain C stdio, which MEMFS supports.
#define MINIZ_NO_STDIO
#include "miniz.h"

// The top-level save directory (see save_dir default in savegamemanager.cpp for
// the web build) and the archive path JS reads/writes in MEMFS.
#define NEIL_SAVE_ROOT    "/Save"
#define NEIL_SAVE_ARCHIVE "/savestate.gz"

//==========================================================================
//
// helpers
//
//==========================================================================

// Recursively collect every regular file under base, storing paths relative to
// base (forward slashes) in out.
static void NeilCollectFiles(const std::string &base, const std::string &rel, std::vector<std::string> &out)
{
	std::string dirpath = base;
	if (!rel.empty())
		dirpath += "/" + rel;

	DIR *d = opendir(dirpath.c_str());
	if (d == nullptr)
		return;

	struct dirent *ent;
	while ((ent = readdir(d)) != nullptr)
	{
		const std::string name = ent->d_name;
		if (name == "." || name == "..")
			continue;

		const std::string childRel = rel.empty() ? name : rel + "/" + name;
		const std::string childFull = base + "/" + childRel;

		struct stat st;
		if (stat(childFull.c_str(), &st) != 0)
			continue;

		if (S_ISDIR(st.st_mode))
			NeilCollectFiles(base, childRel, out);
		else if (S_ISREG(st.st_mode))
			out.push_back(childRel);
	}

	closedir(d);
}

// Recursively delete a directory and everything under it.
static void NeilRemoveTree(const std::string &path)
{
	DIR *d = opendir(path.c_str());
	if (d != nullptr)
	{
		struct dirent *ent;
		while ((ent = readdir(d)) != nullptr)
		{
			const std::string name = ent->d_name;
			if (name == "." || name == "..")
				continue;

			const std::string child = path + "/" + name;
			struct stat st;
			if (stat(child.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
				NeilRemoveTree(child);
			else
				unlink(child.c_str());
		}
		closedir(d);
	}
	rmdir(path.c_str());
}

// Create an absolute directory path, making each intermediate component.
static void NeilMakeDirs(const std::string &dir)
{
	std::string path;
	size_t start = 0;
	while (start < dir.size())
	{
		size_t slash = dir.find('/', start);
		const std::string part = (slash == std::string::npos)
			? dir.substr(start)
			: dir.substr(start, slash - start);

		if (!part.empty())
		{
			path += "/" + part;
			mkdir(path.c_str(), 0777);
		}

		if (slash == std::string::npos)
			break;
		start = slash + 1;
	}
}

// Read an entire file into buf. Returns false if the file can't be opened.
static bool NeilReadFile(const std::string &path, std::vector<unsigned char> &buf)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (f == nullptr)
		return false;

	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (sz < 0)
		sz = 0;

	buf.resize((size_t)sz);
	if (sz > 0)
	{
		if (fread(buf.data(), 1, (size_t)sz, f) != (size_t)sz)
		{
			fclose(f);
			return false;
		}
	}
	fclose(f);
	return true;
}

static void NeilNotifySaved(bool success)
{
	EM_ASM({
		if (window.myApp && window.myApp.SaveStateEvent)
			window.myApp.SaveStateEvent($0 != 0);
	}, success ? 1 : 0);
}

static void NeilNotifyLoaded(bool success)
{
	EM_ASM({
		if (window.myApp && window.myApp.LoadStateEvent)
			window.myApp.LoadStateEvent($0 != 0);
	}, success ? 1 : 0);
}

//==========================================================================
//
// coalescing auto-backup timer
//
// A save may schedule a JS-side backup (zip /Save -> cloud / IndexedDB). How it
// does so depends on the cloud login state:
//
//   * Logged in  -> only deliberate menu / named saves back up, and the player
//                   makes those one at a time, so there's nothing to coalesce:
//                   fire the backup immediately.
//   * Logged out -> every save (including the frequent autosaves fired on level
//                   transitions, and the rotating quicksave) backs up to cheap
//                   local IndexedDB. To avoid re-zipping + re-storing the whole
//                   /Save tree once per individual save, arm a short countdown
//                   measured in main-loop frames; the engine ticks it down once
//                   per rendered frame (Neil_TickSaveBackup, from D_DoomLoopFrame)
//                   and fires the backup when it reaches zero. A fresh save while
//                   the timer is still running re-arms it back to the top, so a
//                   burst collapses into a single backup a beat after the last
//                   save settles.
//
//==========================================================================

#define NEIL_BACKUP_FRAMES 60

static int neil_backup_countdown = 0;   // >0 => a backup is pending, in frames

static void NeilFireBackup()
{
	EM_ASM({
		if (window.myApp && window.myApp.onGameSaved)
			window.myApp.onGameSaved();
	});
}

// Called for a just-completed save. menuSave != 0 for a deliberate menu / named
// save (autosaves and the rotating quicksave pass 0). Applies the policy above.
void Neil_ArmSaveBackup(int menuSave)
{
	const int loggedIn = EM_ASM_INT({
		return (window.myApp && window.myApp.loggedIn) ? 1 : 0;
	});

	if (loggedIn)
	{
		// Only deliberate menu saves back up when logged in; do it immediately.
		if (menuSave)
		{
			neil_backup_countdown = 0;   // cancel any pending logged-out timer
			NeilFireBackup();
		}
	}
	else
	{
		// Logged out: back up every save, but coalesce a burst via the countdown.
		neil_backup_countdown = NEIL_BACKUP_FRAMES;
	}
}

// Tick once per main-loop frame; fire the backup when the countdown expires.
void Neil_TickSaveBackup()
{
	if (neil_backup_countdown > 0 && --neil_backup_countdown == 0)
		NeilFireBackup();
}

//==========================================================================
//
// exported entry points
//
//==========================================================================

extern "C" {

// Zip up /Save -> /savestate.gz and notify JS.
EMSCRIPTEN_KEEPALIVE
void neil_serialize()
{
	std::vector<std::string> files;
	NeilCollectFiles(NEIL_SAVE_ROOT, "", files);

	mz_zip_archive zip;
	memset(&zip, 0, sizeof(zip));
	if (!mz_zip_writer_init_heap(&zip, 0, 0))
	{
		NeilNotifySaved(false);
		return;
	}

	for (const std::string &rel : files)
	{
		const std::string full = std::string(NEIL_SAVE_ROOT) + "/" + rel;
		std::vector<unsigned char> data;
		if (!NeilReadFile(full, data))
			continue;

		mz_zip_writer_add_mem(&zip, rel.c_str(),
			data.empty() ? (const void *)"" : (const void *)data.data(),
			data.size(), MZ_DEFAULT_LEVEL);
	}

	void *outbuf = nullptr;
	size_t outsize = 0;
	mz_bool ok = mz_zip_writer_finalize_heap_archive(&zip, &outbuf, &outsize);
	mz_zip_writer_end(&zip);

	bool wrote = false;
	if (ok && outbuf != nullptr)
	{
		FILE *out = fopen(NEIL_SAVE_ARCHIVE, "wb");
		if (out != nullptr)
		{
			wrote = (fwrite(outbuf, 1, outsize, out) == outsize);
			fclose(out);
		}
	}
	if (outbuf != nullptr)
		mz_free(outbuf);

	printf("neil_serialize: %zu file(s) -> %s (%zu bytes)\n",
		files.size(), NEIL_SAVE_ARCHIVE, outsize);

	NeilNotifySaved(wrote);
}

// Read /savestate.gz, wipe /Save, and extract the archive back into /Save.
EMSCRIPTEN_KEEPALIVE
void neil_unserialize()
{
	std::vector<unsigned char> data;
	if (!NeilReadFile(NEIL_SAVE_ARCHIVE, data) || data.empty())
	{
		printf("neil_unserialize: no archive at %s\n", NEIL_SAVE_ARCHIVE);
		NeilNotifyLoaded(false);
		return;
	}

	mz_zip_archive zip;
	memset(&zip, 0, sizeof(zip));
	if (!mz_zip_reader_init_mem(&zip, data.data(), data.size(), 0))
	{
		printf("neil_unserialize: not a valid archive\n");
		NeilNotifyLoaded(false);
		return;
	}

	// Start clean: remove the whole existing tree, then recreate the root.
	NeilRemoveTree(NEIL_SAVE_ROOT);
	mkdir(NEIL_SAVE_ROOT, 0777);

	mz_uint count = mz_zip_reader_get_num_files(&zip);
	mz_uint extracted = 0;
	for (mz_uint i = 0; i < count; ++i)
	{
		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(&zip, i, &st))
			continue;

		std::string rel = st.m_filename;
		if (rel.empty())
			continue;
		// Skip directory entries; parents are created from the file paths below.
		if (rel.back() == '/')
			continue;
		// Normalise separators just in case an archive used backslashes.
		for (char &c : rel)
			if (c == '\\')
				c = '/';

		const std::string full = std::string(NEIL_SAVE_ROOT) + "/" + rel;

		const size_t slash = full.find_last_of('/');
		if (slash != std::string::npos)
			NeilMakeDirs(full.substr(0, slash));

		size_t outsize = 0;
		void *p = mz_zip_reader_extract_to_heap(&zip, i, &outsize, 0);
		if (p != nullptr)
		{
			FILE *out = fopen(full.c_str(), "wb");
			if (out != nullptr)
			{
				fwrite(p, 1, outsize, out);
				fclose(out);
				++extracted;
			}
			mz_free(p);
		}
	}

	mz_zip_reader_end(&zip);

	printf("neil_unserialize: extracted %u/%u file(s) into %s\n",
		extracted, count, NEIL_SAVE_ROOT);

	NeilNotifyLoaded(true);
}

} // extern "C"

#endif // __EMSCRIPTEN__

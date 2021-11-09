/*
 * This file code is based on code examples in libbluray library
 * Copyright (C) 2016 Taapat
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
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <Python.h>
#include <dirent.h>

#include <libbluray/bluray.h>
#include <udfread/udfread.h>

typedef struct {
	uint64_t duration;
	char clip_id[128];
	char languages[256];
	char coding_type[512];
	uint32_t chapters;
	int title_id;
} titlelist;

typedef struct {
	int value;
	const char *str;
} VALUE_MAP;

/* Codec map see in enigma pmtparse and servicedvb */
const VALUE_MAP codec_map[] = {
	{0x01, "MPEG-1 Video"},
	{0x02, "MPEG-2 Video"},
	{0x03, "MPEG"},
	{0x04, "MPEG"},
	{0x80, "LPCM"},
	{0x81, "AC3"},
	{0x82, "DTS"},
	{0x83, "TrueHD"},
	{0x84, "AC3+"},
	{0x85, "DTS-HD"},
	{0x86, "DTS-HD"},
	{0xa1, "AC3"},
	{0xa2, "DTS"},
	{0xea, "MPEG"},
	{0x1b, "???"},
	{0x90, "Presentation Graphics"},
	{0x91, "Interactive Graphics"},
	{0x92, "Text Subtitle"},
	{0, NULL}
};

static const char *_lookup_str(const VALUE_MAP *map, int val)
{
	int ii;

	for (ii = 0; map[ii].str; ii++)
		if (val == map[ii].value)
			return map[ii].str;

	return "???";
}

static char *_mk_path(const char *base, const char *sub)
{
	size_t n1 = strlen(base);
	size_t n2 = strlen(sub);
	char *result = (char*)malloc(n1 + n2 + 2);
	if (result) {
		strcpy(result, base);
		strcat(result, "/");
		strcat(result, sub);
	}
	return result;
}

static int storeInfo(BLURAY_TITLE_INFO* ti, titlelist *tList, int pos)
{
	int ii;

	tList[pos].duration = ti->duration;
	tList[pos].chapters = ti->chapter_count;

	for (ii = 0; ii < ti->clip_count; ii++) {
		char *clip = NULL;
		clip = _mk_path(tList[pos].clip_id, ti->clips[ii].clip_id);
		if (clip == NULL)
			continue;
		strcpy(tList[pos].clip_id, clip);
		free(clip);
	}

	BLURAY_CLIP_INFO *ci = &ti->clips[0];
	for (ii = 0; ii < ci->audio_stream_count; ii++) {
		char *lang = NULL, *coding = NULL;
		lang = _mk_path(tList[pos].languages, (const char *)ci->audio_streams[ii].lang);
		if (lang == NULL)
			continue;
		strcpy(tList[pos].languages, lang);
		free(lang);

		coding = _mk_path(tList[pos].coding_type, _lookup_str(codec_map, ci->audio_streams[ii].coding_type));
		if (coding == NULL)
			continue;
		strcpy(tList[pos].coding_type, coding);
		free(coding);
	}

	return 0;
}

static int parseInfo(const char *bd_path, titlelist *tList)
{
	int ii, pos = 0, ret = 0;

	BLURAY *bd = bd_open(bd_path, NULL);
	if (!bd) {
		fprintf(stderr, "[blurayinfo] Failed to open:%s\n", bd_path);
		return ret;
	}

	int title_count = bd_get_titles(bd, TITLES_RELEVANT, 180);
	if (title_count == 0) {
		fprintf(stderr, "[blurayinfo] No usable playlists found!\n");
		goto fail;
	}
	if (title_count > 99)
		title_count = 99;

	int main_title = bd_get_main_title(bd);
	BLURAY_TITLE_INFO* ti = bd_get_title_info(bd, main_title, 0);
	tList[pos].title_id = main_title;
	storeInfo(ti, tList, pos++);
	bd_free_title_info(ti);

	for (ii = 0; ii < title_count; ii++) {
		BLURAY_TITLE_INFO* ti = bd_get_title_info(bd, ii, 0);
		if (ii != main_title) {
			tList[pos].title_id = ii;
			storeInfo(ti, tList, pos++);
		}
		bd_free_title_info(ti);
	}

	ret = 1;
fail:
	bd_close(bd);
	return ret;
}

titlelist *newTitleList(void)
{
	titlelist *tList = malloc(sizeof(titlelist)*100);
	if(!tList)
		exit(0);

	memset(tList, 0, sizeof(titlelist)*100);
	return tList;
}

void freeTitleList(titlelist *tList)
{
	free(tList);
}

PyObject *_getTitles(PyObject *self, PyObject *args)
{
	titlelist *tList;
	int i;
	char *s;
	PyObject *plist, *result, *duration, *clip_id, *languages, *coding_type, *chapters, *title_id;

	if(!PyArg_ParseTuple(args, "s", &s)) {
		fprintf(stderr, "[blurayinfo] getTitles: wrong arguments!\n");
		return NULL;
	}

	if(!(plist = PyList_New(0)))
		return NULL;

	if(!(result = PyList_New(0)))
		return NULL;

	tList = newTitleList();
	if(!parseInfo(s, tList)) {
		fprintf(stderr, "[blurayinfo] getTitles: error in parse!\n");
		return NULL;
	}

	for (i = 0; i < 100; i++) {
		if(tList[i].clip_id[0] == '\0')
			break;
		else {
			duration = Py_BuildValue("k", (unsigned long)tList[i].duration);
			clip_id = Py_BuildValue("s", tList[i].clip_id);
			languages = Py_BuildValue("s", tList[i].languages);
			coding_type = Py_BuildValue("s", tList[i].coding_type);
			chapters = Py_BuildValue("i", tList[i].chapters);
			title_id = Py_BuildValue("i", tList[i].title_id);
			PyList_Append(plist, duration);
			PyList_Append(plist, clip_id);
			PyList_Append(plist, languages);
			PyList_Append(plist, coding_type);
			PyList_Append(plist, chapters);
			PyList_Append(plist, title_id);
			PyList_Append(result, plist);
			if(!(plist = PyList_New(0))) {
				freeTitleList(tList);
				return NULL;
			}
		}
	}

	freeTitleList(tList);
	return result;
}

static int parseChapters(const char *bd_path, titlelist *tList, int title_id, int clip_id)
{
	int ii, ret = 0, pos = 0;
	uint64_t start_time = 2000000, end_time = 0;

	BLURAY *bd = bd_open(bd_path, NULL);
	if (!bd) {
		fprintf(stderr, "[blurayinfo] Failed to open %s\n", bd_path);
		return 0;
	}

	int title_count = bd_get_titles(bd, TITLES_RELEVANT, 180);
	if (title_count == 0) {
		fprintf(stderr, "[blurayinfo] No usable playlists found!\n");
		goto fail;
	}

	BLURAY_TITLE_INFO* ti = bd_get_title_info(bd, title_id, 0);
	for (ii = 0; ii <= clip_id; ii++) {
		if (end_time)
			start_time += end_time;
		end_time += ti->clips[ii].out_time - ti->clips[ii].in_time;
	}

	int chapter_count = ti->chapter_count;
	if (chapter_count > 99)
		chapter_count = 99;

	for (ii = 0; ii < chapter_count; ii++) {
		uint64_t chaper_time = ti->chapters[ii].start;
		if (chaper_time > start_time && chaper_time <= end_time)
			tList[pos++].duration = chaper_time;
	}
	tList[pos].duration = -1;
	bd_free_title_info(ti);

	ret = 1;
fail:
	bd_close(bd);
	return ret;
}

PyObject *_getChapters(PyObject *self, PyObject *args)
{
	titlelist *tList;
	int i, c;
	char *s;
	PyObject *result, *duration;

	if(!PyArg_ParseTuple(args, "sii", &s, &i, &c)) {
		fprintf(stderr, "[blurayinfo] getChapters: wrong arguments!\n");
		return NULL;
	}

	if(!(result = PyList_New(0)))
		return NULL;

	tList = newTitleList();
	if(!parseChapters(s, tList, i, c)) {
		fprintf(stderr, "[blurayinfo] getChapters: error in parse!\n");
		return NULL;
	}

	for (i = 0; i < 1000; i++) {
		if(tList[i].duration == -1)
			break;
		duration = Py_BuildValue("k", (unsigned long)tList[i].duration);
		PyList_Append(result, duration);
	}

	freeTitleList(tList);
	return result;
}

static int _lsdir(udfread *udf, const char *path, const char *dirname)
{
	int ret = 0;
	struct udfread_dirent dirent;
	UDFDIR *dir = udfread_opendir(udf, path);

	if (!dir) {
		fprintf(stderr, "[blurayinfo] udfread_opendir(%s) failed!\n", path);
		return 0;
	}

	while (udfread_readdir(dir, &dirent)) {
		if (dirent.d_type == UDF_DT_DIR && !strcmp(dirent.d_name, dirname)) {
			ret = 1;
			break;
		}
	}

	udfread_closedir(dir);
	return ret;
}

static int blurayDir(const char *path)
{
	int ret = 0;
	udfread *udf = udfread_init();

	if (!udf) {
		fprintf(stderr, "[blurayinfo] udfread_init() failed!\n");
		return 0;
	}

	if (udfread_open(udf, path) < 0) {
		fprintf(stderr, "[blurayinfo] udfread_open(%s) failed!\n", path);
		udfread_close(udf);
		return 0;
	}

	if (_lsdir(udf, "/", "BDMV") && _lsdir(udf, "BDMV/", "PLAYLIST"))
		ret = 1;

	udfread_close(udf);
	return ret;
}

PyObject *_isBluray(PyObject *self, PyObject *args)
{
	int ret = 0;
	char *s;

	if(!PyArg_ParseTuple(args, "s", &s)) {
		fprintf(stderr, "[blurayinfo] isBluray: wrong arguments!\n");
		return NULL;
	}

	if (blurayDir(s))
		ret = 1;

	return Py_BuildValue("i", ret);
}

static PyMethodDef blurayinfo_funcs[] = {
	{"getTitles", _getTitles, METH_VARARGS,
		"Return bluray disc title info"},
	{"getChapters", _getChapters, METH_VARARGS,
		"Get chapters from title"},
	{"isBluray", _isBluray, METH_VARARGS,
		"Check if folder structure is as in bluray disc"},
	{NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef moduledef =
{
    PyModuleDef_HEAD_INIT,
    "blurayinfo", "Get info from bluray disc",
    -1,
    blurayinfo_funcs
};

PyMODINIT_FUNC PyInit_blurayinfo(void)
{
    return PyModule_Create(&moduledef);
}

#else

void initblurayinfo(void)
{
	Py_InitModule("blurayinfo", blurayinfo_funcs);
}

#endif


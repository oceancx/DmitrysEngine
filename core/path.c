
/* Copyright (c) 2017-2019 Dmitry Stepanov a.k.a mr.DIMAS
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

void de_path_init(de_path_t* path)
{
	de_str8_init(&path->str);
}

void de_path_free(de_path_t* path)
{
	de_str8_free(&path->str);
}

void de_path_clear(de_path_t* path)
{
	de_str8_clear(&path->str);
}

void de_path_from_cstr_as_view(de_path_t* path, const char* cstr)
{
	path->str.str.data = (char*)cstr;
	path->str.str.size = strlen(cstr) + 1;
	path->str.str._capacity = path->str.str.size;
}

void de_path_copy(const de_path_t* src, de_path_t* dest)
{
	de_str8_copy(&src->str, &dest->str);
}

void de_path_as_str8_view(de_path_t* path, de_str8_t* str)
{
	path->str.str.data = str->str.data;
	path->str.str.size = str->str.size;
	path->str.str._capacity = str->str._capacity;
}

const char* de_path_cstr(const de_path_t* path)
{
	return de_str8_cstr(&path->str);
}

void de_path_append_cstr(de_path_t* path, const char* utf8str)
{
	de_str8_append_cstr(&path->str, utf8str);
}

void de_path_append_str8(de_path_t* path, const de_str8_t* str)
{
	de_str8_append_str8(&path->str, str);
}

void de_path_append_str_view(de_path_t* path, const de_str8_view_t* view)
{
	de_str8_append_str_view(&path->str, view);
}

bool de_path_eq(const de_path_t* a, const de_path_t* b)
{
	return de_str8_eq_str8(&a->str, &b->str);
}

bool de_path_is_empty(const de_path_t* src)
{
	return !src || src->str.str.size == 0;
}

void de_path_extension(const de_path_t* p, de_str8_view_t* ext)
{
	if(!de_path_is_empty(p)) {
		const char* dot_pos = strrchr(de_path_cstr(p), '.');
		if (dot_pos) {
			de_str8_view_set(ext, dot_pos, strlen(dot_pos));
			return;
		}
	}
	de_str8_view_set(ext, NULL, 0);	
}

void de_path_file_name(const de_path_t* p, de_str8_view_t* name)
{
	const char* dot_pos, *slash_pos, *str;

	if (!p) {
		de_str8_view_set(name, NULL, 0);
		return;
	}

	str = de_path_cstr(p);

	if (!str) {
		de_str8_view_set(name, NULL, 0);
		return;
	}

	/* look for slash first */
	slash_pos = strrchr(str, '/');
	if (!slash_pos) {
		/* look for windows-style slashes */
		slash_pos = strrchr(str, '\\');

		if (!slash_pos) {
			/* no slashes, so only filename present */
			slash_pos = str;
		}
	}

	/* look for dot */
	dot_pos = strrchr(str, '.');
	if (!dot_pos) {
		/* no extension - everything from slash to the end is file name */
		de_str8_view_set(name, slash_pos, strlen(slash_pos));
	} else if (slash_pos < dot_pos) {
		/* everything between slash and dot is file name */
		de_str8_view_set(name, slash_pos, (size_t)(dot_pos - slash_pos));
	} else {
		/* invalid path */
		de_str8_view_set(name, NULL, 0);
	}
}
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

char * de_load_file_into_memory(const char * path, size_t* out_size)
{
	size_t file_size;
	size_t content_size;
	char* content;
	FILE* file;

	/* try to open file */
	file = fopen(path, "rb");
	if (!file) {
		de_fatal_error("unable to read file: %s", path);
	}

	/* get file size */
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	content_size = file_size + 1;
	fseek(file, 0, SEEK_SET);

	/* read file */
	content = (char*)de_malloc(content_size);
	if (fread(content, sizeof(char), file_size, file) != file_size) {
		de_fatal_error("file %s is corrupted", path);
	}

	if (out_size) {
		*out_size = file_size;
	}

	/* close file and write trailing zero at end of content array */
	fclose(file);
	content[file_size] = '\0';

	return content;
}


void de_convert_to_c_array(const char* source, const char* dest)
{
	size_t i;
	size_t size;
	FILE* out;
	unsigned char* data;

	data = (unsigned char*)de_load_file_into_memory(source, &size);

	out = fopen(dest, "w");

	fprintf(out, "static const unsigned char array[] = {\n");
	for (i = 0; i < size; ++i) {
		fprintf(out, "%u, ", data[i]);

		if (i % 20 == 0) {
			fprintf(out, "\n\t");
		}
	}

	fprintf(out, "\n};");

	de_free(data);
	fclose(out);
}

bool de_file_exists(const char* filename) 
{
	/* TODO: look for a better way */
	FILE* file = fopen(filename, "r");
	if(file) {
		fclose(file);
		return true;
	}
	return false;
}

void de_file_tests(void)
{
	DE_ASSERT(de_file_exists("___foobar_1234") == false);
	/* TODO: add more */
}
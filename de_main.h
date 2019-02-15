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

 /*
  * Important notes:
  * - Minumum supported version of Windows is Windows Vista (due to using of
  *   Windows conditional variables in implentation of threading routines, can be
  *   fixed if someone needs WinXP support)
  * - Engine uses right-handed coordinate system which means Z axis points towards
  *   screen, Y axis points up, and X axis points right.
  * - Vectors are single-column matrices.
  **/

#ifndef DE_DENGINE_H
#define DE_DENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Set this to 1 to disable asserts, could be useful for release builds to increase performance a bit. */
#define DE_DISABLE_ASSERTS 0

/* Suppress compiler-specific warnings */
#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#  pragma warning(disable : 4204 4820)
#elif defined __GNUC__
#  pragma GCC diagnostic ignored "-Woverlength-strings" /* built-in shaders does not fit in C89 limits of 512 chars */
#else
#error Compiler not supported
#endif

#define _USE_MATH_DEFINES
#define DE_UNUSED(x) ((void)x)
#define DE_BIT(n) (1 << n)
#define DE_STRINGIZE_(x) #x
#define DE_STRINGIZE(x) DE_STRINGIZE_(x)

#if DE_DISABLE_ASSERTS
#define DE_ASSERT
#else 
#define DE_ASSERT(expression) assert(expression)
#endif

typedef void(*de_proc)(void);

/* Standard library */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <process.h>

#if defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#include <pthread.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Platform-specific */
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#else
#  include <X11/Xlib.h>
#  define __USE_MISC
#  include <unistd.h>
#endif

/* OpenGL */
#include <GL/gl.h>
#include <GL/glext.h>

/* External header-only dependencies */
#ifdef _WIN32
#  include "GL/wglext.h"
#  include <mmsystem.h>
#  include "external/dsound.h"
#else
#  include "GL/glx.h"
#endif

/* Forward declarations */
typedef struct de_renderer_t de_renderer_t;
typedef struct de_node_t de_node_t;
typedef struct de_surface_t de_surface_t;
typedef struct de_body_t de_body_t;
typedef struct de_animation_track_t de_animation_track_t;
typedef struct de_texture_t de_texture_t;
typedef struct de_static_triangle_t de_static_triangle_t;
typedef struct de_static_geometry_t de_static_geometry_t;
typedef struct de_mesh_t de_mesh_t;
typedef struct de_light_t de_light_t;
typedef struct de_gui_t de_gui_t;
typedef struct de_core_t de_core_t;
typedef struct de_scene_t de_scene_t;
typedef struct de_sound_device_t de_sound_device_t;
typedef struct de_sound_source_t de_sound_source_t;
typedef struct de_sound_buffer_t de_sound_buffer_t;

/**
* Order is important here, because some parts depends on other
* Modules with minimum dependencies should be placed before others.
**/
#include "core/bool.h"
#include "core/log.h"
#include "core/byteorder.h"
#include "core/memmgr.h"
#include "core/array.h"
#include "core/base64.h"
#include "core/thread.h"
#include "core/string.h"
#include "core/string_utils.h"
#include "core/utf32string.h"
#include "core/path.h"
#include "core/linked_list.h"
#include "core/time.h"
#include "core/color.h"
#include "core/pool.h"
#include "input/input.h"
#include "core/event.h"
#include "resources/builtin_fonts.h"
#include "math/mathlib.h"
#include "core/serialization.h"
#include "math/triangulator.h"
#include "core/rect.h"
#include "renderer/vertex.h"	
#include "core/rectpack.h"
#include "resources/image.h"
#include "vg/vgraster.h"
#include "scene/camera.h"
#include "scene/mesh.h"
#include "scene/light.h"
#include "scene/node.h"
#include "scene/animation.h"
#include "scene/scene.h"
#include "physics/octree.h"
#include "physics/collision.h"
#include "renderer/surface.h"
#include "fbx/fbx.h"
#include "renderer/renderer.h"
#include "resources/texture.h"
#include "font/font.h"
#include "core/utility.h"
#include "gui/gui.h"
#include "sound/sound.h"
#include "core/core.h" 


/* TINFL (part of miniz) - used to decompress FBX data */
#ifdef DE_IMPLEMENTATION
#  define TINFL_IMPLEMENTATION
#endif

#include "external/miniz_tinfl.h"

/**
 * Implementation.
 * Not sensitive to order of includes.
 **/
#ifdef DE_IMPLEMENTATION
#  include "resources/impl/image.h"
#  include "core/impl/byteorder.h"
#  include "core/impl/array.h"
#  include "core/impl/color.h"
#  include "core/impl/pool.h"
#  include "core/impl/log.h" 
#  include "core/impl/memmgr.h"
#  include "core/impl/base64.h"
#  include "core/string_utils_impl.h"
#  include "core/string_impl.h"
#  include "core/utf32string_impl.h"
#  include "core/path_impl.h"
#  include "core/impl/rectpack.h"
#  include "core/impl/rect.h"
#  include "core/impl/utility.h"
#  include "core/impl/serialization.h"
#  include "core/impl/core.h"
#  include "physics/impl/octree.h"
#  include "physics/impl/collision.h"
#  include "fbx/impl/fbx.h"
#  include "font/impl/font.h"
#  include "math/impl/mathlib.h"
#  include "math/impl/triangulator.h"
#  include "scene/impl/animation.h"
#  include "scene/impl/camera.h"
#  include "scene/impl/light.h"
#  include "scene/impl/mesh.h"
#  include "scene/impl/node.h"
#  include "scene/impl/scene.h"
#  include "renderer/impl/renderer.h"
#  include "renderer/impl/surface.h"
#  include "resources/impl/texture.h"
#  include "gui/impl/gui.h" 
#  include "vg/impl/vgraster.h"
#  include "core/thread_impl.h"
#  include "sound/impl/sound.h"
#endif

#ifdef __cplusplus
}
#endif

#endif
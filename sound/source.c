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

de_sound_source_t* de_sound_source_create(de_sound_context_t* ctx, de_sound_source_type_t type) {
	de_sound_source_t* src;
	de_sound_context_lock(ctx);
	src = DE_NEW(de_sound_source_t);
	src->type = type;
	src->ctx = ctx;
	src->pitch = 1.0f;
	src->buf_read_pos = 0.0;
	src->current_sample_rate = 1.0f;
	src->status = DE_SOUND_SOURCE_STATUS_STOPPED;
	src->pan = 0;
	src->loop = false;
	src->left_gain = 1.0f;
	src->right_gain = 1.0f;
	src->radius = 10.0f;
	DE_ARRAY_APPEND(ctx->sounds, src);
	de_sound_context_unlock(ctx);
	return src;
}

void de_sound_source_free(de_sound_source_t* src) {
	de_sound_context_lock(src->ctx);
	DE_ARRAY_REMOVE(src->ctx->sounds, src);
	de_sound_context_unlock(src->ctx);
}

void de_sound_source_set_buffer(de_sound_source_t* src, de_sound_buffer_t* buf) {
	de_sound_context_lock(src->ctx);
	src->buffer = buf;
	src->playback_pos = 0;
	src->buf_read_pos = 0;
	de_sound_context_unlock(src->ctx);
}

void de_sound_source_sample(de_sound_source_t* src, float* left, float* right) {
	uint64_t i;

	/* source can be stopped during mixing, in this case write silence out */
	if (src->status & DE_SOUND_SOURCE_STATUS_STOPPED) {
		*left = 0;
		*right = 0;
		return;
	}

	src->buf_read_pos += src->current_sample_rate;
	src->playback_pos += src->current_sample_rate;

	i = (uint64_t)src->buf_read_pos;

	if (i >= src->buffer->sample_per_channel) {
		if (src->buffer->flags & DE_SOUND_BUFFER_FLAGS_STREAM) {
			de_sound_buffer_swap_pointers(src->buffer);
			de_sound_buffer_set_flags(src->buffer, DE_SOUND_BUFFER_FLAGS_UPLOAD_NEXT_BLOCK);
		}
		src->buf_read_pos = 0;
		i = 0;
	}

	if (src->playback_pos >= src->buffer->total_sample_per_channel) {
		src->playback_pos = 0;
		src->buf_read_pos = 0;
		src->status = src->loop ? DE_SOUND_SOURCE_STATUS_PLAYING : DE_SOUND_SOURCE_STATUS_STOPPED;
	}

	if (src->buffer->channel_count == 2) {
		*left = src->left_gain * src->buffer->read_ptr[i];
		*right = src->right_gain * src->buffer->read_ptr[i + src->buffer->sample_per_channel];
	} else if (src->buffer->channel_count == 1) {
		float sample = src->buffer->read_ptr[i];
		*left = src->left_gain * sample;
		*right = src->right_gain * sample;
	}
}

bool de_sound_source_can_produce_samples(de_sound_source_t* src) {
	return src->buffer && (src->status == DE_SOUND_SOURCE_STATUS_PLAYING) &&
		(src->left_gain > 0.0005f || src->right_gain > 0.0005f);
}

void de_sound_source_play(de_sound_source_t* src) {
	DE_ASSERT(src);
	if (src->status != DE_SOUND_SOURCE_STATUS_PLAYING) {
		de_sound_context_lock(src->ctx);
		src->status = DE_SOUND_SOURCE_STATUS_PLAYING;
		de_sound_context_unlock(src->ctx);
	}
}

void de_sound_source_stop(de_sound_source_t* src) {
	DE_ASSERT(src);
	de_sound_context_lock(src->ctx);
	src->status = DE_SOUND_SOURCE_STATUS_STOPPED;
	/* rewind */
	src->buf_read_pos = 0;
	de_sound_buffer_rewind(src->buffer);
	de_sound_context_unlock(src->ctx);
}

void de_sound_source_pause(de_sound_source_t* src) {
	DE_ASSERT(src);
	de_sound_context_lock(src->ctx);
	src->status = DE_SOUND_SOURCE_STATUS_PAUSED;
	de_sound_context_unlock(src->ctx);
}

void de_sound_source_set_type(de_sound_source_t* src, de_sound_source_type_t type) {
	DE_ASSERT(src);
	de_sound_context_lock(src->ctx);
	src->type = type;
	de_sound_context_unlock(src->ctx);
}

void de_sound_source_update(de_sound_source_t* src) {
	float dist_gain;
	de_listener_t* lst;
	de_vec3_t dir;
	float sqr_distance;

	DE_ASSERT(src);
	dist_gain = 1.0f;
	lst = &src->ctx->listener;
	if (src->type == DE_SOUND_SOURCE_TYPE_3D) {
		de_vec3_sub(&dir, &src->position, &lst->position);
		sqr_distance = de_vec3_sqr_len(&dir);
		if (sqr_distance < 0.0001f) {
			src->pan = 0;
		} else {
			de_vec3_normalize(&dir, &dir);
			src->pan = de_vec3_dot(&dir, &lst->ear_axis);
		}
		dist_gain = 1.0f / (1.0f + (sqr_distance / (src->radius * src->radius)));		
	}
	src->left_gain = dist_gain * (1.0f + src->pan);
	src->right_gain = dist_gain * (1.0f - src->pan);
}
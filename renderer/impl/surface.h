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

#define DE_MAKE_VERTEX( surf, cx, cy, cz, tx, ty )	\
	v = surf->AddVertex();						\
	v->position.x = cx;							\
	v->position.y = cy;							\
	v->position.z = cz;							\
	v->texCoord.x = tx;							\
	v->texCoord.y = ty;							\

/*=======================================================================================*/
void de_surface_load_data(de_surface_t* surf, de_vertex_t* vertices, size_t vertex_count, int* indices, size_t index_count)
{
	size_t i;

	DE_ARRAY_FREE(surf->indices);
	DE_ARRAY_FREE(surf->vertices);

	for (i = 0; i < index_count; ++i)
	{
		DE_ARRAY_APPEND(surf->indices, indices[i]);
	}

	for (i = 0; i < vertex_count; ++i)
	{
		DE_ARRAY_APPEND(surf->vertices, vertices[i]);
	}

	surf->need_upload = true;
}

/*=======================================================================================*/
void de_surface_upload(de_surface_t* surf)
{
	surf->need_upload = true;
}

/*=======================================================================================*/
void de_surface_set_diffuse_texture(de_surface_t * surf, de_texture_t *tex)
{
	if (!surf || !tex)
	{
		return;
	}

	if (surf->diffuse_map)
	{
		de_texture_release(surf->diffuse_map);
	}

	de_texture_add_ref(tex);

	surf->diffuse_map = tex;
}

/*=======================================================================================*/
void de_surface_set_normal_texture(de_surface_t * surf, de_texture_t *tex)
{
	if (!surf || !tex)
	{
		return;
	}

	if (surf->normal_map)
	{
		de_texture_release(surf->normal_map);
	}

	de_texture_add_ref(tex);

	surf->normal_map = tex;
}

/*=======================================================================================*/
void de_surface_calculate_normals(de_surface_t * surf)
{
	size_t m;

	for (m = 0; m < surf->indices.size; m += 3)
	{
		int ia, ib, ic;
		de_vertex_t *a, *b, *c;
		de_vec3_t ab, ac;
		de_vec3_t normal;

		ia = surf->indices.data[m];
		ib = surf->indices.data[m + 1];
		ic = surf->indices.data[m + 2];

		a = surf->vertices.data + ia;
		b = surf->vertices.data + ib;
		c = surf->vertices.data + ic;

		de_vec3_sub(&ab, &b->position, &a->position);
		de_vec3_sub(&ac, &c->position, &a->position);

		de_vec3_cross(&normal, &ab, &ac);
		de_vec3_normalize(&normal, &normal);

		a->normal = normal;
		b->normal = normal;
		c->normal = normal;
	}
}

/*=======================================================================================*/
bool de_surface_prepare_vertices_for_skinning(de_surface_t* surf)
{
	size_t i, k;

	/* ensure that surface can be skinned */
	if (surf->vertex_weights.size != surf->vertices.size)
	{
		return false;
	}

	/* problem: gpu knows only index of matrix for each vertex that it should apply to vertex 
	 * to do skinning, but on engine side we want to be able to use scene nodes as bones,
	 * so here we just calculating index of affecting bone of a vertex, so index will point 
	 * to matrix that gpu will use.
	 * */

	/* map bone nodes to bone indices */
	for (i = 0; i < surf->vertices.size; ++i)
	{
		de_vertex_t* v = surf->vertices.data + i;
		de_vertex_weight_group_t* weight_group = surf->vertex_weights.data + i;

		for (k = 0; k < weight_group->weight_count; ++k)
		{
			de_vertex_weight_t* vertex_weight = weight_group->bones + k;

			v->bone_weights[k] = vertex_weight->weight;
			v->bone_indices[k] = (uint8_t)de_surface_get_bone_index(surf, (de_node_t*)vertex_weight->node);
		}
	}

	return true;
}

/*=======================================================================================*/
bool de_surface_add_bone(de_surface_t* surf, de_node_t* bone)
{
	size_t i;

	for (i = 0; i < surf->weights.size; ++i)
	{
		if (surf->weights.data[i] == bone)
		{
			return false;
		}
	}

	DE_ARRAY_APPEND(surf->weights, bone);

	return true;
}

/*=======================================================================================*/
int de_surface_get_bone_index(de_surface_t* surf, de_node_t* bone)
{
	size_t i;

	for (i = 0; i < surf->weights.size; ++i)
	{
		if (surf->weights.data[i] == bone)
		{
			return i;
		}
	}

	de_log("error: no such bone %s found in surface's bones!", bone->name);

	return -1;
}

/*=======================================================================================*/
void de_surface_get_skinning_matrices(de_surface_t* surf, de_mat4_t* out_matrices, size_t max_matrices)
{
	size_t i;

	for (i = 0; i < surf->weights.size && i < max_matrices; ++i)
	{
		de_node_t* bone_node = surf->weights.data[ i];
		de_mat4_t* m = out_matrices + i;		
		de_mat4_mul(m, &bone_node->global_matrix, &bone_node->inv_bind_pose_matrix);		
	}
}

/*=======================================================================================*/
bool de_surface_is_skinned(de_surface_t* surf)
{
	return surf->vertex_weights.size > 0;
}

/*=======================================================================================*/
void de_surface_calculate_tangents(de_surface_t* surf)
{
	size_t i;
	de_vec3_t *tan1 = (de_vec3_t *)  de_calloc(surf->vertices.size * 2, sizeof(de_vec3_t));
	de_vec3_t *tan2 = tan1 + surf->vertices.size;

	for (i = 0; i < surf->indices.size; i += 3)
	{
		de_vec3_t sdir, tdir;

		int i1 = surf->indices.data[i + 0];
		int i2 = surf->indices.data[i + 1];
		int i3 = surf->indices.data[i + 2];

		const de_vec3_t* v1 = &surf->vertices.data[i1].position;
		const de_vec3_t* v2 = &surf->vertices.data[i2].position;
		const de_vec3_t* v3 = &surf->vertices.data[i3].position;

		const de_vec2_t* w1 = &surf->vertices.data[i1].tex_coord;
		const de_vec2_t* w2 = &surf->vertices.data[i2].tex_coord;
		const de_vec2_t* w3 = &surf->vertices.data[i3].tex_coord;

		float x1 = v2->x - v1->x;
		float x2 = v3->x - v1->x;
		float y1 = v2->y - v1->y;
		float y2 = v3->y - v1->y;
		float z1 = v2->z - v1->z;
		float z2 = v3->z - v1->z;

		float s1 = w2->x - w1->x;
		float s2 = w3->x - w1->x;
		float t1 = w2->y - w1->y;
		float t2 = w3->y - w1->y;

		float r = 1.0F / (s1 * t2 - s2 * t1);

		sdir.x = (t2 * x1 - t1 * x2) * r;
		sdir.y = (t2 * y1 - t1 * y2) * r;
		sdir.z = (t2 * z1 - t1 * z2) * r;

		tdir.x = (s1 * x2 - s2 * x1) * r;
		tdir.y = (s1 * y2 - s2 * y1) * r;
		tdir.z = (s1 * z2 - s2 * z1) * r;

		tan1[i1].x += sdir.x;
		tan1[i1].y += sdir.y;
		tan1[i1].z += sdir.z;

		tan1[i2].x += sdir.x;
		tan1[i2].y += sdir.y;
		tan1[i2].z += sdir.z;

		tan1[i3].x += sdir.x;
		tan1[i3].y += sdir.y;
		tan1[i3].z += sdir.z;

		tan2[i1].x += tdir.x;
		tan2[i1].y += tdir.y;
		tan2[i1].z += tdir.z;

		tan2[i2].x += tdir.x;
		tan2[i2].y += tdir.y;
		tan2[i2].z += tdir.z;

		tan2[i3].x += tdir.x;
		tan2[i3].y += tdir.y;
		tan2[i3].z += tdir.z;
	}

	for (i = 0; i < surf->vertices.size; ++i)
	{
		de_vertex_t* v = surf->vertices.data + i;
		const de_vec3_t* n = &v->normal;
		const de_vec3_t* t = tan1 + i;
		de_vec3_t tangent, temp;

		/* Gram-Schmidt orthogonalize */
		de_vec3_scale(&temp, n, de_vec3_dot(n, t));
		de_vec3_sub(&tangent, t, &temp);
		de_vec3_normalize(&tangent, &tangent);
	
		v->tangent.x = tangent.x;
		v->tangent.y = tangent.y;
		v->tangent.z = tangent.z;
		
		/* Calculate handedness */
		de_vec3_cross(&temp, n, t);
		v->tangent.w = (de_vec3_dot(&temp, &tan2[i]) < 0.0F) ? -1.0F : 1.0F;
	}

	de_free(tan1);
}

/*=======================================================================================*/
void de_surface_make_cube(de_surface_t* surf)
{
	static de_vertex_t vertices[] = {
		/* front */
		{ { -0.5, -0.5, 0.5 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { -0.5, 0.5, 0.5 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5, 0.5, 0.5 }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5, -0.5, 0.5 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },

		/* back */
		{ { -0.5, -0.5, -0.5 }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { -0.5, 0.5, -0.5 }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5, 0.5, -0.5 }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5, -0.5, -0.5 }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },

		/* left */
		{ { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },

		/* right */
		{ { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },

		/* top */
		{ { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },

		/* bottom */
		{ { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } },
		{ { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0 }, { 0 } }
	};

	static int faces[] = {
		2, 1, 0,
		3, 2, 0,

		4, 5, 6,
		4, 6, 7,

		10, 9, 8,
		11, 10, 8,

		12, 13, 14,
		12, 14, 15,

		18, 17, 16,
		19, 18, 16,

		20, 21, 22,
		20, 22, 23
	};

	de_surface_load_data(surf, vertices, DE_ARRAY_SIZE(vertices), faces, DE_ARRAY_SIZE(faces));
	de_surface_calculate_normals(surf);
	de_surface_calculate_tangents(surf);
}

/*=======================================================================================*/
void de_surface_make_sphere(de_surface_t* surf, int slices, int stacks, float r)
{
#if 0
	int i, j;

	float d_theta = M_PI / slices;
	float d_phi = 2.0f * M_PI / stacks;
	float d_tc_y = 1.0f / stacks;
	float d_tc_x = 1.0f / slices;
	de_vertex_t* v;

	for (i = 0; i < stacks; ++i)
	{
		for (j = 0; j < slices; ++j)
		{
			int nj = j + 1;
			int ni = i + 1;

			float k0 = r * sin(d_theta * i);
			float k1 = cos(d_phi * j);
			float k2 = sin(d_phi * j);
			float k3 = r * cos(d_theta * i);

			float k4 = r * sin(d_theta * ni);
			float k5 = cos(d_phi * nj);
			float k6 = sin(d_phi * nj);
			float k7 = r * cos(d_theta * ni);

			if (i != (stacks - 1))
			{
				DE_MAKE_VERTEX(surf, k0 * k1, k0 * k2, k3, d_tc_x * j, d_tc_y * i);
				DE_MAKE_VERTEX(surf, k4 * k1, k4 * k2, k7, d_tc_x * j, d_tc_y * ni);
				DE_MAKE_VERTEX(surf, k4 * k5, k4 * k6, k7, d_tc_x * nj, d_tc_y * ni);
			}

			if (i != 0)
			{
				DE_MAKE_VERTEX(surf, k4 * k5, k4 * k6, k7, d_tc_x * nj, d_tc_y * ni);
				DE_MAKE_VERTEX(surf, k0 * k5, k0 * k6, k3, d_tc_x * nj, d_tc_y * i);
				DE_MAKE_VERTEX(surf, k0 * k1, k0 * k2, k3, d_tc_x * j, d_tc_y * i);
			}
		}
	}

	de_surface_calculate_normals(surf);
	de_surface_calculate_tangents(surf);
#endif 
	DE_UNUSED(surf);
	DE_UNUSED(slices);

	DE_UNUSED(stacks);
	DE_UNUSED(r);
}

#undef DE_MAKE_VERTEX
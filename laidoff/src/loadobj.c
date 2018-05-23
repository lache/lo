#include <math.h>
#include <sys/stat.h>
#include <float.h>

#include "lwgl.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

typedef struct {
	GLuint vb;
	int numTriangles;
} DrawObject;

DrawObject gDrawObject;

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
	float v10[3];
	float v20[3];
	float len2;

	v10[0] = v1[0] - v0[0];
	v10[1] = v1[1] - v0[1];
	v10[2] = v1[2] - v0[2];

	v20[0] = v2[0] - v0[0];
	v20[1] = v2[1] - v0[1];
	v20[2] = v2[2] - v0[2];

	N[0] = v20[1] * v10[2] - v20[2] * v10[1];
	N[1] = v20[2] * v10[0] - v20[0] * v10[2];
	N[2] = v20[0] * v10[1] - v20[1] * v10[0];

	len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
	if (len2 > 0.0f) {
		float len = (float)sqrt((double)len2);

		N[0] /= len;
		N[1] /= len;
	}
}

static const char* mmap_file(size_t* len, const char* filename)
{
	FILE* f;
	long file_size;
	char* p;

	(*len) = 0;

	f = fopen(filename, "rb");
	fseek(f, 0, SEEK_END);
	file_size = ftell(f);
	
	(*len) = file_size;
	p = (char*)malloc(file_size);
	fseek(f, 0, SEEK_SET);
	fread(p, file_size, 1, f);

	fclose(f);

	return p;
}

static const char* get_file_data(size_t* len, const char* filename) {
	const char* ext = strrchr(filename, '.');

	size_t data_len = 0;
	const char* data = NULL;

	if (strcmp(ext, ".gz") == 0) {
		assert(0); /* todo */

	}
	else {
		data = mmap_file(&data_len, filename);
	}

	(*len) = data_len;
	return data;
}

int LoadObjAndConvert(float bmin[3], float bmax[3], const char* filename) {
	tinyobj_attrib_t attrib;
	tinyobj_shape_t* shapes = NULL;
	size_t num_shapes;
	tinyobj_material_t* materials = NULL;
	size_t num_materials;

	size_t data_len = 0;
	const char* data = get_file_data(&data_len, filename);
	if (data == NULL) {
		exit(-1);
		/* return 0; */
	}
	printf("filesize: %d\n", (int)data_len);

	{
		unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
		int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
			&num_materials, data, data_len, flags);
		if (ret != TINYOBJ_SUCCESS) {
			return 0;
		}

		printf("# of shapes    = %d\n", (int)num_shapes);
		printf("# of materiasl = %d\n", (int)num_materials);

		/*
		{
		int i;
		for (i = 0; i < num_shapes; i++) {
		printf("shape[%d] name = %s\n", i, shapes[i].name);
		}
		}
		*/
	}

	bmin[0] = bmin[1] = bmin[2] = FLT_MAX;
	bmax[0] = bmax[1] = bmax[2] = -FLT_MAX;

	{
		DrawObject o;
		float* vb;
		/* std::vector<float> vb; //  */
		size_t face_offset = 0;
		size_t i;

		/* Assume triangulated face. */
		size_t num_triangles = attrib.num_face_num_verts;
		size_t stride = 8; /* 9 = pos(3float), normal(3float), color(3float) */

		vb = (float*)malloc(sizeof(float) * stride * num_triangles * 3);

		assert(attrib.num_vertices == attrib.num_texcoords);

		for (i = 0; i < attrib.num_face_num_verts; i++) {
			size_t f;
			assert(attrib.face_num_verts[i] % 3 ==
				0); /* assume all triangle faces. */
			for (f = 0; f < (size_t)attrib.face_num_verts[i] / 3; f++) {
				size_t k;
				float v[3][3];
				float n[3][3];
				float c[3];
				float u[3][2];
				float len2;

				tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 3 * f + 0];
				tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 3 * f + 1];
				tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 3 * f + 2];

				for (k = 0; k < 3; k++) { // k for x, y, z axis
					int f0 = idx0.v_idx;
					int f1 = idx1.v_idx;
					int f2 = idx2.v_idx;
					assert(f0 >= 0);
					assert(f1 >= 0);
					assert(f2 >= 0);

					v[0][k] = attrib.vertices[3 * (size_t)f0 + k];
					v[1][k] = attrib.vertices[3 * (size_t)f1 + k];
					v[2][k] = attrib.vertices[3 * (size_t)f2 + k];

					if (k < 2)
					{
						u[0][k] = attrib.texcoords[3 * (size_t)f0 + k];
						u[1][k] = attrib.texcoords[3 * (size_t)f1 + k];
						u[2][k] = attrib.texcoords[3 * (size_t)f2 + k];
					}

					bmin[k] = (v[0][k] < bmin[k]) ? v[0][k] : bmin[k];
					bmin[k] = (v[1][k] < bmin[k]) ? v[1][k] : bmin[k];
					bmin[k] = (v[2][k] < bmin[k]) ? v[2][k] : bmin[k];
					bmax[k] = (v[0][k] > bmax[k]) ? v[0][k] : bmax[k];
					bmax[k] = (v[1][k] > bmax[k]) ? v[1][k] : bmax[k];
					bmax[k] = (v[2][k] > bmax[k]) ? v[2][k] : bmax[k];
				}

				if (attrib.num_normals > 0) {
					int f0 = idx0.vn_idx;
					int f1 = idx1.vn_idx;
					int f2 = idx2.vn_idx;
					if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
						assert(f0 < (int)attrib.num_normals);
						assert(f1 < (int)attrib.num_normals);
						assert(f2 < (int)attrib.num_normals);
						for (k = 0; k < 3; k++) {
							n[0][k] = attrib.normals[3 * (size_t)f0 + k];
							n[1][k] = attrib.normals[3 * (size_t)f1 + k];
							n[2][k] = attrib.normals[3 * (size_t)f2 + k];
						}
					}
					else { /* normal index is not defined for this face */
						   /* compute geometric normal */
						CalcNormal(n[0], v[0], v[1], v[2]);
						n[1][0] = n[0][0];
						n[1][1] = n[0][1];
						n[1][2] = n[0][2];
						n[2][0] = n[0][0];
						n[2][1] = n[0][1];
						n[2][2] = n[0][2];
					}
				}
				else {
					/* compute geometric normal */
					CalcNormal(n[0], v[0], v[1], v[2]);
					n[1][0] = n[0][0];
					n[1][1] = n[0][1];
					n[1][2] = n[0][2];
					n[2][0] = n[0][0];
					n[2][1] = n[0][1];
					n[2][2] = n[0][2];
				}

				for (k = 0; k < 3; k++) {
					// position
					vb[(3 * i + k) * stride + 0] = v[k][0];
					vb[(3 * i + k) * stride + 1] = v[k][1];
					vb[(3 * i + k) * stride + 2] = v[k][2];

					/* Use normal as color. */
					c[0] = n[k][0];
					c[1] = n[k][1];
					c[2] = n[k][2];
					len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
					if (len2 > 0.0f) {
						float len = sqrtf(len2);

						c[0] /= len;
						c[1] /= len;
						c[2] /= len;
					}

					// color
					vb[(3 * i + k) * stride + 3] = (c[0] * 0.5f + 0.5f);
					vb[(3 * i + k) * stride + 4] = (c[1] * 0.5f + 0.5f);
					vb[(3 * i + k) * stride + 5] = (c[2] * 0.5f + 0.5f);

					// uv
					vb[(3 * i + k) * stride + 6] = u[k][0];
					vb[(3 * i + k) * stride + 7] = u[k][1];
				}
			}
			
			face_offset += (size_t)attrib.face_num_verts[i];
		}

		o.vb = 0;
		o.numTriangles = 0;
		if (num_triangles > 0) {
			glGenBuffers(1, &o.vb);
			glBindBuffer(GL_ARRAY_BUFFER, o.vb);
			glBufferData(GL_ARRAY_BUFFER, num_triangles * 3 * stride * sizeof(float),
				vb, GL_STATIC_DRAW);
			o.numTriangles = (int)num_triangles;
		}

		free(vb);

		gDrawObject = o;
	}

	printf("bmin = %f, %f, %f\n", (double)bmin[0], (double)bmin[1],
		(double)bmin[2]);
	printf("bmax = %f, %f, %f\n", (double)bmax[0], (double)bmax[1],
		(double)bmax[2]);

	tinyobj_attrib_free(&attrib);
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);

	return 1;
}

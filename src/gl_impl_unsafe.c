

#include <stdarg.h>


/******************************************
 * PORTABLEGL_IMPLEMENTATION
 ******************************************/

#include <stdio.h>
#include <assert.h>
#include <float.h>

// for CHAR_BIT
#include <limits.h>



#ifdef DEBUG
#define IS_VALID(target, error, ...) is_valid(target, error, __VA_ARGS__)
#else
#define IS_VALID(target, error, ...) 1
#endif

int is_valid(GLenum target, GLenum error, int n, ...)
{
	va_list argptr;

	va_start(argptr, n);
	for (int i=0; i<n; ++i) {
		if (target == va_arg(argptr, GLenum)) {
			return 1;
		}
	}
	va_end(argptr);

	if (!c->error) {
		c->error = error;
	}
	return 0;
}








// default pass through shaders for index 0
void default_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}

void default_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec4* fragcolor = &builtins->gl_FragColor;
	//wish I could use a compound literal, stupid C++ compatibility
	fragcolor->x = 1.0f;
	fragcolor->y = 0.0f;
	fragcolor->z = 0.0f;
	fragcolor->w = 1.0f;
}


void init_glVertex_Array(glVertex_Array* v)
{
	v->deleted = GL_FALSE;
	for (int i=0; i<GL_MAX_VERTEX_ATTRIBS; ++i)
		init_glVertex_Attrib(&v->vertex_attribs[i]);
}

void init_glVertex_Attrib(glVertex_Attrib* v)
{
	/*
	GLint size;      // number of components 1-4
	GLenum type;     // GL_FLOAT, default
	GLsizei stride;  //
	GLsizei offset;  //
	GLboolean normalized;
	unsigned int buf;
	GLboolean enabled;
	GLuint divisor;
*/
	v->buf = 0;
	v->enabled = 0;
	v->divisor = 0;
}


#define GET_SHIFT(mask, shift) \
	do {\
	shift = 0;\
	while ((mask & 1) == 0) {\
		mask >>= 1;\
		++shift;\
	}\
	} while (0)


int init_glContext(glContext* context, u32** back, int w, int h, int bitdepth, u32 Rmask, u32 Gmask, u32 Bmask, u32 Amask)
{
	if (bitdepth > 32 || !back)
		return 0;

	context->user_alloced_backbuf = *back != NULL;
	if (!*back) {
		int bytes_per_pixel = (bitdepth + CHAR_BIT-1) / CHAR_BIT;
		*back = (u32*)malloc(w * h * bytes_per_pixel);
		if (!*back)
			return 0;
	}

	context->zbuf.buf = (u8*)malloc(w*h * sizeof(float));
	if (!context->zbuf.buf) {
		if (!context->user_alloced_backbuf) {
			free(*back);
			*back = NULL;
		}
		return 0;
	}

	context->stencil_buf.buf = (u8*)malloc(w*h);
	if (!context->stencil_buf.buf) {
		if (!context->user_alloced_backbuf) {
			free(*back);
			*back = NULL;
		}
		free(context->zbuf.buf);
		return 0;
	}

	context->x_min = 0;
	context->y_min = 0;
	context->x_max = w;
	context->y_max = h;

	context->zbuf.w = w;
	context->zbuf.h = h;
	context->zbuf.lastrow = context->zbuf.buf + (h-1)*w*sizeof(float);

	context->stencil_buf.w = w;
	context->stencil_buf.h = h;
	context->stencil_buf.lastrow = context->stencil_buf.buf + (h-1)*w;

	context->back_buffer.w = w;
	context->back_buffer.h = h;
	context->back_buffer.buf = (u8*) *back;
	context->back_buffer.lastrow = context->back_buffer.buf + (h-1)*w*sizeof(u32);

	context->bitdepth = bitdepth; //not used yet
	context->Rmask = Rmask;
	context->Gmask = Gmask;
	context->Bmask = Bmask;
	context->Amask = Amask;
	GET_SHIFT(Rmask, context->Rshift);
	GET_SHIFT(Gmask, context->Gshift);
	GET_SHIFT(Bmask, context->Bshift);
	GET_SHIFT(Amask, context->Ashift);

	//initialize all vectors
	cvec_glVertex_Array(&context->vertex_arrays, 0, 3);
	cvec_glBuffer(&context->buffers, 0, 3);
	cvec_glProgram(&context->programs, 0, 3);
	cvec_glTexture(&context->textures, 0, 1);
	cvec_glVertex(&context->glverts, 0, 10);

	//TODO might as well just set it to MAX_VERTICES * MAX_OUTPUT_COMPONENTS
	cvec_float(&context->vs_output.output_buf, 0, 0);


	context->clear_stencil = 0;
	context->clear_color = make_Color(0, 0, 0, 0);
	SET_VEC4(context->blend_color, 0, 0, 0, 0);
	context->point_size = 1.0f;
	context->clear_depth = 1.0f;
	context->depth_range_near = 0.0f;
	context->depth_range_far = 1.0f;
	make_viewport_matrix(context->vp_mat, 0, 0, w, h, 1);


	//set flags
	//TODO match order in structure definition
	context->provoking_vert = GL_LAST_VERTEX_CONVENTION;
	context->cull_mode = GL_BACK;
	context->cull_face = GL_FALSE;
	context->front_face = GL_CCW;
	context->depth_test = GL_FALSE;
	context->fragdepth_or_discard = GL_FALSE;
	context->depth_clamp = GL_FALSE;
	context->depth_mask = GL_TRUE;
	context->blend = GL_FALSE;
	context->logic_ops = GL_FALSE;
	context->poly_offset = GL_FALSE;
	context->scissor_test = GL_FALSE;

	context->stencil_test = GL_FALSE;
	context->stencil_writemask = -1; // all 1s for the masks
	context->stencil_writemask_back = -1;
	context->stencil_ref = 0;
	context->stencil_ref_back = 0;
	context->stencil_valuemask = -1;
	context->stencil_valuemask_back = -1;
	context->stencil_func = GL_ALWAYS;
	context->stencil_func_back = GL_ALWAYS;
	context->stencil_sfail = GL_KEEP;
	context->stencil_dpfail = GL_KEEP;
	context->stencil_dppass = GL_KEEP;
	context->stencil_sfail_back = GL_KEEP;
	context->stencil_dpfail_back = GL_KEEP;
	context->stencil_dppass_back = GL_KEEP;

	context->logic_func = GL_COPY;
	context->blend_sfactor = GL_ONE;
	context->blend_dfactor = GL_ZERO;
	context->blend_equation = GL_FUNC_ADD;
	context->depth_func = GL_LESS;
	context->line_smooth = GL_FALSE;
	context->poly_mode_front = GL_FILL;
	context->poly_mode_back = GL_FILL;
	context->point_spr_origin = GL_UPPER_LEFT;

	context->poly_factor = 0.0f;
	context->poly_units = 0.0f;

	context->scissor_lx = 0;
	context->scissor_ly = 0;
	context->scissor_ux = w;
	context->scissor_uy = h;

	// According to refpages https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glPixelStore.xhtml
	context->unpack_alignment = 4;
	context->pack_alignment = 4;

	context->draw_triangle_front = draw_triangle_fill;
	context->draw_triangle_back = draw_triangle_fill;

	context->error = GL_NO_ERROR;

	//program 0 is supposed to be undefined but not invalid so I'll
	//just make it default
	glProgram tmp_prog = { default_vs, default_fs, NULL, GL_FALSE };
	cvec_push_glProgram(&context->programs, tmp_prog);
	context->cur_program = 0;

	//setup default vertex_array (vao) at position 0
	//we're like a compatibility profile for this but come on
	//no reason not to have this imo
	//https://www.opengl.org/wiki/Vertex_Specification#Vertex_Array_Object
	glVertex_Array tmp_va;
	init_glVertex_Array(&tmp_va);
	cvec_push_glVertex_Array(&context->vertex_arrays, tmp_va);
	context->cur_vertex_array = 0;

	//setup buffers and textures
	//need to push back once since 0 is invalid
	//valid buffers/textures have to start at position 1
	glBuffer tmp_buf;
	tmp_buf.user_owned = GL_TRUE;
	tmp_buf.deleted = GL_FALSE;

	glTexture tmp_tex;
	tmp_tex.user_owned = GL_TRUE;
	tmp_tex.deleted = GL_FALSE;
	tmp_tex.format = GL_RGBA;
	tmp_tex.type = GL_TEXTURE_UNBOUND;
	tmp_tex.data = NULL;
	tmp_tex.w = 0;
	tmp_tex.h = 0;
	tmp_tex.d = 0;
	cvec_push_glBuffer(&context->buffers, tmp_buf);
	cvec_push_glTexture(&context->textures, tmp_tex);

	return 1;
}

void free_glContext(glContext* context)
{
	int i;

	free(context->zbuf.buf);
	free(context->stencil_buf.buf);
	if (!context->user_alloced_backbuf) {
		free(context->back_buffer.buf);
	}


	for (i=0; i<context->buffers.size; ++i) {
		if (!context->buffers.a[i].user_owned) {
			printf("freeing buffer %d\n", i);
			free(context->buffers.a[i].data);
		}
	}

	for (i=0; i<context->textures.size; ++i) {
		if (!context->textures.a[i].user_owned) {
			printf("freeing texture %d\n", i);
			free(context->textures.a[i].data);
		}
	}

	//free vectors
	cvec_free_glVertex_Array(&context->vertex_arrays);
	cvec_free_glBuffer(&context->buffers);
	cvec_free_glProgram(&context->programs);
	cvec_free_glTexture(&context->textures);
	cvec_free_glVertex(&context->glverts);

	cvec_free_float(&context->vs_output.output_buf);
}

void set_glContext(glContext* context)
{
	c = context;
}

void* pglResizeFramebuffer(size_t w, size_t h)
{
	u8* tmp;
	tmp = (u8*)realloc(c->zbuf.buf, w*h * sizeof(float));

	c->zbuf.buf = tmp;
	c->zbuf.w = w;
	c->zbuf.h = h;
	c->zbuf.lastrow = c->zbuf.buf + (h-1)*w*sizeof(float);

	tmp = (u8*)realloc(c->back_buffer.buf, w*h * sizeof(u32));

	c->back_buffer.buf = tmp;
	c->back_buffer.w = w;
	c->back_buffer.h = h;
	c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(u32);

	return tmp;
}



GLubyte* glGetString(GLenum name)
{
	static GLubyte vendor[] = "Robert Winkler";
	static GLubyte renderer[] = "PortableGL";
	static GLubyte version[] = "OpenGL 3.x-ish PortableGL 0.96";
	static GLubyte shading_language[] = "C/C++";

	switch (name) {
	case GL_VENDOR:                   return vendor;
	case GL_RENDERER:                 return renderer;
	case GL_VERSION:                  return version;
	case GL_SHADING_LANGUAGE_VERSION: return shading_language;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return 0;
	}
}

GLenum glGetError()
{
	GLenum err = c->error;
	c->error = GL_NO_ERROR;
	return err;
}

void glGenVertexArrays(GLsizei n, GLuint* arrays)
{
	glVertex_Array tmp;
	init_glVertex_Array(&tmp);

	tmp.deleted = GL_FALSE;

	//fill up empty slots first
	--n;
	for (int i=1; i<c->vertex_arrays.size && n>=0; ++i) {
		if (c->vertex_arrays.a[i].deleted) {
			c->vertex_arrays.a[i] = tmp;
			arrays[n--] = i;
		}
	}

	for (; n>=0; --n) {
		cvec_push_glVertex_Array(&c->vertex_arrays, tmp);
		arrays[n] = c->vertex_arrays.size-1;
	}
}

void glDeleteVertexArrays(GLsizei n, const GLuint* arrays)
{
	for (int i=0; i<n; ++i) {
		if (!arrays[i] || arrays[i] >= c->vertex_arrays.size)
			continue;

		if (arrays[i] == c->cur_vertex_array) {
			//TODO check if memcpy isn't enough
			memcpy(&c->vertex_arrays.a[0], &c->vertex_arrays.a[arrays[i]], sizeof(glVertex_Array));
			c->cur_vertex_array = 0;
		}

		c->vertex_arrays.a[arrays[i]].deleted = GL_TRUE;
	}
}

void glGenBuffers(GLsizei n, GLuint* buffers)
{
	//fill up empty slots first
	int j = 0;
	for (int i=1; i<c->buffers.size && j<n; ++i) {
		if (c->buffers.a[i].deleted) {
			c->buffers.a[i].deleted = GL_FALSE;
			buffers[j++] = i;
		}
	}

	if (j != n) {
		int s = c->buffers.size;
		cvec_extend_glBuffer(&c->buffers, n-j);
		for (int i=s; j<n; i++) {
			c->buffers.a[i].data = NULL;
			c->buffers.a[i].deleted = GL_FALSE;
			buffers[j++] = i;
		}
	}
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!buffers[i] || buffers[i] >= c->buffers.size)
			continue;

		// NOTE(rswinkle): type is stored as correct index not the raw enum value so no need to
		// subtract here see glBindBuffer
		type = c->buffers.a[buffers[i]].type;
		if (buffers[i] == c->bound_buffers[type])
			c->bound_buffers[type] = 0;

		if (!c->buffers.a[buffers[i]].user_owned) {
			free(c->buffers.a[buffers[i]].data);
		}
		c->buffers.a[buffers[i]].data = NULL;
		c->buffers.a[buffers[i]].deleted = GL_TRUE;
	}
}

void glGenTextures(GLsizei n, GLuint* textures)
{
	glTexture tmp;
	//SET_VEC4(tmp.border_color, 0, 0, 0, 0);
	tmp.mag_filter = GL_LINEAR;
	tmp.min_filter = GL_LINEAR; //NOTE: spec says should be mipmap_linear
	tmp.wrap_s = GL_REPEAT;
	tmp.wrap_t = GL_REPEAT;
	tmp.data = NULL;
	tmp.deleted = GL_FALSE;
	tmp.user_owned = GL_TRUE;  // NOTE: could be either before data
	tmp.format = GL_RGBA;
	tmp.type = GL_TEXTURE_UNBOUND;
	tmp.w = 0;
	tmp.h = 0;
	tmp.d = 0;

	--n;
	for (int i=0; i<c->textures.size && n>=0; ++i) {
		if (c->textures.a[i].deleted) {
			c->textures.a[i] = tmp;
			textures[n--] = i;
		}
	}
	for (; n>=0; --n) {
		cvec_push_glTexture(&c->textures, tmp);
		textures[n] = c->textures.size-1;
	}
}

void glDeleteTextures(GLsizei n, GLuint* textures)
{
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!textures[i] || textures[i] >= c->textures.size)
			continue;

		// NOTE(rswinkle): type is stored as correct index not the raw enum value so no need to
		// subtract here see glBindTexture
		type = c->textures.a[textures[i]].type;
		if (textures[i] == c->bound_textures[type])
			c->bound_textures[type] = 0;

		if (!c->textures.a[textures[i]].user_owned) {
			free(c->textures.a[textures[i]].data);
			c->textures.a[textures[i]].data = NULL;
		}

		c->textures.a[textures[i]].deleted = GL_TRUE;
	}
}

void glBindVertexArray(GLuint array)
{
	c->cur_vertex_array = array;
}

void glBindBuffer(GLenum target, GLuint buffer)
{
	target -= GL_ARRAY_BUFFER;

	c->bound_buffers[target] = buffer;

	// Note type isn't set till binding and we're not storing the raw
	// enum but the enum - GL_ARRAY_BUFFER so it's an index into c->bound_buffers
	// TODO need to see what's supposed to happen if you try to bind
	// a buffer to multiple targets
	c->buffers.a[buffer].type = target;
}

void glBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage)
{
	target -= GL_ARRAY_BUFFER;

	// the spec says any pre-existing data store is deleted there's no reason to
	// c->buffers.a[c->bound_buffers[target]].data is always NULL or valid
	c->buffers.a[c->bound_buffers[target]].data = (u8*)realloc(c->buffers.a[c->bound_buffers[target]].data, size);

	if (data) {
		memcpy(c->buffers.a[c->bound_buffers[target]].data, data, size);
	}

	c->buffers.a[c->bound_buffers[target]].user_owned = GL_FALSE;
	c->buffers.a[c->bound_buffers[target]].size = size;

	if (target == GL_ELEMENT_ARRAY_BUFFER - GL_ARRAY_BUFFER) {
		c->vertex_arrays.a[c->cur_vertex_array].element_buffer = c->bound_buffers[target];
	}
}

void glBufferSubData(GLenum target, GLsizei offset, GLsizei size, const GLvoid* data)
{
	target -= GL_ARRAY_BUFFER;

	memcpy(&c->buffers.a[c->bound_buffers[target]].data[offset], data, size);
}

void glNamedBufferData(GLuint buffer, GLsizei size, const GLvoid* data, GLenum usage)
{
	//always NULL or valid
	free(c->buffers.a[buffer].data);

	c->buffers.a[buffer].data = (u8*)malloc(size);

	if (data) {
		memcpy(c->buffers.a[buffer].data, data, size);
	}

	c->buffers.a[buffer].user_owned = GL_FALSE;
	c->buffers.a[buffer].size = size;

	if (c->buffers.a[buffer].type == GL_ELEMENT_ARRAY_BUFFER - GL_ARRAY_BUFFER) {
		c->vertex_arrays.a[c->cur_vertex_array].element_buffer = buffer;
	}
}

void glNamedBufferSubData(GLuint buffer, GLsizei offset, GLsizei size, const GLvoid* data)
{
	memcpy(&c->buffers.a[buffer].data[offset], data, size);
}

void glBindTexture(GLenum target, GLuint texture)
{
	target -= GL_TEXTURE_UNBOUND + 1;

	if (c->textures.a[texture].type == GL_TEXTURE_UNBOUND) {
		c->bound_textures[target] = texture;
		c->textures.a[texture].type = target;
	} else {
		c->bound_textures[target] = texture;
	}
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	//shift to range 0 - NUM_TEXTURES-1 to access bound_textures array
	target -= GL_TEXTURE_UNBOUND + 1;

	if (pname == GL_TEXTURE_MIN_FILTER) {

		//TODO mipmapping isn't actually supported, not sure it's worth trouble/perf hit
		//just adding the enums to make porting easier
		if (param == GL_NEAREST_MIPMAP_NEAREST || param == GL_NEAREST_MIPMAP_LINEAR)
			param = GL_NEAREST;
		if (param == GL_LINEAR_MIPMAP_NEAREST || param == GL_LINEAR_MIPMAP_LINEAR)
			param = GL_LINEAR;

		c->textures.a[c->bound_textures[target]].min_filter = param;

	} else if (pname == GL_TEXTURE_MAG_FILTER) {
		c->textures.a[c->bound_textures[target]].mag_filter = param;
	} else if (pname == GL_TEXTURE_WRAP_S) {
		c->textures.a[c->bound_textures[target]].wrap_s = param;
	} else if (pname == GL_TEXTURE_WRAP_T) {
		c->textures.a[c->bound_textures[target]].wrap_t = param;
	} else if (pname == GL_TEXTURE_WRAP_R) {
		c->textures.a[c->bound_textures[target]].wrap_r = param;
	}
}

void glPixelStorei(GLenum pname, GLint param)
{
	if (pname == GL_UNPACK_ALIGNMENT) {
		c->unpack_alignment = param;
	} else if (pname == GL_PACK_ALIGNMENT) {
		c->pack_alignment = param;
	}

}

void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;

	if (type != GL_UNSIGNED_BYTE) {

		return;
	}

	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;

	// NULL or valid
	free(c->textures.a[cur_tex].data);

	//TODO support other internal formats? components should be of internalformat not format
	c->textures.a[cur_tex].data = (u8*)malloc(width * components);

	u32* texdata = (u32*)c->textures.a[cur_tex].data;

	if (data)
		memcpy(&texdata[0], data, width*sizeof(u32));

	c->textures.a[cur_tex].user_owned = GL_FALSE;

	//TODO
	//assume for now always RGBA coming in and that's what I'm storing it as
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// TODO I don't actually support anything other than GL_RGBA for input or
	// internal format ... so I should probably make the others errors and
	// I'm not even checking internalFormat currently..
	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;

	int cur_tex;

	// TODO If I ever support type other than GL_UNSIGNED_BYTE (also using for both internalformat and format)
	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

		c->textures.a[cur_tex].w = width;
		c->textures.a[cur_tex].h = height;

		// either NULL or valid
		free(c->textures.a[cur_tex].data);

		//TODO support other internal formats? components should be of internalformat not format
		c->textures.a[cur_tex].data = (u8*)malloc(height * byte_width);

		if (data) {
			if (!padding_needed) {
				memcpy(c->textures.a[cur_tex].data, data, height*byte_width);
			} else {
				for (int i=0; i<height; ++i) {
					memcpy(&c->textures.a[cur_tex].data[i*byte_width], &((u8*)data)[i*padded_row_len], byte_width);
				}
			}
		}

		c->textures.a[cur_tex].user_owned = GL_FALSE;

	} else {  //CUBE_MAP
		cur_tex = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1];

		// If we're reusing a texture, and we haven't already loaded
		// one of the planes of the cubemap, data is either NULL or valid
		if (!c->textures.a[cur_tex].w)
			free(c->textures.a[cur_tex].data);

		int mem_size = width*height*6 * components;
		if (c->textures.a[cur_tex].w == 0) {
			c->textures.a[cur_tex].w = width;
			c->textures.a[cur_tex].h = width; //same cause square

			c->textures.a[cur_tex].data = (u8*)malloc(mem_size);
		}

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		// TODO handle different format and internalFormat
		int p = height*byte_width;
		u8* texdata = c->textures.a[cur_tex].data;

		if (data) {
			if (!padding_needed) {
				memcpy(&texdata[target*p], data, height*byte_width);
			} else {
				for (int i=0; i<height; ++i) {
					memcpy(&texdata[target*p + i*byte_width], &((u8*)data)[i*padded_row_len], byte_width);
				}
			}
		}

		c->textures.a[cur_tex].user_owned = GL_FALSE;

	} //end CUBE_MAP
}

void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;
	c->textures.a[cur_tex].h = height;
	c->textures.a[cur_tex].d = depth;

	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	// NULL or valid
	free(c->textures.a[cur_tex].data);

	//TODO support other internal formats? components should be of internalformat not format
	c->textures.a[cur_tex].data = (u8*)malloc(width*height*depth * components);

	u32* texdata = (u32*)c->textures.a[cur_tex].data;

	if (data) {
		if (!padding_needed) {
			memcpy(texdata, data, width*height*depth*sizeof(u32));
		} else {
			for (int i=0; i<height*depth; ++i) {
				memcpy(&texdata[i*byte_width], &((u8*)data)[i*padded_row_len], byte_width);
			}
		}
	}

	c->textures.a[cur_tex].user_owned = GL_FALSE;

	//TODO
	//assume for now always RGBA coming in and that's what I'm storing it as
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data)
{
	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	u32* texdata = (u32*) c->textures.a[cur_tex].data;
	memcpy(&texdata[xoffset], data, width*sizeof(u32));
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data)
{
	int cur_tex;
	u32* d = (u32*) data;

	if (target == GL_TEXTURE_2D) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];
		u32* texdata = (u32*) c->textures.a[cur_tex].data;

		int w = c->textures.a[cur_tex].w;

		for (int i=0; i<height; ++i) {
			memcpy(&texdata[(yoffset+i)*w + xoffset], &d[i*width], width*sizeof(u32));
		}

	} else {  //CUBE_MAP
		cur_tex = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1];
		u32* texdata = (u32*) c->textures.a[cur_tex].data;

		int w = c->textures.a[cur_tex].w;

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		int p = w*w;

		for (int i=0; i<height; ++i)
			memcpy(&texdata[p*target + (yoffset+i)*w + xoffset], &d[i*width], width*sizeof(u32));
	} //end CUBE_MAP
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data)
{
	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	int w = c->textures.a[cur_tex].w;
	int h = c->textures.a[cur_tex].h;
	int p = w*h;
	u32* d = (u32*) data;
	u32* texdata = (u32*) c->textures.a[cur_tex].data;

	for (int j=0; j<depth; ++j) {
		for (int i=0; i<height; ++i) {
			memcpy(&texdata[(zoffset+j)*p + (yoffset+i)*w + xoffset], &d[j*width*height + i*width], width*sizeof(u32));
		}
	}
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
{
	glVertex_Attrib* v = &(c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index]);
	v->size = size;
	v->type = type;

	//TODO expand for other types etc.
	v->stride = (stride) ? stride : size*sizeof(GLfloat);

	v->offset = (GLsizeiptr)pointer;
	v->normalized = normalized;
	// I put ARRAY_BUFFER-itself instead of 0 to reinforce that bound_buffers is indexed that way, buffer type - GL_ARRAY_BUFFER
	v->buf = c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER]; //can be 0 if offset is 0/NULL
}

void glEnableVertexAttribArray(GLuint index)
{
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_TRUE;
}

void glDisableVertexAttribArray(GLuint index)
{
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_FALSE;
}

void glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].divisor = divisor;
}


//TODO not used
vec4 get_vertex_attrib_array(glVertex_Attrib* v, GLsizei i)
{
	//this line need work for future flexibility and handling more than floats
	u8* buf_pos = (u8*)c->buffers.a[v->buf].data + v->offset + v->stride*i;

	vec4 tmpvec4;
	memcpy(&tmpvec4, buf_pos, sizeof(float)*v->size);

	//c->cur_vertex_array->vertex_attribs[enabled[j]].buf->data;
	return tmpvec4;
}


void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	if (!count)
		return;
	run_pipeline(mode, first, count, 0, 0, GL_FALSE);
}

void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount)
{
	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, first[i], count[i], 0, 0, GL_FALSE);
	}
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, GLsizei offset)
{
	if (!count)
		return;
	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;
	run_pipeline(mode, offset, count, 0, 0, GL_TRUE);
}

void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, GLsizei* indices, GLsizei drawcount)
{
	// TODO I assume this belongs here since I have it in DrawElements
	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;

	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, indices[i], count[i], 0, 0, GL_TRUE);
	}
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
	if (!count || !instancecount)
		return;
	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, first, count, instance, 0, GL_FALSE);
	}
}

void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
{
	if (!count || !instancecount)
		return;
	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, first, count, instance, baseinstance, GL_FALSE);
	}
}


void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei instancecount)
{
	if (!count || !instancecount)
		return;
	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, offset, count, instance, 0, GL_TRUE);
	}
}

void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei instancecount, GLuint baseinstance)
{
	if (!count || !instancecount)
		return;
	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, offset, count, instance, baseinstance, GL_TRUE);
	}
}


void glViewport(int x, int y, GLsizei width, GLsizei height)
{
	make_viewport_matrix(c->vp_mat, x, y, width, height, 1);
	c->x_min = x;
	c->y_min = y;
	c->x_max = x + width;
	c->y_max = y + height;
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	red = clampf_01(red);
	green = clampf_01(green);
	blue = clampf_01(blue);
	alpha = clampf_01(alpha);

	vec4 tmp = { red, green, blue, alpha };
	c->clear_color = vec4_to_Color(tmp);
}

void glClearDepth(GLclampf depth)
{
	c->clear_depth = clampf_01(depth);
}

void glDepthFunc(GLenum func)
{
	c->depth_func = func;
}

void glDepthRange(GLclampf nearVal, GLclampf farVal)
{
	c->depth_range_near = clampf_01(nearVal);
	c->depth_range_far = clampf_01(farVal);
}

void glDepthMask(GLboolean flag)
{
	c->depth_mask = flag;
}

void glClear(GLbitfield mask)
{
	// TODO since all the buffers should be the same width and height
	// (right? even though they're different types they should be 1 to 1),
	// why not just set local w and h and use for all instead of member w/h
	// for each framebuffer?

	// better to just set min/max x/y and use nested loops even when scissor is disabled?
	Color col = c->clear_color;
	if (mask & GL_COLOR_BUFFER_BIT) {
		if (!c->scissor_test) {
			for (int i=0; i<c->back_buffer.w*c->back_buffer.h; ++i) {
				((u32*)c->back_buffer.buf)[i] = (u32)col.a << c->Ashift | (u32)col.r << c->Rshift | (u32)col.g << c->Gshift | (u32)col.b << c->Bshift;
			}
		} else {
			for (int y=c->scissor_ly; y<c->scissor_uy; ++y) {
				for (int x=c->scissor_lx; x<c->scissor_ux; ++x) {
					((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x] = (u32)col.a << c->Ashift | (u32)col.r << c->Rshift | (u32)col.g << c->Gshift | (u32)col.b << c->Bshift;
				}
			}
		}
	}

	if (mask & GL_DEPTH_BUFFER_BIT) {
		if (!c->scissor_test) {
			//TODO try a big memcpy or other way to clear it
			for (int i=0; i < c->zbuf.w * c->zbuf.h; ++i) {
				((float*)c->zbuf.buf)[i] = c->clear_depth;
			}
		} else {
			for (int y=c->scissor_ly; y<c->scissor_uy; ++y) {
				for (int x=c->scissor_lx; x<c->scissor_ux; ++x) {
					((float*)c->zbuf.lastrow)[-y*c->zbuf.w + x] = c->clear_depth;
				}
			}
		}
	}

	if (mask & GL_STENCIL_BUFFER_BIT) {
		if (!c->scissor_test) {
			//TODO try a big memcpy or other way to clear it
			for (int i=0; i < c->stencil_buf.w * c->stencil_buf.h; ++i) {
				c->stencil_buf.buf[i] = c->clear_stencil;
			}
		} else {
			for (int y=c->scissor_ly; y<c->scissor_uy; ++y) {
				for (int x=c->scissor_lx; x<c->scissor_ux; ++x) {
					c->stencil_buf.lastrow[-y*c->stencil_buf.w + x] = c->clear_stencil;
				}
			}
		}
	}
}

void glEnable(GLenum cap)
{
	switch (cap) {
	case GL_CULL_FACE:
		c->cull_face = GL_TRUE;
		break;
	case GL_DEPTH_TEST:
		c->depth_test = GL_TRUE;
		break;
	case GL_DEPTH_CLAMP:
		c->depth_clamp = GL_TRUE;
		break;
	case GL_LINE_SMOOTH:
		// TODO implementation needs work/upgrade
		//c->line_smooth = GL_TRUE;
		break;
	case GL_BLEND:
		c->blend = GL_TRUE;
		break;
	case GL_COLOR_LOGIC_OP:
		c->logic_ops = GL_TRUE;
		break;
	case GL_POLYGON_OFFSET_FILL:
		c->poly_offset = GL_TRUE;
		break;
	case GL_SCISSOR_TEST:
		c->scissor_test = GL_TRUE;
		break;
	case GL_STENCIL_TEST:
		c->stencil_test = GL_TRUE;
		break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glDisable(GLenum cap)
{
	switch (cap) {
	case GL_CULL_FACE:
		c->cull_face = GL_FALSE;
		break;
	case GL_DEPTH_TEST:
		c->depth_test = GL_FALSE;
		break;
	case GL_DEPTH_CLAMP:
		c->depth_clamp = GL_FALSE;
		break;
	case GL_LINE_SMOOTH:
		c->line_smooth = GL_FALSE;
		break;
	case GL_BLEND:
		c->blend = GL_FALSE;
		break;
	case GL_COLOR_LOGIC_OP:
		c->logic_ops = GL_FALSE;
		break;
	case GL_POLYGON_OFFSET_FILL:
		c->poly_offset = GL_FALSE;
		break;
	case GL_SCISSOR_TEST:
		c->scissor_test = GL_FALSE;
		break;
	case GL_STENCIL_TEST:
		c->stencil_test = GL_FALSE;
		break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

GLboolean glIsEnabled(GLenum cap)
{
	// make up my own enum for this?  rename member as no_early_z?
	//GLboolean fragdepth_or_discard;
	switch (cap) {
	case GL_DEPTH_TEST: return c->depth_test;
	case GL_LINE_SMOOTH: return c->line_smooth;
	case GL_CULL_FACE: return c->cull_face;
	case GL_DEPTH_CLAMP: return c->depth_clamp;
	case GL_BLEND: return c->blend;
	case GL_COLOR_LOGIC_OP: return c->logic_ops;
	case GL_POLYGON_OFFSET_FILL: return c->poly_offset;
	case GL_SCISSOR_TEST: return c->scissor_test;
	case GL_STENCIL_TEST: return c->stencil_test;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}

	return GL_FALSE;
}

void glGetBooleanv(GLenum pname, GLboolean* params)
{
	// not sure it's worth adding every enum, spec says
	// gelGet* will convert/map types if they don't match the function
	switch (pname) {
	case GL_DEPTH_TEST:          *params = c->depth_test;   break;
	case GL_LINE_SMOOTH:         *params = c->line_smooth;  break;
	case GL_CULL_FACE:           *params = c->cull_face;    break;
	case GL_DEPTH_CLAMP:         *params = c->depth_clamp;  break;
	case GL_BLEND:               *params = c->blend;        break;
	case GL_COLOR_LOGIC_OP:      *params = c->logic_ops;    break;
	case GL_POLYGON_OFFSET_FILL: *params = c->poly_offset;  break;
	case GL_SCISSOR_TEST:        *params = c->scissor_test; break;
	case GL_STENCIL_TEST:        *params = c->stencil_test; break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glGetFloatv(GLenum pname, GLfloat* params)
{
	switch (pname) {
	case GL_POLYGON_OFFSET_FACTOR: *params = c->poly_factor; break;
	case GL_POLYGON_OFFSET_UNITS:  *params = c->poly_units;  break;
	case GL_POINT_SIZE:            *params = c->point_size;  break;
	case GL_DEPTH_CLEAR_VALUE:     *params = c->clear_depth; break;
	case GL_DEPTH_RANGE:
		params[0] = c->depth_range_near;
		params[1] = c->depth_range_near;
		break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glGetIntegerv(GLenum pname, GLint* params)
{
	// TODO maybe make all the enum/int member names match the associated ENUM?
	switch (pname) {
	case GL_STENCIL_WRITE_MASK:       params[0] = c->stencil_writemask; break;
	case GL_STENCIL_REF:              params[0] = c->stencil_ref; break;
	case GL_STENCIL_VALUE_MASK:       params[0] = c->stencil_valuemask; break;
	case GL_STENCIL_FUNC:             params[0] = c->stencil_func; break;
	case GL_STENCIL_FAIL:             params[0] = c->stencil_sfail; break;
	case GL_STENCIL_PASS_DEPTH_FAIL:  params[0] = c->stencil_dpfail; break;
	case GL_STENCIL_PASS_DEPTH_PASS:  params[0] = c->stencil_dppass; break;

	case GL_STENCIL_BACK_WRITE_MASK:       params[0] = c->stencil_writemask_back; break;
	case GL_STENCIL_BACK_REF:              params[0] = c->stencil_ref_back; break;
	case GL_STENCIL_BACK_VALUE_MASK:       params[0] = c->stencil_valuemask_back; break;
	case GL_STENCIL_BACK_FUNC:             params[0] = c->stencil_func_back; break;
	case GL_STENCIL_BACK_FAIL:             params[0] = c->stencil_sfail_back; break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL:  params[0] = c->stencil_dpfail_back; break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS:  params[0] = c->stencil_dppass_back; break;


	//TODO implement glBlendFuncSeparate and glBlendEquationSeparate
	case GL_LOGIC_OP_MODE:             params[0] = c->logic_func; break;
	case GL_BLEND_SRC_RGB:
	case GL_BLEND_SRC_ALPHA:           params[0] = c->blend_sfactor; break;
	case GL_BLEND_DST_RGB:
	case GL_BLEND_DST_ALPHA:           params[0] = c->blend_dfactor; break;

	case GL_BLEND_EQUATION_RGB:
	case GL_BLEND_EQUATION_ALPHA:      params[0] = c->blend_equation; break;

	case GL_CULL_FACE_MODE:            params[0] = c->cull_mode; break;
	case GL_FRONT_FACE:                params[0] = c->front_face; break;
	case GL_DEPTH_FUNC:                params[0] = c->depth_func; break;
	case GL_POINT_SPRITE_COORD_ORIGIN: params[0] = c->point_spr_origin;
	case GL_PROVOKING_VERTEX:          params[0] = c->provoking_vert; break;

	case GL_POLYGON_MODE:
		params[0] = c->poly_mode_front;
		params[1] = c->poly_mode_back;
		break;

	// TODO decide if 3.2 is the best approixmation
	case GL_MAJOR_VERSION:             params[0] = 3; break;
	case GL_MINOR_VERSION:             params[0] = 2; break;

	case GL_TEXTURE_BINDING_1D:        params[0] = c->bound_textures[GL_TEXTURE_1D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_2D:        params[0] = c->bound_textures[GL_TEXTURE_2D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_3D:        params[0] = c->bound_textures[GL_TEXTURE_3D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_1D_ARRAY:  params[0] = c->bound_textures[GL_TEXTURE_1D_ARRAY-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_2D_ARRAY:  params[0] = c->bound_textures[GL_TEXTURE_2D_ARRAY-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_RECTANGLE: params[0] = c->bound_textures[GL_TEXTURE_RECTANGLE-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_CUBE_MAP:  params[0] = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1]; break;

	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glCullFace(GLenum mode)
{
	c->cull_mode = mode;
}


void glFrontFace(GLenum mode)
{
	c->front_face = mode;
}

void glPolygonMode(GLenum face, GLenum mode)
{
	if (mode == GL_POINT) {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_point;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_point;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_point;
			c->draw_triangle_back = draw_triangle_point;
		}
	} else if (mode == GL_LINE) {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_line;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_line;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_line;
			c->draw_triangle_back = draw_triangle_line;
		}
	} else  {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_fill;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_fill;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_fill;
			c->draw_triangle_back = draw_triangle_fill;
		}
	}
}


void glPointSize(GLfloat size)
{
	c->point_size = size;
}

void glPointParameteri(GLenum pname, GLint param)
{
	c->point_spr_origin = param;
}


void glProvokingVertex(GLenum provokeMode)
{
	c->provoking_vert = provokeMode;
}


// Shader functions
GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard)
{
	glProgram tmp = {vertex_shader, fragment_shader, NULL, n, {0}, fragdepth_or_discard, GL_FALSE };
	for (int i=0; i<n; ++i) {
		tmp.interpolation[i] = interpolation[i];
	}

	for (int i=1; i<c->programs.size; ++i) {
		if (c->programs.a[i].deleted && i != c->cur_program) {
			c->programs.a[i] = tmp;
			return i;
		}
	}

	cvec_push_glProgram(&c->programs, tmp);
	return c->programs.size-1;
}

void glDeleteProgram(GLuint program)
{
	if (!program)
		return;

	c->programs.a[program].deleted = GL_TRUE;
}

void glUseProgram(GLuint program)
{
	c->vs_output.size = c->programs.a[program].vs_output_size;
	cvec_reserve_float(&c->vs_output.output_buf, c->vs_output.size * MAX_VERTICES);
	c->vs_output.interpolation = c->programs.a[program].interpolation;
	c->fragdepth_or_discard = c->programs.a[program].fragdepth_or_discard;

	c->cur_program = program;
}

void pglSetUniform(void* uniform)
{
	//TODO check for NULL? definitely if I ever switch to storing a local
	//copy in glProgram
	c->programs.a[c->cur_program].uniform = uniform;
}


void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	c->blend_sfactor = sfactor;
	c->blend_dfactor = dfactor;
}

void glBlendEquation(GLenum mode)
{
	c->blend_equation = mode;
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	SET_VEC4(c->blend_color, clampf_01(red), clampf_01(green), clampf_01(blue), clampf_01(alpha));
}

void glLogicOp(GLenum opcode)
{
	c->logic_func = opcode;
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
	c->poly_factor = factor;
	c->poly_units = units;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	c->scissor_lx = x;
	c->scissor_ly = y;
	c->scissor_ux = x+width;
	c->scissor_uy = y+height;
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	c->stencil_func = func;
	c->stencil_func_back = func;

	// TODO clamp byte function?
	if (ref > 255)
		ref = 255;
	if (ref < 0)
		ref = 0;

	c->stencil_ref = ref;
	c->stencil_ref_back = ref;

	c->stencil_valuemask = mask;
	c->stencil_valuemask_back = mask;
}

void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	if (face == GL_FRONT_AND_BACK) {
		glStencilFunc(func, ref, mask);
		return;
	}

	// TODO clamp byte function?
	if (ref > 255)
		ref = 255;
	if (ref < 0)
		ref = 0;

	if (face == GL_FRONT) {
		c->stencil_func = func;
		c->stencil_ref = ref;
		c->stencil_valuemask = mask;
	} else {
		c->stencil_func_back = func;
		c->stencil_ref_back = ref;
		c->stencil_valuemask_back = mask;
	}
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
	c->stencil_sfail = sfail;
	c->stencil_dpfail = dpfail;
	c->stencil_dppass = dppass;

	c->stencil_sfail_back = sfail;
	c->stencil_dpfail_back = dpfail;
	c->stencil_dppass_back = dppass;
}

void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	if (face == GL_FRONT_AND_BACK) {
		glStencilOp(sfail, dpfail, dppass);
		return;
	}

	if (face == GL_FRONT) {
		c->stencil_sfail = sfail;
		c->stencil_dpfail = dpfail;
		c->stencil_dppass = dppass;
	} else {
		c->stencil_sfail_back = sfail;
		c->stencil_dpfail_back = dpfail;
		c->stencil_dppass_back = dppass;
	}
}

void glClearStencil(GLint s)
{
	// stencil is 8 bit bytes so just hardcoding FF here
	c->clear_stencil = s & 0xFF;
}

void glStencilMask(GLuint mask)
{
	c->stencil_writemask = mask;
	c->stencil_writemask_back = mask;
}

void glStencilMaskSeparate(GLenum face, GLuint mask)
{
	if (face == GL_FRONT_AND_BACK) {
		glStencilMask(mask);
		return;
	}

	if (face == GL_FRONT) {
		c->stencil_writemask = mask;
	} else {
		c->stencil_writemask_back = mask;
	}
}


// Just wrap my pgl extension getter, unmap does nothing
void* glMapBuffer(GLenum target, GLenum access)
{
	// adjust to access bound_buffers
	target -= GL_ARRAY_BUFFER;

	void* data = NULL;
	pglGetBufferData(c->bound_buffers[target], &data);
	return data;
}

void* glMapNamedBuffer(GLuint buffer, GLenum access)
{
	void* data = NULL;
	pglGetBufferData(buffer, &data);
	return data;
}


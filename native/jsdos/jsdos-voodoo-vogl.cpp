#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/gl.h>

#include <stdlib.h>
#include <math.h>
#include <map>

// #include "dosbox.h"
// #include "dos_inc.h"
#include "logging.h"


typedef uint8_t	UINT8;
typedef signed char INT8;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned int UINT32;
typedef signed int INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;

#include "hardware/voodoo_vogl.h"


#ifdef EMSCRIPTEN
extern "C" void* gl4es_GetProcAddress(const char *name);
#define SDL_GL_GetProcAddress(x) gl4es_GetProcAddress(x)
#endif

PFNGLCREATESHADEROBJECTARBPROC db_glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC db_glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC db_glCompileShaderARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC db_glCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC db_glAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC db_glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC db_glUseProgramObjectARB = NULL;
PFNGLUNIFORM1IARBPROC db_glUniform1iARB = NULL;
PFNGLUNIFORM1FARBPROC db_glUniform1fARB = NULL;
PFNGLUNIFORM2FARBPROC db_glUniform2fARB = NULL;
PFNGLUNIFORM3FARBPROC db_glUniform3fARB = NULL;
PFNGLUNIFORM4FARBPROC db_glUniform4fARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC db_glGetUniformLocationARB = NULL;
PFNGLDETACHOBJECTARBPROC db_glDetachObjectARB = NULL;
PFNGLDELETEOBJECTARBPROC db_glDeleteObjectARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC db_glGetObjectParameterivARB = NULL;
PFNGLGETINFOLOGARBPROC db_glGetInfoLogARB = NULL;
PFNGLBLENDFUNCSEPARATEEXTPROC db_glBlendFuncSeparateEXT = NULL;
PFNGLGENERATEMIPMAPEXTPROC db_glGenerateMipmapEXT = NULL;
PFNGLGETATTRIBLOCATIONARBPROC db_glGetAttribLocationARB = NULL;
PFNGLVERTEXATTRIB1FARBPROC db_glVertexAttrib1fARB = NULL;


static int32_t opengl_version = -1;

static bool has_shaders = false;
static bool has_stencil = false;
static bool has_alpha = false;


static INT32 current_begin_mode = -1;

static int32_t current_depth_mode = -1;
static int32_t current_depth_func = -1;

static int32_t current_alpha_enabled = -1;
static int32_t current_src_rgb_fac = -1;
static int32_t current_dst_rgb_fac = -1;
static int32_t current_src_alpha_fac = -1;
static int32_t current_dst_alpha_fac = -1;

static bool depth_masked = false;
static bool color_masked = false;
static bool alpha_masked = false;

// buffer read/write defaults are back-buffer for double buffered contexts
static bool draw_to_front_buffer = false;
static bool read_from_front_buffer = false;


void VOGL_Reset(void) {
	opengl_version = -1;
	has_shaders = false;
	has_stencil = false;
	has_alpha = false;

	current_depth_mode=-1;
	current_depth_func=-1;

	current_alpha_enabled=-1;
	current_src_rgb_fac=-1;
	current_dst_rgb_fac=-1;
	current_src_alpha_fac=-1;
	current_dst_alpha_fac=-1;

	depth_masked = false;
	color_masked = false;
	alpha_masked = false;

	draw_to_front_buffer = false;
	read_from_front_buffer = false;
}


void VOGL_InitVersion(void) {
	opengl_version = -1;

	char gl_verstr[16];
	const GLubyte* gl_verstr_ub = glGetString(GL_VERSION);
	strncpy(gl_verstr, (const char*)gl_verstr_ub, 16);
	gl_verstr[15] = 0;
	char* gl_ver_minor = strchr(gl_verstr, '.');
	if (gl_ver_minor != NULL) {
		gl_ver_minor++;
		char* skip = strchr(gl_ver_minor, '.');
		if (skip != NULL) *skip = 0;
	}

	int ogl_ver = 100;
	if (gl_verstr[0] != 0) {
		int major = 1;
		int minor = 0;
		if (strchr(gl_verstr, '.') != NULL) {
			if (sscanf(gl_verstr,"%d.%d", &major, &minor) != 2) major = 0;
		} else {
			if (sscanf(gl_verstr, "%d", &major) != 1) major = 0;
		}
		if (major > 0) {
			ogl_ver = major*100;
			if (minor >= 0) {
				if (minor < 10) ogl_ver += minor*10;
				else ogl_ver += minor;
			}
		}
	}

	if (ogl_ver > 0) opengl_version = ogl_ver;
}

void VOGL_ClearShaderFunctions(void) {
	db_glShaderSourceARB = NULL;
	db_glCompileShaderARB = NULL;
	db_glCreateProgramObjectARB = NULL;
	db_glAttachObjectARB = NULL;
	db_glLinkProgramARB = NULL;
	db_glUseProgramObjectARB = NULL;
	db_glUniform1iARB = NULL;
	db_glUniform1fARB = NULL;
	db_glUniform2fARB = NULL;
	db_glUniform3fARB = NULL;
	db_glUniform4fARB = NULL;
	db_glGetUniformLocationARB = NULL;
	db_glDetachObjectARB = NULL;
	db_glDeleteObjectARB  = NULL;
	db_glGetObjectParameterivARB = NULL;
	db_glGetInfoLogARB = NULL;
}

bool VOGL_Initialize(void) {
	VOGL_ClearShaderFunctions();
	
	VOGL_InitVersion();

#ifdef WIN32
	__glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glActiveTextureARB"));
	if (!__glActiveTextureARB) {
		LOG_MSG("opengl: glActiveTextureARB extension not supported");
		return false;
	}

	__glMultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glMultiTexCoord4fARB"));
	if (!__glMultiTexCoord4fARB) {
		LOG_MSG("opengl: glMultiTexCoord4fARB extension not supported");
		return false;
	}

	__glMultiTexCoord4fvARB = (PFNGLMULTITEXCOORD4FVARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glMultiTexCoord4fvARB"));
	if (!__glMultiTexCoord4fvARB) {
		LOG_MSG("opengl: glMultiTexCoord4fvARB extension not supported");
		return false;
	}
#endif

	db_glBlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC)((uintptr_t)SDL_GL_GetProcAddress("glBlendFuncSeparateEXT"));
	if (!db_glBlendFuncSeparateEXT) {
		LOG_MSG("opengl: glBlendFuncSeparateEXT extension not supported");
		return false;
	}

	db_glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)((uintptr_t)SDL_GL_GetProcAddress("glGenerateMipmapEXT"));
	if (!db_glGenerateMipmapEXT) {
		LOG_MSG("opengl: glGenerateMipmapEXT extension not supported");
		return false;
	}

	if (VOGL_CheckFeature(VOGL_ATLEAST_V20)) {
		const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
		if (strstr(extensions, "GL_ARB_shader_objects") && strstr(extensions, "GL_ARB_vertex_shader") &&
			strstr(extensions, "GL_ARB_fragment_shader")) {

			

			db_glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glCreateShaderObjectARB"));
			if (!db_glCreateShaderObjectARB) {
				LOG_MSG("opengl: shader extensions not supported. Using fixed pipeline");
			} else {
				db_glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glShaderSourceARB"));
				if (!db_glShaderSourceARB) LOG_MSG("opengl: glShaderSourceARB extension not supported");

				db_glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glCompileShaderARB"));
				if (!db_glCompileShaderARB) LOG_MSG("opengl: glCompileShaderARB extension not supported");

				db_glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glCreateProgramObjectARB"));
				if (!db_glCreateProgramObjectARB) LOG_MSG("opengl: glCreateProgramObjectARB extension not supported");

				db_glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glAttachObjectARB"));
				if (!db_glAttachObjectARB) LOG_MSG("opengl: glAttachObjectARB extension not supported");

				db_glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glLinkProgramARB"));
				if (!db_glLinkProgramARB) LOG_MSG("opengl: glLinkProgramARB extension not supported");

				db_glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glUseProgramObjectARB"));
				if (!db_glUseProgramObjectARB) LOG_MSG("opengl: glUseProgramObjectARB extension not supported");

				db_glUniform1iARB = (PFNGLUNIFORM1IARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glUniform1iARB"));
				if (!db_glUniform1iARB) LOG_MSG("opengl: glUniform1iARB extension not supported");

				db_glUniform1fARB = (PFNGLUNIFORM1FARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glUniform1fARB"));
				if (!db_glUniform1fARB) LOG_MSG("opengl: glUniform1fARB extension not supported");

				db_glUniform2fARB = (PFNGLUNIFORM2FARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glUniform2fARB"));
				if (!db_glUniform2fARB) LOG_MSG("opengl: glUniform2fARB extension not supported");

				db_glUniform3fARB = (PFNGLUNIFORM3FARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glUniform3fARB"));
				if (!db_glUniform3fARB) LOG_MSG("opengl: glUniform3fARB extension not supported");

				db_glUniform4fARB = (PFNGLUNIFORM4FARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glUniform4fARB"));
				if (!db_glUniform4fARB) LOG_MSG("opengl: glUniform4fARB extension not supported");

				db_glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glGetUniformLocationARB"));
				if (!db_glGetUniformLocationARB) LOG_MSG("opengl: glGetUniformLocationARB extension not supported");

				db_glDetachObjectARB = (PFNGLDETACHOBJECTARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glDetachObjectARB"));
				if (!db_glDetachObjectARB) LOG_MSG("opengl: glDetachObjectARB extension not supported");

				db_glDeleteObjectARB  = (PFNGLDELETEOBJECTARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glDeleteObjectARB"));
				if (!db_glDeleteObjectARB) LOG_MSG("opengl: glDeleteObjectARB extension not supported");

				db_glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glGetObjectParameterivARB"));
				if (!db_glGetObjectParameterivARB) LOG_MSG("opengl: glGetObjectParameterivARB extension not supported");

				db_glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glGetInfoLogARB"));
				if (!db_glGetInfoLogARB) LOG_MSG("opengl: glGetInfoLogARB extension not supported");

				db_glGetAttribLocationARB = (PFNGLGETATTRIBLOCATIONARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glGetAttribLocationARB"));
				if (!db_glGetAttribLocationARB) LOG_MSG("opengl: glGetAttribLocationARB extension not supported");
			
				db_glVertexAttrib1fARB = (PFNGLVERTEXATTRIB1FARBPROC)((uintptr_t)SDL_GL_GetProcAddress("glVertexAttrib1fARB"));
				if (!db_glVertexAttrib1fARB) LOG_MSG("opengl: glVertexAttrib1fARB extension not supported");

				if (db_glShaderSourceARB && db_glCompileShaderARB && db_glCreateProgramObjectARB &&
					db_glAttachObjectARB && db_glLinkProgramARB && db_glUseProgramObjectARB &&
					db_glUniform1iARB && db_glUniform1fARB && db_glUniform2fARB && db_glUniform3fARB &&
					db_glUniform4fARB && db_glGetUniformLocationARB && db_glDetachObjectARB &&
					db_glDeleteObjectARB && db_glGetObjectParameterivARB && db_glGetInfoLogARB) {
						VOGL_FlagFeature(VOGL_HAS_SHADERS);
						printf("opengl: shader functionality enabled\n");
//						LOG_MSG("opengl: shader functionality enabled");
				} else {
					printf("opengl: shader functionality not enabled\n");
					VOGL_ClearShaderFunctions();
				}
			}
		}
	}

	printf("opengl: I am able to use OpenGL to emulate Voodoo graphics\n");
	return true;
}


bool VOGL_CheckFeature(uint32_t feat) {
	switch (feat) {
		case VOGL_ATLEAST_V20:
			if (opengl_version >= 200) return true;
			break;
		case VOGL_ATLEAST_V21:
			if (opengl_version >= 210) return true;
			break;
		case VOGL_ATLEAST_V30:
			if (opengl_version >= 300) return true;
			break;
		case VOGL_HAS_SHADERS:
			if (has_shaders) return true;
			break;
		case VOGL_HAS_STENCIL_BUFFER:
			if (has_stencil) return true;
			break;
		case VOGL_HAS_ALPHA_PLANE:
			if (has_alpha) return true;
			break;
		default:
			LOG_MSG("opengl: unknown feature queried: %x",feat);
			break;
	}

	return false;
}

void VOGL_FlagFeature(uint32_t feat) {
	switch (feat) {
		case VOGL_HAS_SHADERS:
			has_shaders = true;
			break;
		case VOGL_HAS_STENCIL_BUFFER:
			has_stencil = true;
			break;
		case VOGL_HAS_ALPHA_PLANE:
			has_alpha = true;
			break;
		default:
			LOG_MSG("opengl: unknown feature: %x",feat);
			break;
	}
}


void VOGL_BeginMode(INT32 new_mode) {
	if (current_begin_mode > -1) {
		if (new_mode != current_begin_mode) {
			glEnd();
			if (new_mode > -1) glBegin((GLenum)new_mode);
			current_begin_mode = new_mode;
		}
	} else {
		if (new_mode > -1) {
			glBegin((GLenum)new_mode);
			current_begin_mode = new_mode;
		}
	}
}

void VOGL_ClearBeginMode(void) {
	if (current_begin_mode > -1) {
		glEnd();
		current_begin_mode = -1;
	}
}


void VOGL_SetDepthMode(int32_t mode, int32_t func) {
	if (current_depth_mode!=mode) {
		if (mode!=0) {
			VOGL_ClearBeginMode();
			glEnable(GL_DEPTH_TEST);
			current_depth_mode=1;
			if (current_depth_func!=func) {
				glDepthFunc((GLenum)(GL_NEVER+func));
				current_depth_func=func;
			}
		} else {
			VOGL_ClearBeginMode();
			glDisable(GL_DEPTH_TEST);
			current_depth_mode=0;
		}
	} else {
		if ((mode!=0) && (current_depth_func!=func)) {
			VOGL_ClearBeginMode();
			glDepthFunc((GLenum)(GL_NEVER+func));
			current_depth_func=func;
		}
	}
}


void VOGL_SetAlphaMode(int32_t enabled_mode,GLuint src_rgb_fac,GLuint dst_rgb_fac,
											GLuint src_alpha_fac,GLuint dst_alpha_fac) {
	if (current_alpha_enabled!=enabled_mode) {
		VOGL_ClearBeginMode();
		if (enabled_mode!=0) {
			glEnable(GL_BLEND);
			current_alpha_enabled=1;
			if ((current_src_rgb_fac!=(int32_t)src_rgb_fac) || (current_dst_rgb_fac!=(int32_t)dst_rgb_fac) ||
				(current_src_alpha_fac!=(int32_t)src_alpha_fac) || (current_dst_alpha_fac!=(int32_t)dst_alpha_fac)) {
				db_glBlendFuncSeparateEXT(src_rgb_fac, dst_rgb_fac, src_alpha_fac, dst_alpha_fac);
				current_src_rgb_fac=(int32_t)src_rgb_fac;
				current_dst_rgb_fac=(int32_t)dst_rgb_fac;
				current_src_alpha_fac=(int32_t)src_alpha_fac;
				current_dst_alpha_fac=(int32_t)dst_alpha_fac;
			}
		} else {
			glDisable(GL_BLEND);
			current_alpha_enabled=0;
		}
	} else {
		if (current_alpha_enabled!=0) {
			if ((current_src_rgb_fac!=(int32_t)src_rgb_fac) || (current_dst_rgb_fac!=(int32_t)dst_rgb_fac) ||
				(current_src_alpha_fac!=(int32_t)src_alpha_fac) || (current_dst_alpha_fac!=(int32_t)dst_alpha_fac)) {
				VOGL_ClearBeginMode();
				db_glBlendFuncSeparateEXT(src_rgb_fac, dst_rgb_fac, src_alpha_fac, dst_alpha_fac);
				current_src_rgb_fac=(int32_t)src_rgb_fac;
				current_dst_rgb_fac=(int32_t)dst_rgb_fac;
				current_src_alpha_fac=(int32_t)src_alpha_fac;
				current_dst_alpha_fac=(int32_t)dst_alpha_fac;
			}
		}
	}
}


void VOGL_SetDepthMaskMode(bool masked) {
	if (depth_masked!=masked) {
		VOGL_ClearBeginMode();
		if (masked) {
			glDepthMask(GL_TRUE);
			depth_masked=true;
		} else {
			glDepthMask(GL_FALSE);
			depth_masked=false;
		}
	}
}


void VOGL_SetColorMaskMode(bool cmasked, bool amasked) {
	if ((color_masked!=cmasked) || (alpha_masked!=amasked)) {
		color_masked=cmasked;
		alpha_masked=amasked;
		GLboolean cm = (color_masked ? GL_TRUE : GL_FALSE);
		GLboolean am = (alpha_masked ? GL_TRUE : GL_FALSE);
		glColorMask(cm,cm,cm,am);
	}
}


void VOGL_SetDrawMode(bool front_draw) {
	if (draw_to_front_buffer!=front_draw) {
		VOGL_ClearBeginMode();
		if (front_draw) glDrawBuffer(GL_FRONT);
		else glDrawBuffer(GL_BACK);
		draw_to_front_buffer=front_draw;
	}
}


void VOGL_SetReadMode(bool front_read) {
	VOGL_ClearBeginMode();

	if (read_from_front_buffer!=front_read) {
		if (front_read) glReadBuffer(GL_FRONT);
		else glReadBuffer(GL_BACK);
		read_from_front_buffer=front_read;
	}
}


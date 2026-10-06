#include "glWrapper.h"
#include "glcommon_internal.h"


static void GLAD_API_PTR _debugOutputGL(
    GLenum source,GLenum type,GLuint id,GLenum severity,
    GLsizei length,const GLchar *message,const void *userParam)
{
	std::string error_msg;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               error_msg += "[ERROR] ";                break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: error_msg += "[DEPRECATED] ";           break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  error_msg += "[UNDEFINED_BEHAVIOUR] ";  break;
	case GL_DEBUG_TYPE_PORTABILITY:         error_msg += "[PORTABILITY] ";          break;
	case GL_DEBUG_TYPE_PERFORMANCE:         error_msg += "[PERFORMANCE] ";          break;
	case GL_DEBUG_TYPE_OTHER:               error_msg += "[OTHER] ";                break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:    error_msg += "(SEVERE) ";       break;
	case GL_DEBUG_SEVERITY_MEDIUM:  error_msg += "(KINDA SEVERE) "; break;
	case GL_DEBUG_SEVERITY_LOW:     error_msg += "(WARNING) ";      break;
	}

	error_msg += message;
	error_msg += '\n';

	_GLError(error_msg.c_str());
}

void GLDebugLayer_initialize()
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glDebugMessageCallback(_debugOutputGL, nullptr);

    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, true);
	
}
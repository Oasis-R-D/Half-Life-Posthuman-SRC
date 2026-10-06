#define _GLASSERT(x, msg) if(!(x)) _GLError(msg)
extern void _GLError(const char* msg);
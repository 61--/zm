#ifndef __OUT_H
#define __OUT_H


void __DbgOut(const char* format, ...);
void __DbgOut_(const char* file, int line, const char* format, ...);

#define DbgOut(format, ...) __DbgOut_(__FILE__, __LINE__, format, __VA_ARGS__)

#endif//__OUT_H
// VsVer.h

#pragma once

/***************************************/
// Ϊ�˴�����vc6��vs2010�¶���ͨ�����룬�������º�
#if _MSC_VER == 1200		// vc6��cl�汾��
#define _ultoa_s _ultoa
#define _itoa_s _itoa
#define _ltoa_s _ltoa
#define _gcvt_s(buffer, sizeInBytes, value, digits) _gcvt(value, digits, buffer)
#define sprintf_s sprintf
#define fopen_s(p,f,m) {*p = fopen(f,m);}
#define _tfopen_s(p,f,m) {*p = _tfopen(f,m);}
#define _tfreopen_s(p,t,m,s) {*p = _tfreopen(t,m,s);}
#define _tcscpy_s _tcscpy
#define strcpy_s(dst, ct, src) strcpy(dst, src)
#define _vsntprintf_s(buff, size, count, format, argptr) _vsntprintf(buff, count, format, argptr)
#define _vsnwprintf_s(buff, size, count, format, argptr) _vsnwprintf(buff, count, format, argptr)
#define _vsnprintf_s(buff, size, count, format, argptr) _vsnprintf(buff, count, format, argptr)
//#define _snprintf_s(buff, size, count, format, argptr) _snprintf(buff, count, format, argptr)
//#define _snprintf_s(buff, count, format, argptr) _snprintf(buff, format, argptr)
#define _stprintf_s(buff, format, arg1, arg2) _stprintf(buff, format, arg1, arg2)
#define strcat_s(dst, len, src) strcat(dst, src)
#define _countof sizeof

#endif
/***************************************/
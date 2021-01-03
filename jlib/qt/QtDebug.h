#pragma once

#include <QDebug>
#ifdef _WIN32
#include <Windows.h>
#endif //_WIN32


#define JLIBQT_QDEBUG_FILE_LINE_STREAM "[" << __FUNCTION__ << "ln" << __LINE__ << "]: "
#define JLIBQT_QDEBUG_FILE_LINE_VALUE (QString("[") + __FUNCTION__ + " ln" + QString::number(__LINE__) + "]: ")

#define MYQDEBUG qDebug() << JLIBQT_QDEBUG_FILE_LINE_STREAM
#define MYQINFO qInfo() << JLIBQT_QDEBUG_FILE_LINE_STREAM
#define MYQWARN qWarning() << JLIBQT_QDEBUG_FILE_LINE_STREAM
#define MYQCRITICAL qCritical() << JLIBQT_QDEBUG_FILE_LINE_STREAM

//! ���������кſ���
// #define JLIBQT_SHOW_LINE 0
//! ���кŴ����·������ֵʱ�����������кţ�������ԡ����ڶ��cpp�ļ��ֱ��岻ͬ��ֵ��
// #define JLIBQT_SHOW_MSGBOX_AFTER_LINE 1

#if JLIBQT_SHOW_LINE
#ifdef JLIBQT_SHOW_MSGBOX_AFTER_LINE
#define JLIBQT_SHOW_MSGBOX_LINE_NUMBER if(__LINE__ >= JLIBQT_SHOW_MSGBOX_AFTER_LINE) { MessageBoxW(nullptr, (std::wstring(__FILEW__) + L":" + std::to_wstring(__LINE__)).data(), L"Test", 0); }
#define JLIBQT_SHOW_MSGBOX_LINE_NUMBER_WITH_TITLE(title) if(__LINE__ >= JLIBQT_SHOW_MSGBOX_AFTER_LINE) { MessageBoxW(nullptr, (std::wstring(__FILEW__) + L":" + std::to_wstring(__LINE__)).data(), title, 0); }
#else
#define JLIBQT_SHOW_MSGBOX_LINE_NUMBER { MessageBoxW(nullptr, (std::wstring(__FILEW__) + L":" + std::to_wstring(__LINE__)).data(), L"Test", 0); }
#define JLIBQT_SHOW_MSGBOX_LINE_NUMBER_WITH_TITLE(title) { MessageBoxW(nullptr, (std::wstring(__FILEW__) + L":" + std::to_wstring(__LINE__)).data(), title, 0); }
#endif
#else
#define JLIBQT_SHOW_MSGBOX_LINE_NUMBER
#define JLIBQT_SHOW_MSGBOX_LINE_NUMBER_WITH_TITLE(title)
#endif

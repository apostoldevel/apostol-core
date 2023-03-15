/*++

Library name:

  apostol-core

Module Name:

  Log.hpp

Notices:

   Apostol Core (Log)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_LOG_HPP
#define APOSTOL_LOG_HPP
//----------------------------------------------------------------------------------------------------------------------

#define LOG_MAX_ERROR_STR     2048
//----------------------------------------------------------------------------------------------------------------------

#define APP_LOG_STDERR            0
#define APP_LOG_EMERG             1
#define APP_LOG_ALERT             2
#define APP_LOG_CRIT              3
#define APP_LOG_ERR               4
#define APP_LOG_WARN              5
#define APP_LOG_NOTICE            6
#define APP_LOG_INFO              7
#define APP_LOG_DEBUG             8
//----------------------------------------------------------------------------------------------------------------------

#define APP_LOG_DEBUG_CORE        0x010u
#define APP_LOG_DEBUG_ALLOC       0x020u
#define APP_LOG_DEBUG_MUTEX       0x040u
#define APP_LOG_DEBUG_EVENT       0x080u
#define APP_LOG_DEBUG_HTTP        0x100u
#define APP_LOG_DEBUG_ALL         0x7f0u
#define APP_LOG_DEBUG_CONNECTION  0x800u
//----------------------------------------------------------------------------------------------------------------------

#define COLOR_OFF    "\e[0m"

#define CC_BEGIN     "\e["
#define CC_END       "m"

#define CC_BOLD      "1;"
#define CC_UNDERLINE "1;"

#define CC_NORMAL    "3"
#define CC_LIGHT     "9"

#define CC_BLACK     "0"
#define CC_RED       "1"
#define CC_GREEN     "2"
#define CC_YELLOW    "3"
#define CC_BLUE      "4"
#define CC_MAGENTA   "5"
#define CC_CYAN      "6"
#define CC_WHITE     "7"

#define COLOR_BLACK   CC_BEGIN CC_NORMAL CC_BLACK   CC_END
#define COLOR_RED     CC_BEGIN CC_NORMAL CC_RED     CC_END
#define COLOR_GREEN   CC_BEGIN CC_NORMAL CC_GREEN   CC_END
#define COLOR_YELLOW  CC_BEGIN CC_NORMAL CC_YELLOW  CC_END
#define COLOR_BLUE    CC_BEGIN CC_NORMAL CC_BLUE    CC_END
#define COLOR_MAGENTA CC_BEGIN CC_NORMAL CC_MAGENTA CC_END
#define COLOR_CYAN    CC_BEGIN CC_NORMAL CC_CYAN    CC_END
#define COLOR_WHITE   CC_BEGIN CC_NORMAL CC_WHITE   CC_END

//----------------------------------------------------------------------------------------------------------------------

#ifndef log_pid
#define log_pid                  MainProcessID
#endif
//----------------------------------------------------------------------------------------------------------------------

#define log_tid                  MainThreadID
#define LOG_TID_T_FMT            "%P"
//----------------------------------------------------------------------------------------------------------------------

static string_t err_levels[] = {
        CreateString("stderr"),
        CreateString("emerg"),
        CreateString("alert"),
        CreateString("crit"),
        CreateString("error"),
        CreateString("warn"),
        CreateString("notice"),
        CreateString("info"),
        CreateString("debug")
};
//----------------------------------------------------------------------------------------------------------------------

static string_t level_colors[] = {
        CreateString(CC_BEGIN         CC_LIGHT  CC_WHITE   CC_END),
        CreateString(CC_BEGIN         CC_LIGHT  CC_BLUE    CC_END),
        CreateString(CC_BEGIN         CC_LIGHT  CC_MAGENTA CC_END),
        CreateString(CC_BEGIN CC_BOLD CC_LIGHT  CC_RED     CC_END),
        CreateString(CC_BEGIN         CC_LIGHT  CC_RED     CC_END),
        CreateString(CC_BEGIN         CC_LIGHT  CC_YELLOW  CC_END),
        CreateString(CC_BEGIN         CC_NORMAL CC_CYAN    CC_END),
        CreateString(CC_BEGIN         CC_NORMAL CC_GREEN   CC_END),
        CreateString(CC_BEGIN         CC_NORMAL CC_WHITE   CC_END)
};
//----------------------------------------------------------------------------------------------------------------------

static u_int GetLogLevelByName(LPCTSTR lpszName) {
    for (size_t I = 1; I < chARRAY(err_levels); ++I) {
        if (SameText(lpszName, err_levels[I].str))
            return I;
    }
    return 0;
};
//----------------------------------------------------------------------------------------------------------------------

static inline ssize_t write_fd(int fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}
//----------------------------------------------------------------------------------------------------------------------

static inline void write_stderr(LPCSTR text)
{
    (void) write_fd(STDERR_FILENO, (char *) text, strlen(text));
}
//----------------------------------------------------------------------------------------------------------------------

#define write_console        write_fd
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Log {

        class CLog;
        //--------------------------------------------------------------------------------------------------------------

        extern CLog *GLog;
        //--------------------------------------------------------------------------------------------------------------

        enum CLogType { ltError = 0, ltAccess, ltPostgres, ltStream, ltDebug };

        //--------------------------------------------------------------------------------------------------------------

        //-- CLogFile --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CLogFile: public CFile, public CCollectionItem {
        private:

            u_int m_uLevel;

            CLogType m_LogType;

        public:

            explicit CLogFile(CLog *ALog, const CString &FileName);

            ~CLogFile() override = default;

            u_int Level() const { return m_uLevel; }
            void Level(u_int Value) { m_uLevel = Value; };

            CLogType LogType() { return m_LogType; }
            void LogType(CLogType Value) { m_LogType = Value; };

        }; // class CLogFile

        //--------------------------------------------------------------------------------------------------------------

        //-- CLogComponent ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CLogComponent {
        public:
            CLogComponent();
            ~CLogComponent();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CLog ------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CLog: public CSysErrorComponent, public CCollection {
            typedef CCollection inherited;
        private:

            u_int       m_uLevel;
            int         m_CurrentIndex;
            bool        m_fUseStdErr;
            time_t      m_DiskFullTime;

        protected:

            static char *StrError(int AError, char *AStr, size_t ASize);
            static char *ErrNo(char *ADest, char *ALast, int AError);
            void ErrorCore(u_int ALevel, int AError, LPCSTR AFormat, CLogType ALogType, va_list args);

            void SetLevel(u_int Value);

            void CheckCurrentIndex();
            void SetCurrentIndex(int Index);

        public:

            CLog();

            explicit CLog(const CString &FileName, u_int ALevel);

            inline static class CLog *CreateLog() { return GLog = new CLog(); };

            inline static void DestroyLog() { delete GLog; };

            ~CLog() override = default;

            CLogFile *AddLogFile(const CString &FileName, u_int ALevel = APP_LOG_STDERR);

            void Error(u_int ALevel, int AErrNo, LPCSTR AFormat, ...);
            void Error(u_int ALevel, int AErrNo, LPCSTR AFormat, va_list args);

            void Debug(u_int ALevel, LPCSTR AFormat, ...);
            void Debug(u_int ALevel, LPCSTR AFormat, va_list args);

            void Warning(LPCSTR AFormat, ...);
            void Warning(LPCSTR AFormat, va_list args);

            void Notice(LPCSTR AFormat, ...);
            void Notice(LPCSTR AFormat, va_list args);

            void Message(LPCSTR AFormat, ...);
            void Message(LPCSTR AFormat, va_list args);

            void Access(LPCSTR AFormat, ...);
            void Access(LPCSTR AFormat, va_list args);

            void Stream(LPCSTR AFormat, ...);
            void Stream(LPCSTR AFormat, va_list args);

            void Postgres(u_int ALevel, LPCSTR AFormat, ...);
            void Postgres(u_int ALevel, LPCSTR AFormat, va_list args);

            u_int Level() const { return m_uLevel; }
            void Level(u_int Value) { SetLevel(Value); };

            time_t DiskFullTime() const { return m_DiskFullTime; };
            void DiskFullTime(time_t Value) { m_DiskFullTime = Value; };

            bool UseStdErr() const { return m_fUseStdErr; };
            void UseStdErr(bool Value) { m_fUseStdErr = Value; };

            void RedirectStdErr();

            int CurrentIndex() const { return m_CurrentIndex; }
            void CurrentIndex(int Index) { SetCurrentIndex(Index); };

            CLogFile *Current();
            CLogFile *First();
            CLogFile *Last();
            CLogFile *Prior();
            CLogFile *Next();

            CLogFile *LogFiles(int Index) { return (CLogFile *) inherited::GetItem(Index); }
            void LogFiles(int Index, CLogFile *Item) { inherited::SetItem(Index, Item); }

        }; // class CLog
    }
}

using namespace Apostol::Log;
}
//----------------------------------------------------------------------------------------------------------------------

#endif // APOSTOL_LOG_HPP

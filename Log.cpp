/*++

Library name:

  apostol-core

Module Name:

  Log.cpp

Notices:

  Apostol Core (Log)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Log.hpp"

extern "C++" {

namespace Apostol {

    namespace Log {

        static unsigned long GLogCount = 0;
        //--------------------------------------------------------------------------------------------------------------

        CLog *GLog = nullptr;
        //--------------------------------------------------------------------------------------------------------------

        inline void AddLog() {
            if (GLogCount == 0) {
                GLog = CLog::CreateLog();
            }

            GLogCount++;
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void RemoveLog() {
            GLogCount--;

            if (GLogCount == 0) {
                CLog::DestroyLog();
                GLog = nullptr;
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CFile -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CFile::CFile(LPCSTR AFileName, int AFlags): CObject() {

            m_hHandle = INVALID_FILE;
            m_iFlags = AFlags;
            m_uOffset = 0;

            m_lpFileName = nullptr;
            m_OnFilerError = nullptr;

            SetFileName(AFileName);
        };
        //--------------------------------------------------------------------------------------------------------------

        CFile::~CFile() {
            Close(true);
            SetFileName(nullptr);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CFile::SetFileName(LPCSTR AValue) {

            if (m_lpFileName != nullptr) {
                ::free(m_lpFileName);
                m_lpFileName = nullptr;
            }

            if (AValue != nullptr) {

                size_t len = strlen(AValue);

                m_lpFileName = (char *) ::malloc(len + 1);
                m_lpFileName = strcpy(m_lpFileName, AValue);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CFile::Open() {

            if (m_lpFileName == nullptr) {
                throw EFilerError(_T("Error open file: file name cannot be empty"));
            }

            Close();

            if (strncmp(m_lpFileName, "stderr:", 7) == 0) {
                m_hHandle = STDERR_FILENO;
            } else {
                m_hHandle = ::open(m_lpFileName, m_iFlags, FILE_DEFAULT_ACCESS);

                if (m_hHandle == INVALID_FILE) {
                    throw EFilerError(errno, _T("Could not open file: \"%s\" error: "), m_lpFileName);
                }
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CFile::Close(bool ASafe) {
            if ((m_hHandle != INVALID_FILE) && (m_hHandle != STDERR_FILENO)) {
                if (::close(m_hHandle) == FILE_ERROR) {
                    if (ASafe) {
                        DoFilerError(errno, FILE_CLOSE_MESSAGE, m_lpFileName);
                    } else {
                        throw EFilerError(errno, FILE_CLOSE_MESSAGE, m_lpFileName);
                    }
                } else {
                    m_hHandle = INVALID_FILE;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        ssize_t CFile::Read(char *buf, size_t size, size_t offset)
        {
            ssize_t  n;

            n = pread(m_hHandle, buf, size, offset);

            if (n == -1) {
                throw EReadError(errno, _T("pread() \"%s\" failed: "), m_lpFileName);
            }

            m_uOffset += n;

            return n;
        }
        //--------------------------------------------------------------------------------------------------------------

        ssize_t CFile::Write(char *buf, size_t size, size_t offset)
        {
            ssize_t n;
            size_t  written;
            int     err;

            written = 0;

            for ( ;; ) {
                n = pwrite(m_hHandle, buf + written, size, offset);

                if (n == -1) {
                    err = errno;

                    if (err == EINTR) {
                        continue;
                    }

                    throw EWriteError(errno, _T("pwrite() \"%s\" failed"), m_lpFileName);
                }

                m_uOffset += n;
                written += n;

                if ((size_t) n == size) {
                    return written;
                }

                offset += n;
                size -= n;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CFile::DoFilerError(int AError, LPCTSTR lpFormat, ...) {
            if (m_OnFilerError) {
                va_list args;
                va_start(args, lpFormat);
                m_OnFilerError((Pointer) this, AError, lpFormat, args);
                va_end(args);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CLogFile --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CLogFile::CLogFile(CLog *ALog, LPCSTR AFileName) :
                CFile(AFileName, FILE_APPEND | FILE_CREATE_OR_OPEN),
                CCollectionItem(ALog), m_uLevel(APP_LOG_STDERR), m_LogType(ltError) {
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CLogComponent ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CLogComponent::CLogComponent() {
            AddLog();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogComponent::~CLogComponent() {
            RemoveLog();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CLog ------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CLog::CLog(): CSysErrorComponent(), CCollection(this) {
            m_uLevel = APP_LOG_DEBUG_CORE;
            m_CurrentIndex = -1;
            m_fUseStdErr = true;
            m_DiskFullTime = 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        CLog::CLog(LPCSTR  AFileName, u_int ALevel): CLog() {
            AddLogFile(AFileName, ALevel);
        };
        //--------------------------------------------------------------------------------------------------------------

        char *CLog::StrError(int AError, char *ADest, size_t ASize)
        {
            const CString &Msg = GSysError->StrError(AError);
            ASize = Min(ASize, Msg.Size());

            return MemCopy(ADest, Msg.c_str(), ASize);
        }
        //--------------------------------------------------------------------------------------------------------------

        char *CLog::ErrNo(char *ADest, char *ALast, int AError)
        {
            if (ADest > ALast - 50) {

                /* leave a space for an error code */

                ADest = ALast - 50;
                *ADest++ = '.';
                *ADest++ = '.';
                *ADest++ = '.';
            }

            ADest = ld_slprintf(ADest, ALast, " (%d: ", AError);
            ADest = StrError(AError, ADest, ALast - ADest);

            if (ADest < ALast) {
                *ADest++ = ')';
            }

            return ADest;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::ErrorCore(u_int ALevel, int AError, LPCSTR AFormat, CLogType ALogType, va_list args) {
            TCHAR       *f, *last_f;
            TCHAR       *c, *last_c;
            TCHAR       *last_a;

            ssize_t     n;

            bool        wrote_stderr;

            TCHAR       time_str [LOG_MAX_ERROR_STR] = {0};
            TCHAR       args_str [LOG_MAX_ERROR_STR] = {0};
            TCHAR       file_str [LOG_MAX_ERROR_STR] = {0};
            TCHAR       cons_str [LOG_MAX_ERROR_STR] = {0};

            time_t      itime;
            struct tm   *timeInfo;

            last_f = file_str + LOG_MAX_ERROR_STR * sizeof(TCHAR);
            last_c = cons_str + LOG_MAX_ERROR_STR * sizeof(TCHAR);
            last_a = args_str + LOG_MAX_ERROR_STR * sizeof(TCHAR);

            itime = time(&itime);
            timeInfo = localtime(&itime);

            strftime(time_str, LOG_MAX_ERROR_STR, "%Y/%m/%d %H:%M:%S", timeInfo);

            f = ld_slprintf(file_str, last_f, "[%s] ", time_str);
            c = ld_slprintf(cons_str, last_c, COLOR_WHITE "[%s] ", time_str);
#ifdef _DEBUG
            /* pid#tid */
            f = ld_slprintf(f, last_f, "[%P#" LOG_TID_T_FMT "] ", log_pid, log_tid);
            c = ld_slprintf(c, last_c, "[%P#" LOG_TID_T_FMT "] ", log_pid, log_tid);
#endif
            f = ld_slprintf(f, last_f, "%V: ", &err_levels[ALevel]);
            c = ld_slprintf(c, last_c, "%V", &level_colors[ALevel]);

            ld_vslprintf(args_str, last_a, AFormat, args);

            f = ld_slprintf(f, last_f, "%s", args_str);
            c = ld_slprintf(c, last_c, "%s", args_str);

            if (AError) {
                f = ErrNo(f, last_f, AError);
                c = ErrNo(c, last_c, AError);
            }

            if (f > last_f - LINEFEED_SIZE) {
                f = last_f - LINEFEED_SIZE;
            }

            linefeed(f);
            c = ld_slprintf(c, last_c, COLOR_OFF "\n");

            wrote_stderr = false;

            CLogFile *logfile = First();
            while (logfile) {
                if (logfile->LogType() == ALogType && logfile->Level() >= ALevel && itime != m_DiskFullTime) {
                    n = write_fd(logfile->Handle(), file_str, f - file_str);

                    if (n == -1 && errno == ENOSPC) {
                        DiskFullTime(itime);
                    }

                    wrote_stderr = logfile->Handle() == STDERR_FILENO;
                }

                logfile = Next();
            }
#ifdef _DEBUG
            DebugMessage(_T("%s"), UseStdErr() ? cons_str : file_str);
#else
            if (UseStdErr() && ALevel < APP_LOG_DEBUG && !wrote_stderr) {
                (void) write_console(STDERR_FILENO, cons_str, c - cons_str);
            }
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Error(u_int ALevel, int AErrNo, LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            ErrorCore(ALevel, AErrNo, AFormat, ltError, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Error(u_int ALevel, int AErrNo, LPCSTR AFormat, va_list args) {
            ErrorCore(ALevel, AErrNo, AFormat, ltError, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Debug(u_int ALevel, LPCSTR AFormat, ...) {
            if (Level() & ALevel) {
                va_list args;
                va_start(args, AFormat);
                ErrorCore(APP_LOG_DEBUG, 0, AFormat, ltDebug, args);
                va_end(args);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Debug(u_int ALevel, LPCSTR AFormat, va_list args) {
            if (Level() & ALevel) {
                ErrorCore(APP_LOG_DEBUG, 0, AFormat, ltDebug, args);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Warning(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Error(APP_LOG_WARN, 0, AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Warning(LPCSTR AFormat, va_list args) {
            Error(APP_LOG_WARN, 0, AFormat,  args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Notice(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Error(APP_LOG_NOTICE, 0, AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Notice(LPCSTR AFormat, va_list args) {
            Error(APP_LOG_NOTICE, 0, AFormat,  args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Message(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Error(APP_LOG_INFO, 0, AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Message(LPCSTR AFormat, va_list args) {
            Error(APP_LOG_INFO, 0, AFormat,  args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Access(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Access(AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Access(LPCSTR AFormat, va_list args) {
            TCHAR szBuffer[MAX_ERROR_STR + sizeof(TCHAR) + 1] = {0};
            size_t LSize = MAX_ERROR_STR;
            CLogFile *logfile = First();
            while (logfile) {
                if (logfile->LogType() == ltAccess) {
                    chVERIFY(SUCCEEDED(StringPCchVPrintf(szBuffer, &LSize, AFormat, args)));
                    write_fd(logfile->Handle(), szBuffer, LSize);
                }
                logfile = Next();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Stream(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Stream(AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Stream(LPCSTR AFormat, va_list args) {
            ErrorCore(APP_LOG_DEBUG, 0, AFormat, ltStream, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Postgres(u_int ALevel, LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Postgres(ALevel, AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Postgres(u_int ALevel, LPCSTR AFormat, va_list args) {
            ErrorCore(ALevel, 0, AFormat, ltPostgres, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::AddLogFile(LPCSTR AFileName, u_int ALevel) {
            CLogFile *LogFile = First();
            while (LogFile && !SameText(LogFile->FileName(), AFileName)) {
                LogFile = Next();
            }

            if (LogFile == nullptr) {
                LogFile = new CLogFile(this, AFileName);
                LogFile->Level(ALevel);
                LogFile->Open();
            }

            return LogFile;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::CheckCurrentIndex() {
            if (m_CurrentIndex < -1 ) {
                m_CurrentIndex = -1;
            }
            if (m_CurrentIndex > Count() ) {
                m_CurrentIndex = Count();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::SetCurrentIndex(int AIndex) {
            m_CurrentIndex = AIndex;
            CheckCurrentIndex();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::First() {
            SetCurrentIndex(0);
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Last() {
            SetCurrentIndex(Count() - 1);
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Prior() {
            m_CurrentIndex--;
            CheckCurrentIndex();
            if (m_CurrentIndex == -1)
                return nullptr;
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Next() {
            m_CurrentIndex++;
            CheckCurrentIndex();
            if (m_CurrentIndex == Count())
                return nullptr;
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Current() {
            if ((m_CurrentIndex == -1) || (m_CurrentIndex == Count()))
                return nullptr;
            return (CLogFile *) GetItem(m_CurrentIndex);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::SetLevel(u_int AValue) {
            m_uLevel = AValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::RedirectStdErr() {
            CLogFile *log = Last();
            while (log && (log->LogType() != ltDebug || log->Handle() == STDERR_FILENO)) {
                log = Prior();
            }

            if (Assigned(log)) {
                if (dup2(log->Handle(), STDERR_FILENO) == -1) {
                    throw EOSError(errno, "dup2(STDERR) failed");
                }
            }
        }

    }
}
}

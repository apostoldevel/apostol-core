/*++

Library name:

  apostol-core

Module Name:

  Application.cpp

Notices:

  Apostol application

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Application.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include "sys/sendfile.h"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Application {

        CApplication *GApplication = nullptr;

        //--------------------------------------------------------------------------------------------------------------

        //-- CApplication ----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApplication::CApplication(int argc, char *const *argv): CProcessManager(),
            CApplicationProcess(nullptr, this, ptMain, "application"), CCustomApplication(argc, argv) {
            GApplication = this;
            m_ProcessType = ptSingle;
        }
        //--------------------------------------------------------------------------------------------------------------

        CApplication::~CApplication() {
            GApplication = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::SetProcessType(CProcessType Value) {
            if (m_ProcessType != Value) {
                m_ProcessType = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CreateLogFiles() {
            CLogFile *pLogFile;

            Log()->Clear();
#ifdef _DEBUG
            Log()->Level(APP_LOG_DEBUG_ALL & ~APP_LOG_DEBUG_EVENT);
#else
            Log()->Level(APP_LOG_DEBUG_CORE);
#endif
            u_int level;
            for (int i = 0; i < Config()->LogFiles().Count(); ++i) {
                const auto &key = Config()->LogFiles().Names(i);
                const auto &value = Config()->LogFiles().Values(key);
                level = GetLogLevelByName(key.c_str());
                if (level > APP_LOG_STDERR && level <= APP_LOG_DEBUG) {
                    pLogFile = Log()->AddLogFile(value.c_str(), level);
                    if (level == APP_LOG_DEBUG)
                        pLogFile->LogType(ltDebug);
                }
            }

            pLogFile = Log()->AddLogFile(Config()->AccessLog().c_str());
            pLogFile->LogType(ltAccess);

            pLogFile = Log()->AddLogFile(Config()->StreamLog().c_str(), level);
            pLogFile->LogType(ltStream);
#ifdef WITH_POSTGRESQL
            pLogFile = Log()->AddLogFile(Config()->PostgresLog().c_str(), level);
            pLogFile->LogType(ltPostgres);
#endif
#ifdef _DEBUG
            const auto &debug = Config()->LogFiles().Values(_T("debug"));
            if (debug.IsEmpty()) {
                pLogFile = Log()->AddLogFile(Config()->ErrorLog().c_str(), APP_LOG_DEBUG);
                pLogFile->LogType(ltDebug);
            }
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::Daemonize() {

            int  fd;

            switch (fork()) {
                case -1:
                    throw EOSError(errno, "fork() failed");

                case 0:
                    break;

                default:
                    exit(0);
            }

            if (setsid() == -1) {
                throw EOSError(errno, "setsid() failed");
            }

            Daemonized(true);

            umask(0);

            if (chdir("/") == -1) {
                throw EOSError(errno, "chdir(\"/\") failed");
            }

            fd = open("/dev/null", O_RDWR);
            if (fd == -1) {
                throw EOSError(errno, "open(\"/dev/null\") failed");
            }

            if (dup2(fd, STDIN_FILENO) == -1) {
                throw EOSError(errno, "dup2(STDIN) failed");
            }

            if (dup2(fd, STDOUT_FILENO) == -1) {
                throw EOSError(errno, "dup2(STDOUT) failed");
            }

#ifndef _DEBUG
            if (dup2(fd, STDERR_FILENO) == -1) {
                throw EOSError(errno, "dup2(STDERR) failed");
            }
#endif
            if (close(fd) == -1) {
                throw EOSError(errno, "close(\"/dev/null\") failed");
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CopyFile(const CFile &Out, const CFile &In) {
            off_t offset = Out.Offset();
            if (sendfile(Out.Handle(), In.Handle(), &offset, In.Size()) < 0)
                throw EOSError(errno, _T("sendfile \"%s\" to \"%s\" failed "), In.FileName(), Out.FileName());
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApplication::DeleteFile(const CString &FileName) {
            if (unlink(FileName.c_str()) == FILE_ERROR) {
                Log()->Error(APP_LOG_ALERT, errno, _T("could not delete file: \"%s\" error: "), FileName.c_str());
                return false;
            }
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::ChMod(const CString &File, unsigned int Mode) {
            if (chmod(File.c_str(), Mode) == -1) {
                throw EOSError(errno, _T("chmod \"%s\" failed "), File.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::MkDir(const CString &Dir) {
            if (!DirectoryExists(Dir.c_str()))
                if (!CreateDir(Dir.c_str(), 0700))
                    throw EOSError(errno, _T("mkdir \"%s\" failed "), Dir.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApplication::MkTempDir() {
            CString tmp;
            tmp.Format("/tmp/%s.XXXXXX", GApplication->Name().c_str());
            if (!mkdtemp(tmp.Data()))
                throw EOSError(errno, _T("mkdtemp \"%s\" failed "), tmp.c_str());
            return tmp;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CreateDirectories() {
            const auto& Prefix = Config()->Prefix();
            MkDir(Prefix);
            MkDir(Prefix + _T("logs/"));
            MkDir(Prefix + _T("sites/"));
            MkDir(Prefix + _T("certs/"));
            MkDir(Prefix + _T("oauth2/"));
            MkDir(Config()->ConfPrefix());
            MkDir(Config()->CachePrefix());
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApplication::ProcessesNames() {
            CString Names;
            for (int i = 0; i < CProcessManager::Count(); i++) {
                if (i > 0)
                    Names << ", ";
                Names << CProcessManager::Processes(i)->ProcessName();
            }
            return Names;
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplication::ExecNewBinary(char *const *argv, CSocketHandles *ABindings) {

            char **env, *var;
            char *p;
            uint_t i, n;

            pid_t pid;

            CExecuteContext ctx = {nullptr, nullptr, nullptr, nullptr};

            ctx.path = argv[0];
            ctx.name = "new binary process";
            ctx.argv = argv;

            n = 2;
            env = (char **) Environ();

            var = new char[sizeof(APP_VAR) + ABindings->Count() * (_INT32_LEN + 1) + 2];

            p = MemCopy(var, APP_VAR "=", sizeof(APP_VAR));

            for (i = 0; i < ABindings->Count(); i++) {
                p = ld_sprintf(p, "%ud;", ABindings->Handles(i)->Handle());
            }

            *p = '\0';

            env[n++] = var;

            env[n] = nullptr;

#if (_DEBUG)
            {
                char **e;
                for (e = env; *e; e++) {
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("env: %s"), *e);
                }
            }
#endif

            ctx.envp = (char *const *) env;

            if (!RenamePidFile(false, "rename %s to %s failed "
                                      "before executing new binary process \"%s\"")) {
                return INVALID_PID;
            }

            pid = ExecProcess(&ctx);

            if (pid == INVALID_PID) {
                RenamePidFile(true, "rename() %s back to %s failed "
                                    "after an attempt to execute new binary process \"%s\"");
            }

            return pid;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::SetNewBinary(CApplicationProcess *AProcess, CSocketHandles *ABindings) {
            AProcess->NewBinary(ExecNewBinary(m_os_argv, ABindings));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::DoBeforeStartProcess(CSignalProcess *AProcess) {

            AProcess->Pid(getpid());
            AProcess->Assign(this);

            if ((AProcess->Type() <= ptMaster) || (AProcess->Pid() != MainThreadID)) {
                MainThreadID = AProcess->Pid();
                SetSignalProcess(AProcess);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::DoAfterStartProcess(CSignalProcess *AProcess) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CreateCustomProcesses() {
            CreateProcesses(SignalProcess(), this);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::StartProcess() {

            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_START, GetProcessName(), CmdLine().c_str());

            if (m_ProcessType != ptSignaller) {
                CreateCustomProcesses();

                if (Config()->Master()) {
                    m_ProcessType = ptMaster;
                }

                if (Config()->Daemon()) {
                    Daemonize();

                    Log()->UseStdErr(false);
                    Log()->RedirectStdErr();
                };
            }

            if (m_ProcessType == ptCustom) {
                int Index = 0;
                while (Index < ProcessCount() && Processes(Index)->Type() != m_ProcessType)
                    Index++;
                if (Index == ProcessCount())
                    throw ExceptionFrm("Not found custom process.");
                Start(Processes(Index));
            } else {
                Start(CApplicationProcess::Create(SignalProcess(), this, m_ProcessType));
            }

            StopAll();

            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_STOP, GetProcessName());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::Run() {

            ParseCmdLine();

#ifdef _DEBUG
            Log()->Error(APP_LOG_STDERR, 0, "%s version: %s (%s build)", Description().c_str(), Version().c_str(), "debug");
#else
            Log()->Error(APP_LOG_STDERR, 0, "%s version: %s (%s build)", Description().c_str(), Version().c_str(), "release");
#endif
            if (Config()->Flags().show_version) {
                ShowVersionInfo();
                ExitRun(0);
            }

            if (!Config()->ConfFile().IsEmpty()) {
                Log()->Error(APP_LOG_NOTICE, 0, "Config file: %s", Config()->ConfFile().c_str());
            }

            Config()->Reload();

            if (Config()->Flags().test_config) {

                if (!FileExists(Config()->ConfFile().c_str())) {
                    Log()->Error(APP_LOG_EMERG, 0, "configuration file %s not found", Config()->ConfFile().c_str());
                    Log()->Error(APP_LOG_EMERG, 0, "configuration file %s test failed", Config()->ConfFile().c_str());
                    ExitRun(1);
                }

                if (Config()->ErrorCount() == 0) {
                    Log()->Error(APP_LOG_EMERG, 0, "configuration file %s test is successful", Config()->ConfFile().c_str());
                    ExitRun(0);
                }

                Log()->Error(APP_LOG_EMERG, 0, "configuration file %s test failed", Config()->ConfFile().c_str());
                ExitRun(1);
            }

            CreateDirectories();

            DefaultLocale.SetLocale(Config()->Locale().c_str());

            CreateLogFiles();

            StartProcess();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CApplicationProcess ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApplicationProcess::CApplicationProcess(CCustomProcess *AParent, CApplication *AApplication,
                CProcessType AType, LPCTSTR AName): CSignalProcess(AParent, AApplication, AType, AName),
                m_pApplication(AApplication) {

        }
        //--------------------------------------------------------------------------------------------------------------

        class CSignalProcess *CApplicationProcess::Create(CCustomProcess *AParent, CApplication *AApplication, CProcessType AType) {
            switch (AType) {
                case ptSingle:
                    return new CProcessSingle(AParent, AApplication);

                case ptMaster:
                    return new CProcessMaster(AParent, AApplication);

                case ptSignaller:
                    return new CProcessSignaller(AParent, AApplication);

                case ptNewBinary:
                    return new CProcessNewBinary(AParent, AApplication);

                case ptWorker:
                    return new CProcessWorker(AParent, AApplication);

                case ptHelper:
                    return new CProcessHelper(AParent, AApplication);

                default:
                    throw ExceptionFrm(_T("Unknown process type: (%d)"), AType);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::Assign(CCustomProcess *AProcess) {
            CCustomProcess::Assign(AProcess);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::BeforeRun() {
            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_START, GetProcessName(), Application()->CmdLine().c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::AfterRun() {
            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_STOP, GetProcessName());
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplicationProcess::SwapProcess(CProcessType Type, int Index, int Flag, Pointer Data) {

            CSignalProcess *pProcess;

            if (Index >= 0) {
                pProcess = Application()->Processes(Index);
            } else {
                pProcess = CApplicationProcess::Create(this, m_pApplication, Type);
                pProcess->Data(Data);
            }

            pid_t pid = fork();

            switch (pid) {

                case -1:
                    throw EOSError(errno, _T("fork() failed while spawning \"%s process\""), pProcess->GetProcessName());

                case 0:

                    m_pApplication->Start(pProcess);
                    exit(0);

                default:
                    break;
            }

            pProcess->Pid(pid);

            pProcess->Exited(false);

            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("start %s %P"), pProcess->GetProcessName(), pProcess->Pid());

            if (Index == Flag) {
                return pid;
            }

            pProcess->Respawn(Flag == PROCESS_RESPAWN || Flag == PROCESS_JUST_RESPAWN);
            pProcess->JustSpawn(Flag == PROCESS_JUST_SPAWN || Flag == PROCESS_JUST_RESPAWN);
            pProcess->Detached(Flag == PROCESS_DETACHED);

            return pid;
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplicationProcess::ExecProcess(CExecuteContext *AContext) {
            return SwapProcess(ptNewBinary, -1, PROCESS_DETACHED, AContext);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApplicationProcess::RenamePidFile(bool Back, LPCTSTR lpszErrorMessage) {
            int Result;

            TCHAR szOldPid[PATH_MAX + 1] = {0};

            LPCTSTR lpszPid;
            LPCTSTR lpszOldPid;

            lpszPid = Config()->PidFile().c_str();
            Config()->PidFile().Copy(szOldPid, PATH_MAX);
            lpszOldPid = ChangeFileExt(szOldPid, Config()->PidFile().c_str(), APP_OLDPID_EXT);

            if (Back) {
                Result = ::rename(lpszOldPid, lpszPid);
                if (Result == INVALID_FILE)
                    Log()->Error(APP_LOG_ALERT, errno, lpszErrorMessage, lpszOldPid, lpszPid, Application()->argv()[0].c_str());
            } else {
                Result = ::rename(lpszPid, lpszOldPid);
                if (Result == INVALID_FILE)
                    Log()->Error(APP_LOG_ALERT, errno, lpszErrorMessage, lpszPid, lpszOldPid, Application()->argv()[0].c_str());
            }

            return (Result != INVALID_FILE);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::CreatePidFile() {

            size_t len;
            char pid[_INT64_LEN + 2];
            int create = Config()->Flags().test_config ? FILE_CREATE_OR_OPEN : FILE_TRUNCATE;

            if (Daemonized()) {

                CFile File(Config()->PidFile().c_str(), FILE_RDWR | create);

#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                File.OnFilerError([this](auto && Sender, auto && Error, auto && lpFormat, auto && args) { OnFilerError(Sender, Error, lpFormat, args); });
#else
                File.OnFilerError(std::bind(&CApplicationProcess::OnFilerError, this, _1, _2, _3, _4));
#endif
                File.Open();

                if (!Config()->Flags().test_config) {
                    len = ld_snprintf(pid, _INT64_LEN + 2, "%P%N", Pid()) - pid;
                    File.Write(pid, len, 0);
                }

                File.Close();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::DeletePidFile() {
            if (Daemonized()) {
                LPCTSTR lpszPid = Config()->PidFile().c_str();

                if (unlink(lpszPid) == FILE_ERROR) {
                    Log()->Error(APP_LOG_ALERT, errno, _T("could not delete pid file: \"%s\" error: "), lpszPid);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::OnFilerError(Pointer Sender, int Error, LPCTSTR lpFormat, va_list args) {
            Log()->Error(APP_LOG_ALERT, Error, lpFormat, args);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessSingle --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CProcessSingle::CProcessSingle(CCustomProcess *AParent, CApplication *AApplication):
                inherited(AParent, AApplication, ptSingle, "single"), CModuleProcess() {

            InitializeServer(AApplication->Title());
#ifdef WITH_POSTGRESQL
            InitializePQClients(AApplication->Title());
#endif
            if (Config()->Helper()) {
                CreateHelpers(this);
            } else {
                CreateWorkers(this);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::Reload() {
#ifdef WITH_POSTGRESQL
            PQClientsStop();
#endif
            ServerStop();

            CServerProcess::Reload();

            SetLimitNoFile(Config()->LimitNoFile());

            ServerStart();
#ifdef WITH_POSTGRESQL
            if (Config()->Helper()) {
                PQClientStart("helper");
            } else {
                PQClientStart("worker");
            }
#endif
            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::BeforeRun() {
            Application()->Header(Application()->Name() + ": single process " + Application()->CmdLine());

            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            SetLimitNoFile(Config()->LimitNoFile());

            ServerStart();
#ifdef WITH_POSTGRESQL
            if (Config()->Helper()) {
                PQClientStart("helper");
            } else {
                PQClientStart("worker");
            }
#endif
            Initialization();

            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::AfterRun() {
            Finalization();
#ifdef WITH_POSTGRESQL
            PQClientsStop();
#endif
            ServerStop();
            ServerShutDown();

            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::DoExit() {
            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("exiting single process"));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::Run() {

            while (!(sig_terminate || sig_quit)) {

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("single cycle"));

                try {
                    Server().Wait();
                } catch (std::exception &e) {
                    Log()->Error(APP_LOG_ERR, 0, "%s", e.what());
                }

                if (sig_reconfigure) {
                    sig_reconfigure = 0;
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reconfiguring"));

                    Reload();
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reopening logs"));
                    Log()->RedirectStdErr();
                }
            }

            DoExit();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessMaster --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CProcessMaster::CProcessMaster(CCustomProcess *AParent, CApplication *AApplication) :
                inherited(AParent, AApplication, ptMaster, "master"), CModuleProcess() {

            InitializeServer(AApplication->Title());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::BeforeRun() {
            Application()->Header(Application()->Name() + ": master process " + Application()->CmdLine());

            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::AfterRun() {
            ServerShutDown();
            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::StartProcess(CProcessType Type, int Index, int Flag) {
            switch (Type) {
                case ptWorker:
                    for (int i = 0; i < Config()->Workers(); ++i) {
                        SwapProcess(Type, Index, Flag);
                    }
                    break;
                case ptHelper:
                    if (Config()->Helper()) {
                        SwapProcess(Type, Index, Flag);
                    }
                    break;
                default:
                    SwapProcess(Type, Index, Flag);
                    break;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::StartProcesses(int Flag) {
            StartProcess(ptWorker, -1, Flag);
            StartProcess(ptHelper, -1, Flag);
            StartCustomProcesses(Flag);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::StartCustomProcesses(int Flag) {
            CCustomProcess *pProcess;

            int i = 0;
            if (Flag == PROCESS_JUST_RESPAWN) {
                i = Application()->ProcessCount() - 1;
                Application()->CreateCustomProcesses();
            }

            for (; i < Application()->ProcessCount(); ++i) {
                pProcess = Application()->Processes(i);
                if (pProcess->Type() == ptCustom)
                    StartProcess(ptCustom, i, Flag);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::SignalToProcess(CProcessType Type, int SigNo) {
            int err;
            CCustomProcess *pProcess;

            for (int i = 0; i < Application()->ProcessCount(); ++i) {
                pProcess = Application()->Processes(i);

                if (pProcess->Type() != Type)
                    continue;

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("signal child: %i %P e:%d t:%d d:%d r:%d j:%d"),
                              i,
                              pProcess->Pid(),
                              pProcess->Exiting() ? 1 : 0,
                              pProcess->Exited() ? 1 : 0,
                              pProcess->Detached() ? 1 : 0,
                              pProcess->Respawn() ? 1 : 0,
                              pProcess->JustSpawn() ? 1 : 0
                );

                if (pProcess->Detached()) {
                    continue;
                }

                if (pProcess->JustSpawn()) {
                    pProcess->JustSpawn(false);
                    continue;
                }

                if (pProcess->Exiting() && (SigNo == signal_value(SIG_SHUTDOWN_SIGNAL))) {
                    continue;
                }

                Log()->Debug(APP_LOG_DEBUG_CORE, "kill (%P, %d)", pProcess->Pid(), SigNo);

                if (kill(pProcess->Pid(), SigNo) == -1) {
                    err = errno;
                    Log()->Error(APP_LOG_ALERT, err, "kill(%P, %d) failed", pProcess->Pid(), SigNo);

                    if (err == ESRCH) {
                        pProcess->Exited(true);
                        pProcess->Exiting(false);
                        sig_reap = 1;
                    }

                    continue;
                }

                if (SigNo != signal_value(SIG_REOPEN_SIGNAL)) {
                    pProcess->Exiting(true);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::SignalToProcesses(int SigNo) {
            SignalToProcess(ptCustom, SigNo);
            SignalToProcess(ptHelper, SigNo);
            SignalToProcess(ptWorker, SigNo);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::DoExit() {
            DeletePidFile();

            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("exiting master process"));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CProcessMaster::ReapChildren() {

            bool live = false;
            CSignalProcess *pProcess;

            for (int i = 0; i < Application()->ProcessCount(); ++i) {

                pProcess = Application()->Processes(i);

                if (pProcess->Type() == ptMain)
                    continue;

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reap child: %i %P:\texiting: %d\texited: %d\tdetached: %d\trespawn: %d\tjustSpawn: %d\tname: %s"),
                              i,
                              pProcess->Pid(),
                              pProcess->Exiting() ? 1 : 0,
                              pProcess->Exited() ? 1 : 0,
                              pProcess->Detached() ? 1 : 0,
                              pProcess->Respawn() ? 1 : 0,
                              pProcess->JustSpawn() ? 1 : 0,
                              pProcess->GetProcessName()
                );

                if (pProcess->Exited()) {

                    if (pProcess->Respawn() && !pProcess->Exiting() && !(sig_terminate || sig_quit)) {

                        if (pProcess->Type() >= ptWorker) {
                            try {
                                SwapProcess(pProcess->Type(), i, i);
                            } catch (std::exception &e) {
                                Log()->Error(APP_LOG_ALERT, 0, "could not respawn %s", pProcess->GetProcessName());
                                continue;
                            }
                        }

                        live = true;
                        continue;
                    }

                    if (pProcess->Pid() == NewBinary()) {

                        RenamePidFile(true, "rename() %s back to %s failed after the new binary process \"%s\" exited");

                        NewBinary(0);

                        if (sig_noaccepting) {
                            sig_restart = 1;
                            sig_noaccepting = 0;
                        }
                    }

                    Application()->DeleteProcess(i);

                } else if (pProcess->Exiting() || !pProcess->Detached()) {
                    live = true;
                }

            }

            return live;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::Run() {

            int sigio;
            sigset_t set, wset;
            struct itimerval itv = {};
            bool live;
            uint_t delay;

            CreatePidFile();

            SigProcMask(SIG_BLOCK, SigAddSet(&set), &wset);

            StartProcesses(PROCESS_RESPAWN);

            NewBinary(0);

            delay = 0;
            sigio = 0;

            CSignalProcess *pProcess;
            for (int i = 0; i < Application()->ProcessCount(); ++i) {
                pProcess = Application()->Processes(i);

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("process: %i %P:%P\texiting: %d\texited: %d\tdetached: %d\trespawn: %d\tjustSpawn: %d\tname: %s"),
                              i,
                              Pid(),
                              pProcess->Pid(),
                              pProcess->Exiting() ? 1 : 0,
                              pProcess->Exited() ? 1 : 0,
                              pProcess->Detached() ? 1 : 0,
                              pProcess->Respawn() ? 1 : 0,
                              pProcess->JustSpawn() ? 1 : 0,
                              pProcess->GetProcessName()
                );
            }

            live = true;
            while (!(live && (sig_terminate || sig_quit))) {

                if (delay) {
                    if (sig_sigalrm) {
                        sigio = 0;
                        delay *= 2;
                        sig_sigalrm = 0;
                    }

                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("termination cycle: %M"), delay);

                    itv.it_interval.tv_sec = 0;
                    itv.it_interval.tv_usec = 0;
                    itv.it_value.tv_sec = delay / 1000;
                    itv.it_value.tv_usec = (delay % 1000 ) * 1000;

                    if (setitimer(ITIMER_REAL, &itv, nullptr) == -1) {
                        Log()->Error(APP_LOG_ALERT, errno, _T("setitimer() failed"));
                    }
                }

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("sigsuspend"));

                sigsuspend(&wset);

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("wake up, sigio %i"), sigio);

                if (sig_reap) {
                    sig_reap = 0;
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reap children"));

                    live = ReapChildren();
                }

                if (sig_terminate) {
                    if (delay == 0) {
                        delay = 50;
                    }

                    if (sigio) {
                        sigio--;
                        continue;
                    }

                    sigio = (int) Config()->Workers();

                    if (delay > 1000) {
                        SignalToProcesses(SIGKILL);
                    } else {
                        SignalToProcesses(signal_value(SIG_TERMINATE_SIGNAL));
                    }

                    continue;
                }

                if (sig_quit) {
                    SignalToProcesses(signal_value(SIG_SHUTDOWN_SIGNAL));
                    continue;
                }

                if (sig_reconfigure) {
                    sig_reconfigure = 0;

                    if (NewBinary() > 0) {
                        StartProcesses(PROCESS_RESPAWN);
                        sig_noaccepting = 0;

                        continue;
                    }

                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reconfiguring"));

                    Reload();

                    StartProcesses(PROCESS_JUST_RESPAWN);

                    /* allow new processes to start */
                    usleep(100 * 1000);

                    live = true;

                    SignalToProcesses(signal_value(SIG_SHUTDOWN_SIGNAL));
                }

                if (sig_restart) {
                    sig_restart = 0;
                    StartProcesses(PROCESS_RESPAWN);
                    live = true;
                }

                if (sig_reopen) {
                    sig_reopen = 0;

                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reopening logs"));

                    Application()->CreateLogFiles();

                    SignalToProcesses(signal_value(SIG_REOPEN_SIGNAL));
                }

                if (sig_change_binary) {
                    sig_change_binary = 0;
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("changing binary"));
                    Application()->SetNewBinary(this, Server().Bindings());
                }

                if (sig_noaccept) {
                    sig_noaccept = 0;
                    sig_noaccepting = 1;
                    SignalToProcesses(signal_value(SIG_SHUTDOWN_SIGNAL));
                }
            }

            DoExit();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessSignaller -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessSignaller::SignalToProcess(pid_t pid) {

            CSignal *Signal;
            LPCTSTR lpszSignal = Config()->Signal().c_str();

            for (int i = 0; i < SignalsCount(); ++i) {
                Signal = Signals(i);
                if (SameText(lpszSignal, Signal->Name())) {
                    if (kill(pid, Signal->SigNo()) == -1) {
                        Log()->Error(APP_LOG_ALERT, errno, _T("kill(%P, %d) failed"), pid, Signal->SigNo());
                    }
                    break;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSignaller::Run() {

            ssize_t       n;
            pid_t         pid;
            char          buf[_INT64_LEN + 2] = {0};

            LPCTSTR lpszPid = Config()->PidFile().c_str();

            CFile File(lpszPid, FILE_RDONLY | FILE_OPEN);

#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            File.OnFilerError([this](auto && Sender, auto && Error, auto && lpFormat, auto && args) { OnFilerError(Sender, Error, lpFormat, args); });
#else
            File.OnFilerError(std::bind(&CApplicationProcess::OnFilerError, this, _1, _2, _3, _4));
#endif
            File.Open();

            n = File.Read(buf, _INT64_LEN + 2, 0);

            File.Close();

            while (n-- && (buf[n] == '\r' || buf[n] == '\n')) { buf[n] = '\0'; }

            pid = (pid_t) strtol(buf, nullptr, 0);

            if (pid == (pid_t) 0) {
                throw ExceptionFrm(_T("invalid PID number \"%*s\" in \"%s\""), n, buf, lpszPid);
            }

            SignalToProcess(pid);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessNewBinary -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessNewBinary::Run() {
            auto LContext = (CExecuteContext *) Data();
            ExecuteProcess(LContext);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessWorker --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CProcessWorker::CProcessWorker(CCustomProcess *AParent, CApplication *AApplication) :
                inherited(AParent, AApplication, ptWorker, "worker"), CWorkerProcess() {

            auto pParent = dynamic_cast<CServerProcess *>(AParent);
            if (pParent != nullptr) {
                Server() = pParent->Server();
                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("worker process: http server assigned by parent"));
            } else {
                InitializeServer(AApplication->Title());
            }
#ifdef WITH_POSTGRESQL
            InitializePQClients(AApplication->Title());
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::BeforeRun() {
            sigset_t set;

            Application()->Header(Application()->Name() + ": worker process (" + CModuleProcess::ModulesNames() + ")");

            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            SetLimitNoFile(Config()->LimitNoFile());

            ServerStart();
#ifdef WITH_POSTGRESQL
            PQClientStart("worker");
#endif
            Initialization();

            SetUser(Config()->User(), Config()->Group());

            SigProcMask(SIG_UNBLOCK, SigAddSet(&set));

            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::AfterRun() {
            Finalization();
#ifdef WITH_POSTGRESQL
            PQClientsStop();
            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("worker process: postgres clients stopped"));
#endif
            ServerStop();
            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("worker process: http server stopped"));

            ServerShutDown();
            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("worker process: http server shutdown"));

            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::DoExit() {
            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("exiting worker process"));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::Run() {
            sigset_t set;

            while (!sig_exiting) {

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("worker cycle"));

                try {
                    Server().Wait(SigAddSet(&set));
                } catch (std::exception &e) {
                    Log()->Error(APP_LOG_ERR, 0, "%s", e.what());
                }

                if (sig_terminate || sig_quit) {
                    if (sig_quit) {
                        sig_quit = 0;
                        Log()->Debug(APP_LOG_DEBUG_EVENT, _T("gracefully shutting down"));
                        Application()->Header(_T("worker process is shutting down"));
                    }

                    DoExit();

                    if (!sig_exiting) {
                        sig_exiting = 1;
                    }
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reopening logs"));
                    //ReopenFiles(-1);
                }
            }

            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("stop worker process"));
        }
        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessHelper --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CProcessHelper::CProcessHelper(CCustomProcess *AParent, CApplication *AApplication) :
                inherited(AParent, AApplication, ptHelper, "helper"), CHelperProcess() {

            auto pParent = dynamic_cast<CServerProcess *>(AParent);
            if (pParent != nullptr) {
                Server() = pParent->Server();
                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("helper process: http server assigned by parent"));
            } else {
                InitializeServer(AApplication->Title());
            }
#ifdef WITH_POSTGRESQL
            InitializePQClients(AApplication->Title());
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::DoExit() {
            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("exiting helper process"));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::BeforeRun() {
            sigset_t set;

            Application()->Header(Application()->Name() + ": helper process (" + CModuleProcess::ModulesNames() + ")");

            Log()->Debug(APP_LOG_DEBUG_CORE, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            SetLimitNoFile(Config()->LimitNoFile());
#ifdef WITH_POSTGRESQL
            PQClientStart("helper");
#endif
            Initialization();

            SetUser(Config()->User(), Config()->Group());

            SigProcMask(SIG_UNBLOCK, SigAddSet(&set));

            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::AfterRun() {
            Finalization();
#ifdef WITH_POSTGRESQL
            PQClientsStop();
#endif
            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::Run() {
            while (!sig_exiting) {

                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("helper cycle"));

                try
                {
                    Server().Wait();
                }
                catch (Delphi::Exception::Exception &E)
                {
                    Log()->Error(APP_LOG_ERR, 0, "%s", E.what());
                }

                if (sig_terminate || sig_quit) {
                    if (sig_quit) {
                        sig_quit = 0;
                        Log()->Debug(APP_LOG_DEBUG_EVENT, _T("gracefully shutting down"));
                        Application()->Header(_T("helper process is shutting down"));
                    }

                    DoExit();

                    if (!sig_exiting) {
                        sig_exiting = 1;
                    }
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("reopening logs"));
                    //ReopenFiles(-1);
                }
            }

            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("stop helper process"));
        }
        //--------------------------------------------------------------------------------------------------------------
    }
}

}

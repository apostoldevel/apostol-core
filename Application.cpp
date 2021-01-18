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

        void CApplication::CreateLogFile() {
            CLogFile *pLogFile;

            Log()->Level(APP_LOG_DEBUG_CORE);

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

            pLogFile = Log()->AddLogFile(Config()->AccessLog().c_str(), APP_LOG_STDERR);
            pLogFile->LogType(ltAccess);

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

        void CApplication::MkDir(const CString &Dir) {
            if (!DirectoryExists(Dir.c_str()))
                if (!CreateDir(Dir.c_str(), 0700))
                    throw EOSError(errno, _T("mkdir \"%s\" failed "), Dir.c_str());
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
                    log_debug1(APP_LOG_DEBUG_CORE, Log(), 0, "env: %s", *e);
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

        void CApplication::StartProcess() {

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), CmdLine().c_str());

            if (m_ProcessType != ptSignaller) {

                if (Config()->Master()) {
                    m_ProcessType = ptMaster;
                }

                if (Config()->Daemon()) {
                    Daemonize();

                    Log()->UseStdErr(false);
                    Log()->RedirectStdErr();
                } else {
#ifdef _DEBUG
                    Log()->RedirectStdErr();
#endif
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

            Log()->Debug(0, MSG_PROCESS_STOP, GetProcessName());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::Run() {

            ParseCmdLine();

            if (Config()->Flags().show_version) {
                ShowVersionInfo();
                ExitRun(0);
            }

            Config()->Reload();

            if (Config()->Flags().test_config) {

                if (!FileExists(Config()->ConfFile().c_str())) {
                    Log()->Error(APP_LOG_STDERR, 0, "configuration file %s not found", Config()->ConfFile().c_str());
                    Log()->Error(APP_LOG_STDERR, 0, "configuration file %s test failed", Config()->ConfFile().c_str());
                    ExitRun(1);
                }

                if (Config()->ErrorCount() == 0) {
                    Log()->Error(APP_LOG_STDERR, 0, "configuration file %s test is successful", Config()->ConfFile().c_str());
                    ExitRun(0);
                }

                Log()->Error(APP_LOG_STDERR, 0, "configuration file %s test failed", Config()->ConfFile().c_str());
                ExitRun(1);
            }

            CreateDirectories();

            DefaultLocale.SetLocale(Config()->Locale().c_str());

            CreateLogFile();

#ifdef _DEBUG
            Log()->Error(APP_LOG_INFO, 0, "%s version: %s (%s build)", Description().c_str(), Version().c_str(), "debug");
#else
            Log()->Error(APP_LOG_INFO, 0, "%s version: %s (%s build)", Description().c_str(), Version().c_str(), "release");
#endif
            Log()->Error(APP_LOG_INFO, 0, "Config file: %s", Config()->ConfFile().c_str());

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
            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->CmdLine().c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::AfterRun() {
            Log()->Debug(0, MSG_PROCESS_STOP, GetProcessName());
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplicationProcess::SwapProcess(CProcessType Type, int Flag, Pointer Data) {

            CSignalProcess *LProcess;

            if (Flag >= 0) {
                LProcess = Application()->Processes(Flag);
            } else {
                LProcess = CApplicationProcess::Create(this, m_pApplication, Type);
                LProcess->Data(Data);
            }

            pid_t pid = fork();

            switch (pid) {

                case -1:
                    throw EOSError(errno, _T("fork() failed while spawning \"%s process\""), LProcess->GetProcessName());

                case 0:

                    m_pApplication->Start(LProcess);
                    exit(0);

                default:
                    break;
            }

            LProcess->Pid(pid);

            LProcess->Exited(false);

            Log()->Error(APP_LOG_NOTICE, 0, _T("start %s %P"), LProcess->GetProcessName(), LProcess->Pid());

            if (Flag >= 0) {
                return pid;
            }

            LProcess->Exiting(false);

            LProcess->Respawn(Flag == PROCESS_RESPAWN || Flag == PROCESS_JUST_RESPAWN);
            LProcess->JustSpawn(Flag == PROCESS_JUST_SPAWN || Flag == PROCESS_JUST_RESPAWN);
            LProcess->Detached(Flag == PROCESS_DETACHED);

            return pid;
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplicationProcess::ExecProcess(CExecuteContext *AContext) {
            return SwapProcess(ptNewBinary, PROCESS_DETACHED, AContext);
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
                File.setOnFilerError([this](auto && Sender, auto && Error, auto && lpFormat, auto && args) { OnFilerError(Sender, Error, lpFormat, args); });
#else
                File.setOnFilerError(std::bind(&CApplicationProcess::OnFilerError, this, _1, _2, _3, _4));
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

            if (Config()->Helper()) {
                CreateHelpers(this);
            } else {
                CreateWorkers(this);
            }

            InitializeServer(AApplication->Title());
#ifdef WITH_POSTGRESQL
            InitializePQServer(AApplication->Title());
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::Reload() {
#ifdef WITH_POSTGRESQL
            PQServerStop();
#endif
            ServerStop();

            CServerProcess::Reload();

            SetLimitNoFile(Config()->LimitNoFile());

            ServerStart();
#ifdef WITH_POSTGRESQL
            if (Config()->Helper()) {
                PQServerStart("helper");
            } else {
                PQServerStart("worker");
            }
#endif
            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::BeforeRun() {
            Application()->Header(Application()->Name() + ": single process " + Application()->CmdLine());

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            SetLimitNoFile(Config()->LimitNoFile());

            ServerStart();
#ifdef WITH_POSTGRESQL
            if (Config()->Helper()) {
                PQServerStart("helper");
            } else {
                PQServerStart("worker");
            }
#endif
            Initialization();

            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::AfterRun() {
            Finalization();
#ifdef WITH_POSTGRESQL
            PQServerStop();
#endif
            ServerStop();

            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::DoExit() {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::Run() {

            while (!(sig_terminate || sig_quit)) {

                log_debug0(APP_LOG_DEBUG_EVENT, Log(), 0, "single cycle");

                try
                {
                    Server().Wait();
                }
                catch (Delphi::Exception::Exception &E)
                {
                    Log()->Error(APP_LOG_ERR, 0, "%s", E.what());
                }

                DoExit();

                if (sig_reconfigure) {
                    sig_reconfigure = 0;
                    Log()->Error(APP_LOG_NOTICE, 0, "reconfiguring");

                    Reload();
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Error(APP_LOG_NOTICE, 0, "reopening logs");
                    Log()->RedirectStdErr();
                }
            }

            Log()->Error(APP_LOG_NOTICE, 0, "exiting single process");
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

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::AfterRun() {
            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::StartProcess(CProcessType Type, int Flag) {
            switch (Type) {
                case ptWorker:
                    for (int i = 0; i < Config()->Workers(); ++i) {
                        SwapProcess(Type, Flag);
                    }
                    break;
                case ptHelper:
                    if (Config()->Helper()) {
                        SwapProcess(Type, Flag);
                    }
                    break;
                default:
                    SwapProcess(Type, Flag);
                    break;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::StartProcesses(int Flag) {
            StartProcess(ptWorker, Flag);
            StartProcess(ptHelper, Flag);
            StartCustomProcesses();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::StartCustomProcesses() {
            CCustomProcess *LProcess;
            for (int i = 0; i < Application()->ProcessCount(); ++i) {
                LProcess = Application()->Processes(i);
                if (LProcess->Type() == ptCustom)
                    StartProcess(ptCustom, i);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::SignalToProcess(CProcessType Type, int SigNo) {
            int err;
            CCustomProcess *LProcess;

            for (int i = 0; i < Application()->ProcessCount(); ++i) {
                LProcess = Application()->Processes(i);

                if (LProcess->Type() != Type)
                    continue;

                log_debug7(APP_LOG_DEBUG_EVENT, Log(), 0,
                           "signal child: %i %P e:%d t:%d d:%d r:%d j:%d",
                           i,
                           LProcess->Pid(),
                           LProcess->Exiting() ? 1 : 0,
                           LProcess->Exited() ? 1 : 0,
                           LProcess->Detached() ? 1 : 0,
                           LProcess->Respawn() ? 1 : 0,
                           LProcess->JustSpawn() ? 1 : 0);

                if (LProcess->Detached()) {
                    continue;
                }

                if (LProcess->JustSpawn()) {
                    LProcess->JustSpawn(false);
                    continue;
                }

                if (LProcess->Exiting() && (SigNo == signal_value(SIG_SHUTDOWN_SIGNAL))) {
                    continue;
                }

                log_debug2(APP_LOG_DEBUG_CORE, Log(), 0, "kill (%P, %d)", LProcess->Pid(), SigNo);

                if (kill(LProcess->Pid(), SigNo) == -1) {
                    err = errno;
                    Log()->Error(APP_LOG_ALERT, err, "kill(%P, %d) failed", LProcess->Pid(), SigNo);

                    if (err == ESRCH) {
                        LProcess->Exited(true);
                        LProcess->Exiting(false);
                        sig_reap = 1;
                    }

                    continue;
                }

                if (SigNo != signal_value(SIG_REOPEN_SIGNAL)) {
                    LProcess->Exiting(true);
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

            Log()->Error(APP_LOG_NOTICE, 0, _T("exiting master process"));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CProcessMaster::ReapChildren() {

            bool live = false;
            CSignalProcess *LProcess;

            for (int i = 0; i < Application()->ProcessCount(); ++i) {

                LProcess = Application()->Processes(i);

                if (LProcess->Type() == ptMain)
                    continue;

                log_debug7(APP_LOG_DEBUG_EVENT, Log(), 0,
                           "reap child: %i %P e:%d t:%d d:%d r:%d j:%d",
                           i,
                           LProcess->Pid(),
                           LProcess->Exiting() ? 1 : 0,
                           LProcess->Exited() ? 1 : 0,
                           LProcess->Detached() ? 1 : 0,
                           LProcess->Respawn() ? 1 : 0,
                           LProcess->JustSpawn() ? 1 : 0);

                if (LProcess->Exited()) {

                    if (LProcess->Respawn() && !LProcess->Exiting() && !(sig_terminate || sig_quit)) {

                        if (LProcess->Type() >= ptWorker) {
                            if (SwapProcess(LProcess->Type(), i) == -1) {
                                Log()->Error(APP_LOG_ALERT, 0, "could not respawn %s", LProcess->GetProcessName());
                                continue;
                            }
                        }

                        live = true;

                        continue;
                    }

                    if (LProcess->Pid() == NewBinary()) {

                        RenamePidFile(true, "rename() %s back to %s failed after the new binary process \"%s\" exited");

                        NewBinary(0);

                        if (sig_noaccepting) {
                            sig_restart = 1;
                            sig_noaccepting = 0;
                        }
                    }

                    Application()->DeleteProcess(i);

                } else if (LProcess->Exiting() || !LProcess->Detached()) {
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

            CSignalProcess *LProcess;
            for (int i = 0; i < Application()->ProcessCount(); ++i) {
                LProcess = Application()->Processes(i);

                log_debug9(APP_LOG_DEBUG_EVENT, Log(), 0,
                           "process (%s)\t: %P - %i %P e:%d t:%d d:%d r:%d j:%d",
                           LProcess->GetProcessName(),
                           Pid(),
                           i,
                           LProcess->Pid(),
                           LProcess->Exiting() ? 1 : 0,
                           LProcess->Exited() ? 1 : 0,
                           LProcess->Detached() ? 1 : 0,
                           LProcess->Respawn() ? 1 : 0,
                           LProcess->JustSpawn() ? 1 : 0);
            }

            live = true;
            while (!(live && (sig_terminate || sig_quit))) {

                if (delay) {
                    if (sig_sigalrm) {
                        sigio = 0;
                        delay *= 2;
                        sig_sigalrm = 0;
                    }

                    log_debug1(APP_LOG_DEBUG_EVENT, Log(), 0, "termination cycle: %M", delay);

                    itv.it_interval.tv_sec = 0;
                    itv.it_interval.tv_usec = 0;
                    itv.it_value.tv_sec = delay / 1000;
                    itv.it_value.tv_usec = (delay % 1000 ) * 1000;

                    if (setitimer(ITIMER_REAL, &itv, nullptr) == -1) {
                        Log()->Error(APP_LOG_ALERT, errno, "setitimer() failed");
                    }
                }

                log_debug0(APP_LOG_DEBUG_EVENT, Log(), 0, "sigsuspend");

                sigsuspend(&wset);

                log_debug1(APP_LOG_DEBUG_EVENT, Log(), 0, "wake up, sigio %i", sigio);

                if (sig_reap) {
                    sig_reap = 0;
                    log_debug0(APP_LOG_DEBUG_EVENT, Log(), 0, "reap children");

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

                    sigio = Config()->Workers();

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

                    Log()->Error(APP_LOG_NOTICE, 0, "reconfiguring");

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

                    Log()->Error(APP_LOG_NOTICE, 0, "reopening logs");
                    // TODO: reopen files;

                    SignalToProcesses(signal_value(SIG_REOPEN_SIGNAL));
                }

                if (sig_change_binary) {
                    sig_change_binary = 0;
                    Log()->Error(APP_LOG_NOTICE, 0, "changing binary");
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
            File.setOnFilerError([this](auto && Sender, auto && Error, auto && lpFormat, auto && args) { OnFilerError(Sender, Error, lpFormat, args); });
#else
            File.setOnFilerError(std::bind(&CApplicationProcess::OnFilerError, this, _1, _2, _3, _4));
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
                log_debug0(APP_LOG_DEBUG_EVENT, Log(), 0, "worker process: http server assigned by parent");
            } else {
                InitializeServer(AApplication->Title());
            }
#ifdef WITH_POSTGRESQL
            InitializePQServer(AApplication->Title());
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::BeforeRun() {
            sigset_t set;

            Application()->Header(Application()->Name() + ": worker process (" + CModuleProcess::ModulesNames() + ")");

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            SetLimitNoFile(Config()->LimitNoFile());

            ServerStart();
#ifdef WITH_POSTGRESQL
            PQServerStart("worker");
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
            PQServerStop();
            Log()->Error(APP_LOG_NOTICE, 0, "worker process: postgres server stopped");
#endif
            ServerStop();
            Log()->Error(APP_LOG_NOTICE, 0, "worker process: http server stopped");

            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::DoExit() {
            Log()->Error(APP_LOG_NOTICE, 0, "exiting worker process");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::Run() {

            while (!sig_exiting) {

                log_debug0(APP_LOG_DEBUG_EVENT, Log(), 0, "worker cycle");

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
                        Log()->Error(APP_LOG_NOTICE, 0, "gracefully shutting down");
                        Application()->Header("worker process is shutting down");
                    }

                    DoExit();

                    if (!sig_exiting) {
                        sig_exiting = 1;
                    }
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Error(APP_LOG_NOTICE, 0, "reopening logs");
                    //ReopenFiles(-1);
                }
            }

            Log()->Error(APP_LOG_NOTICE, 0, "stop worker process");
        }
        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessHelper --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CProcessHelper::CProcessHelper(CCustomProcess *AParent, CApplication *AApplication) :
                inherited(AParent, AApplication, ptHelper, "helper"), CHelperProcess() {

            auto pParent = dynamic_cast<CServerProcess *>(AParent);
            if (pParent != nullptr) {
                Server() = pParent->Server();
                log_debug0(APP_LOG_DEBUG_EVENT, Log(), 0, "helper process: http server assigned by parent");
            } else {
                InitializeServer(AApplication->Title());
            }
#ifdef WITH_POSTGRESQL
            InitializePQServer(AApplication->Title());
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::DoExit() {
            Log()->Error(APP_LOG_NOTICE, 0, "exiting helper process");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::BeforeRun() {
            sigset_t set;

            Application()->Header(Application()->Name() + ": helper process (" + CModuleProcess::ModulesNames() + ")");

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            SetLimitNoFile(Config()->LimitNoFile());
#ifdef WITH_POSTGRESQL
            PQServerStart("helper");
#endif
            Initialization();

            SigProcMask(SIG_UNBLOCK, SigAddSet(&set));

            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::AfterRun() {
            Finalization();
#ifdef WITH_POSTGRESQL
            PQServerStop();
#endif
            //ServerStop();
            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::Run() {
            while (!sig_exiting) {

                log_debug0(APP_LOG_DEBUG_EVENT, Log(), 0, "helper cycle");

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
                        Log()->Error(APP_LOG_NOTICE, 0, "gracefully shutting down");
                        Application()->Header("helper process is shutting down");
                    }

                    DoExit();

                    if (!sig_exiting) {
                        sig_exiting = 1;
                    }
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Error(APP_LOG_NOTICE, 0, "reopening logs");
                    //ReopenFiles(-1);
                }
            }

            Log()->Error(APP_LOG_NOTICE, 0, "stop helper process");
        }
        //--------------------------------------------------------------------------------------------------------------
    }
}

}

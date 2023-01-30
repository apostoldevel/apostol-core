/*++

Library name:

  apostol-core

Module Name:

  Process.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Process.hpp"
//----------------------------------------------------------------------------------------------------------------------

#define BT_BUF_SIZE 255
//----------------------------------------------------------------------------------------------------------------------

void signal_error(int signo, siginfo_t *siginfo, void *ucontext) {
    void*       addr;
    void*       trace[BT_BUF_SIZE];
    int         i;
    int         count;
    char      **msg;

    GLog->Error(APP_LOG_CRIT, 0, "-----BEGIN BACKTRACE LOG-----");
    GLog->Error(APP_LOG_CRIT, 0, "Signal: %d (%s)", signo, strsignal(signo));
    GLog->Error(APP_LOG_CRIT, 0, "Addr  : %p", siginfo->si_addr);

#ifdef __x86_64__

#if __WORDSIZE == 64
    addr = (void*)((ucontext_t*)ucontext)->uc_mcontext.gregs[REG_RIP];
#else
    addr = (void*)((ucontext_t*)ucontext)->uc_mcontext.gregs[REG_EIP];
#endif
    GLog->Error(APP_LOG_CRIT, 0, "addr  : %p", addr);

    count = backtrace(trace, BT_BUF_SIZE);

    GLog->Error(APP_LOG_CRIT, 0, "Count : %d", count);

    msg = backtrace_symbols(trace, count);
    if (msg) {
        for (i = 0; i < count; ++i) {
            GLog->Error(APP_LOG_CRIT, 0, "%s", msg[i]);
        }

        free(msg);
    }
    GLog->Error(APP_LOG_CRIT, 0, "-----END BACKTRACE LOG-----");
#endif
    exit(3);
}
//----------------------------------------------------------------------------------------------------------------------

void signal_handler(int signo, siginfo_t *siginfo, void *ucontext) {
    try
    {
        GApplication->SignalProcess()->SignalHandler(signo, siginfo, ucontext);
    }
    catch (Delphi::Exception::Exception &E)
    {
        log_failure(E.what());
    }
    catch (std::exception &e)
    {
        log_failure(e.what());
    }
    catch (...)
    {
        log_failure("Unknown fatality error...");
    }
}
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Process {

        //--------------------------------------------------------------------------------------------------------------

        //-- CSignalProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSignalProcess::CSignalProcess(CCustomProcess *AParent, CProcessManager *AManager, CProcessType AType,
                LPCTSTR AName): CCustomProcess(AParent, AType, AName), CSignals(), CCollectionItem(AManager),
                CGlobalComponent(), m_pSignalProcess(this), m_pProcessManager(AManager) {

            sig_reap = 0;
            sig_sigio = 0;
            sig_sigalrm = 0;
            sig_terminate = 0;
            sig_quit = 0;
            sig_debug_quit = 0;
            sig_reconfigure = 0;
            sig_reopen = 0;
            sig_noaccept = 0;
            sig_change_binary = 0;

            sig_exiting = 0;
            sig_restart = 0;
            sig_noaccepting = 0;

            CreateSignals();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::ChildProcessGetStatus() {
            int             status;
            const char     *process;
            pid_t           pid;
            int             err;
            int             i;
            bool            one;

            one = false;

            for ( ;; ) {
                pid = waitpid(-1, &status, WNOHANG);

                if (pid == 0) {
                    return;
                }

                if (pid == -1) {
                    err = errno;

                    if (err == EINTR) {
                        continue;
                    }

                    if (err == ECHILD && one) {
                        return;
                    }

                    /*
                     * Solaris always calls the signal handler for each exited process
                     * despite waitpid() may be already called for this process.
                     *
                     * When several processes exit at the same time FreeBSD may
                     * erroneously call the signal handler for exited process
                     * despite waitpid() may be already called for this process.
                     */

                    if (err == ECHILD) {
                        Log()->Error(APP_LOG_ALERT, err, "waitpid() failed");
                        return;
                    }

                    Log()->Error(APP_LOG_ALERT, err, "waitpid() failed");
                    return;
                }

                one = true;
                process = "unknown process";

                CSignalProcess *pProcess = this;
                for (i = 0; i < m_pProcessManager->ProcessCount(); ++i) {
                    pProcess = m_pProcessManager->Processes(i);
                    if (pProcess->Pid() == pid) {
                        pProcess->Status(status);
                        pProcess->Exited(true);
                        process = pProcess->GetProcessName();
                        break;
                    }
                }

                if (WTERMSIG(status)) {
#ifdef WCOREDUMP
                    Log()->Error(APP_LOG_ALERT, 0,
                                 "%s %P exited on signal %d%s",
                                 process, pid, WTERMSIG(status),
                                 WCOREDUMP(status) ? " (core dumped)" : "");
#else
                    Log()->Error(APP_LOG_ALERT, 0,
                          "%s %P exited on signal %d",
                          process, pid, WTERMSIG(status));
#endif

                } else {
                    Log()->Debug(APP_LOG_DEBUG_EVENT, _T("%s %P exited with code %d"), process, pid, WEXITSTATUS(status));
                }

                if (WEXITSTATUS(status) == 2 && pProcess->Respawn()) {
                    Log()->Error(APP_LOG_ALERT, 0,
                                 "%s %P exited with fatal code %d and cannot be respawned",
                                 process, pid, WEXITSTATUS(status));
                    pProcess->Respawn(false);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::SetSignalProcess(CSignalProcess *Value) {
            if (m_pSignalProcess != Value) {
                m_pSignalProcess = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::CreateSignals() {
            if (Type() == ptSignaller) {

                AddSignal(signal_value(SIG_RECONFIGURE_SIGNAL), "SIG" sig_value(SIG_RECONFIGURE_SIGNAL), "reload", nullptr);
                AddSignal(signal_value(SIG_REOPEN_SIGNAL), "SIG" sig_value(SIG_REOPEN_SIGNAL), "reopen", nullptr);
                AddSignal(signal_value(SIG_TERMINATE_SIGNAL), "SIG" sig_value(SIG_TERMINATE_SIGNAL), "stop", nullptr);
                AddSignal(signal_value(SIG_SHUTDOWN_SIGNAL), "SIG" sig_value(SIG_SHUTDOWN_SIGNAL), "quit", nullptr);

            } else {

                AddSignal(signal_value(SIG_RECONFIGURE_SIGNAL), "SIG" sig_value(SIG_RECONFIGURE_SIGNAL),
                          "reload", signal_handler);

                AddSignal(signal_value(SIG_REOPEN_SIGNAL), "SIG" sig_value(SIG_REOPEN_SIGNAL),
                          "reopen", signal_handler);

                AddSignal(signal_value(SIG_NOACCEPT_SIGNAL), "SIG" sig_value(SIG_NOACCEPT_SIGNAL),
                          "", signal_handler);

                AddSignal(signal_value(SIG_TERMINATE_SIGNAL), "SIG" sig_value(SIG_TERMINATE_SIGNAL),
                          "stop", signal_handler);

                AddSignal(signal_value(SIG_SHUTDOWN_SIGNAL), "SIG" sig_value(SIG_SHUTDOWN_SIGNAL),
                          "quit", signal_handler);

                AddSignal(signal_value(SIG_CHANGEBIN_SIGNAL), "SIG" sig_value(SIG_CHANGEBIN_SIGNAL),
                          "", signal_handler);

                AddSignal(SIGINT, "SIGINT", nullptr, signal_handler);

                AddSignal(SIGIO, "SIGIO", nullptr, signal_handler);

                AddSignal(SIGCHLD, "SIGCHLD", nullptr, signal_handler);

                AddSignal(SIGSYS, "SIGSYS, SIG_IGN", nullptr, nullptr);

                AddSignal(SIGPIPE, "SIGPIPE, SIG_IGN", nullptr, nullptr);
            }

            if (Type() >= ptWorker) {

                AddSignal(SIGFPE, "SIGFPE", nullptr, signal_error);

                AddSignal(SIGILL, "SIGILL", nullptr, signal_error);

                AddSignal(SIGSEGV, "SIGSEGV", nullptr, signal_error);

                AddSignal(SIGBUS, "SIGBUS", nullptr, signal_error);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::SignalHandler(int signo, siginfo_t *siginfo, void *ucontext) {

            LPCTSTR      action;
            LPCTSTR      sigcode;
            int_t        ignore;
            int          err;

            ignore = 0;

            err = errno;

            int i = IndexOfSigNo(signo);
            if (i >= 0)
                sigcode = Signals(i)->Code();
            else
                sigcode = _T("signal code not found");

            action = _T("");

            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("signal handler %d (%s), process name: %s"), signo, sigcode, GetProcessName());

            switch (Type()) {
                case ptMain:
                case ptMaster:
                case ptSingle:
                    switch (signo) {

                        case signal_value(SIG_SHUTDOWN_SIGNAL):
                            sig_quit = 1;
                            action = _T(", shutting down");
                            break;

                        case signal_value(SIG_TERMINATE_SIGNAL):
                        case SIGINT:
                            sig_terminate = 1;
                            action = _T(", exiting");
                            break;

                        case signal_value(SIG_NOACCEPT_SIGNAL):
                            if (Daemonized()) {
                                sig_noaccept = 1;
                                action = _T(", stop accepting connections");
                            }
                            break;

                        case signal_value(SIG_RECONFIGURE_SIGNAL):
                            sig_reconfigure = 1;
                            action = _T(", reconfiguring");
                            break;

                        case signal_value(SIG_REOPEN_SIGNAL):
                            sig_reopen = 1;
                            action = _T(", reopening logs");
                            break;

                        case signal_value(SIG_CHANGEBIN_SIGNAL):
                            if (getppid() == ParentId() || NewBinary() > 0) {

                                /*
                                 * Ignore the signal in the new binary if its parent is
                                 * not changed, i.e. the old binary's process is still
                                 * running.  Or ignore the signal in the old binary's
                                 * process if the new binary's process is already running.
                                 */

                                action = _T(", ignoring");
                                ignore = 1;
                                break;
                            }

                            sig_change_binary = 1;
                            action = _T(", changing binary");
                            break;

                        case SIGALRM:
                            sig_sigalrm = 1;
                            action = _T(", alarm");
                            break;

                        case SIGIO:
                            sig_sigio = 1;
                            break;

                        case SIGCHLD:
                            sig_reap = 1;
                            break;

                        default:
                            break;
                    }

                    break;

                case ptWorker:
                case ptHelper:
                case ptCustom:
                    switch (signo) {

                        case signal_value(SIG_NOACCEPT_SIGNAL):
                            if (!Daemonized()) {
                                break;
                            }
                            sig_debug_quit = 1;
                            /* fall through */
                        case signal_value(SIG_SHUTDOWN_SIGNAL):
                            sig_quit = 1;
                            action = _T(", shutting down");
                            break;

                        case signal_value(SIG_TERMINATE_SIGNAL):
                        case SIGINT:
                            sig_terminate = 1;
                            action = _T(", exiting");
                            break;

                        case signal_value(SIG_REOPEN_SIGNAL):
                            sig_reopen = 1;
                            action = _T(", reopening logs");
                            break;

                        case signal_value(SIG_RECONFIGURE_SIGNAL):
#ifdef _DEBUG
                            sig_terminate = 1;
                            action = _T(", exiting");
#else
                            action = _T(", ignoring");
#endif
                            break;
                        case signal_value(SIG_CHANGEBIN_SIGNAL):
                        case SIGIO:
                            action = _T(", ignoring");
                            break;

                        case SIGALRM:
                            sig_sigalrm = 1;
                            action = _T(", alarm");
                            break;

                        default:
                            break;
                    }

                    break;

                default:
                    break;
            }

            if (siginfo && siginfo->si_pid) {
                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("signal %d (%s) received from %P%s"), signo, sigcode, siginfo->si_pid, action);
            } else {
                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("signal %d (%s) received%s"), signo, sigcode, action);
            }

            if (ignore) {
                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("the changing binary signal is ignored: you should shutdown or terminate before either old or new binary's process"));
            }

            if (signo == SIGCHLD) {
                Log()->Debug(APP_LOG_DEBUG_EVENT, _T("child process get status"));
                ChildProcessGetStatus();
            }

            errno = err;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::ExitSigAlarm(uint_t AMsec) const {

            sigset_t set, wset;
            struct itimerval itv = {0};

            itv.it_interval.tv_sec = 0;
            itv.it_interval.tv_usec = 0;
            itv.it_value.tv_sec = AMsec / 1000;
            itv.it_value.tv_usec = (AMsec % 1000) * 1000000;

            if (setitimer(ITIMER_REAL, &itv, nullptr) == -1) {
                Log()->Error(APP_LOG_ALERT, errno, "setitimer() failed");
            }

            /* Set up the mask of signals to temporarily block. */
            sigemptyset (&set);
            sigaddset (&set, SIGALRM);

            /* Wait for a signal to arrive. */
            sigprocmask(SIG_BLOCK, &set, &wset);

            Log()->Debug(APP_LOG_DEBUG_EVENT, _T("exit alarm after %u msec"), AMsec);

            while (!(sig_terminate || sig_quit))
                sigsuspend(&wset);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::SignalRestart() {
            kill(Pid(), signal_value(SIG_TERMINATE_SIGNAL));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::SignalReload() {
            kill(Pid(), signal_value(SIG_RECONFIGURE_SIGNAL));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::SignalQuit() {
            kill(Pid(), signal_value(SIG_SHUTDOWN_SIGNAL));
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessManager -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::Start(CSignalProcess *AProcess) {
            if (Assigned(AProcess)) {

                DoBeforeStartProcess(AProcess);
                try {
                    AProcess->BeforeRun();
                    AProcess->Run();
                    AProcess->AfterRun();
                } catch (std::exception &e) {
                    log_failure(e.what())
                } catch (...) {
                    log_failure("Unknown error...")
                }
                DoAfterStartProcess(AProcess);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::Stop(int Index) {
            Delete(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::StopAll() {
            for (int i = 1; i < ProcessCount(); i++)
                Stop(i);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::Terminate(int Index) {
            Processes(Index)->Terminate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::TerminateAll() {
            for (int i = 1; i < ProcessCount(); i++)
                Terminate(i);
        }
        //--------------------------------------------------------------------------------------------------------------

        CSignalProcess *CProcessManager::FindProcessById(pid_t Pid) {
            CSignalProcess *Item;

            for (int i = 0; i < ProcessCount(); i++) {
                Item = Processes(i);
                if (Item->Pid() == Pid)
                    return Item;
            }

            return nullptr;
        }
    }
}
}

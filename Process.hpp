/*++

Library name:

  apostol-core

Module Name:

  Process.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_PROCESS_HPP
#define APOSTOL_PROCESS_HPP
//----------------------------------------------------------------------------------------------------------------------

#define PROCESS_NORESPAWN     -1
#define PROCESS_JUST_SPAWN    -2
#define PROCESS_RESPAWN       -3
#define PROCESS_JUST_RESPAWN  -4
#define PROCESS_DETACHED      -5
//----------------------------------------------------------------------------------------------------------------------

#define log_failure(msg) {                                  \
  if (GLog != nullptr)                                      \
    GLog->Error(APP_LOG_EMERG, 0, "Core: %s", msg);         \
  else                                                      \
    std::cerr << APP_NAME << ": " << (msg) << std::endl;    \
  exit(2);                                                  \
}                                                           \
//----------------------------------------------------------------------------------------------------------------------

void signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
void signal_error(int signo, siginfo_t *siginfo, void *ucontext);
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Process {

        enum CProcessStatus { psRunning, psStopped };
        //--------------------------------------------------------------------------------------------------------------

        class CProcessManager;

        //--------------------------------------------------------------------------------------------------------------

        //-- CSignalProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CSignalProcess: public CCustomProcess, public CSignals, public CCollectionItem, public CGlobalComponent {
        private:

            CSignalProcess  *m_pSignalProcess;

            CProcessManager *m_pProcessManager;

        protected:

            sig_atomic_t    sig_reap;
            sig_atomic_t    sig_sigio;
            sig_atomic_t    sig_sigalrm;
            sig_atomic_t    sig_terminate;
            sig_atomic_t    sig_quit;
            sig_atomic_t    sig_debug_quit;
            sig_atomic_t    sig_reconfigure;
            sig_atomic_t    sig_reopen;
            sig_atomic_t    sig_noaccept;
            sig_atomic_t    sig_change_binary;

            uint_t          sig_exiting;
            uint_t          sig_restart;
            uint_t          sig_noaccepting;

            void CreateSignals();

            void ChildProcessGetStatus();

            void SetSignalProcess(CSignalProcess *Value);

        public:

            CSignalProcess(CCustomProcess *AParent, CProcessManager *AManager, CProcessType AType, LPCTSTR AName);

            virtual CSignalProcess *SignalProcess() { return m_pSignalProcess; };
            void SignalProcess(CSignalProcess *Value) { SetSignalProcess(Value); };

            void SignalHandler(int signo, siginfo_t *siginfo, void *ucontext) override;

            void ExitSigAlarm(uint_t AMsec) const;

            virtual void SignalRestart();
            virtual void SignalReload();
            virtual void SignalQuit();

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessManager -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessManager: public CCollection {
            typedef CCollection inherited;

        protected:

            virtual void DoBeforeStartProcess(CSignalProcess *AProcess) abstract;
            virtual void DoAfterStartProcess(CSignalProcess *AProcess) abstract;

        public:

            explicit CProcessManager(): CCollection(this) {};

            void Start(CSignalProcess *AProcess);
            void Stop(int Index);
            void StopAll();

            void Terminate(int Index);
            void TerminateAll();

            int ProcessCount() { return inherited::Count(); };
            void DeleteProcess(int Index) { inherited::Delete(Index); };

            CSignalProcess *FindProcessById(pid_t Pid);

            CSignalProcess *Processes(int Index) { return (CSignalProcess *) inherited::GetItem(Index); }
            void Processes(int Index, CSignalProcess *Value) { inherited::SetItem(Index, (CCollectionItem *) Value); }
        };

    }
//----------------------------------------------------------------------------------------------------------------------
}

using namespace Apostol::Process;
}
//----------------------------------------------------------------------------------------------------------------------

#endif //APOSTOL_PROCESS_HPP

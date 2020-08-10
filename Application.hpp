/*++

Library name:

  apostol-core

Module Name:

  Application.hpp

Notices:

  Apostol application

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_APPLICATION_HPP
#define APOSTOL_APPLICATION_HPP
//----------------------------------------------------------------------------------------------------------------------

#define MSG_PROCESS_START _T("start process: (%s) - %s")
#define MSG_PROCESS_STOP _T("stop process: (%s)")
//----------------------------------------------------------------------------------------------------------------------

#define APP_FILE_NOT_FOUND _T("File not found: %s")
//----------------------------------------------------------------------------------------------------------------------

#define ExitRun(Status) {   \
  ExitCode(Status);         \
  return;                   \
}                           \
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Application {

        class CApplication;
        //--------------------------------------------------------------------------------------------------------------

        extern CApplication *GApplication;

        //--------------------------------------------------------------------------------------------------------------

        //-- CApplicationProcess ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CApplicationProcess: public CSignalProcess {
        private:

            CApplication *m_pApplication;

        protected:

            void BeforeRun() override;
            void AfterRun() override;

            pid_t SwapProcess(CProcessType Type, int Flag, Pointer Data = nullptr);
            pid_t ExecProcess(CExecuteContext *AContext);

        public:

            explicit CApplicationProcess(CCustomProcess* AParent, CApplication *AApplication, CProcessType AType, LPCTSTR AName);

            ~CApplicationProcess() override = default;

            static class CSignalProcess *Create(CCustomProcess *AParent, CApplication *AApplication, CProcessType AType);

            void Assign(CCustomProcess *AProcess) override;

            CApplication *Application() { return m_pApplication; };

            bool RenamePidFile(bool Back, LPCTSTR lpszErrorMessage);

            void CreatePidFile();
            void DeletePidFile();

            void OnFilerError(Pointer Sender, int Error, LPCTSTR lpFormat, va_list args);

        }; // class CApplicationProcess

        //--------------------------------------------------------------------------------------------------------------

        //-- CApplication ----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CApplication: public CProcessManager, public CCustomApplication, public CApplicationProcess {
            friend CApostolModule;

        protected:

            CProcessType m_ProcessType;

            static void CreateLogFile();
            void Daemonize();

            static void CreateDirectories();

            void DoBeforeStartProcess(CSignalProcess *AProcess) override;
            void DoAfterStartProcess(CSignalProcess *AProcess) override;

            void SetProcessType(CProcessType Value);

            virtual void ParseCmdLine() abstract;
            virtual void ShowVersionInfo() abstract;

            virtual void StartProcess();

        public:

            CApplication(int argc, char *const *argv);

            ~CApplication() override;

            inline void Destroy() override { delete this; };

            template <class ClassProcess>
            inline CApplication &AddProcess() {
                new ClassProcess(SignalProcess(), this);
                return *this;
            };

            CString ProcessesNames();

            pid_t ExecNewBinary(char *const *argv, CSocketHandles *AHandles);

            void SetNewBinary(CApplicationProcess *AProcess, CSocketHandles *ABindings);

            CProcessType ProcessType() { return m_ProcessType; };
            void ProcessType(CProcessType Value) { SetProcessType(Value); };

            void Run() override;

            static void MkDir(const CString &Dir);

        }; // class CApplication

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessSingle --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessSingle: public CApplicationProcess, public CModuleProcess {
            typedef CApplicationProcess inherited;

        protected:

            void BeforeRun() override;
            void AfterRun() override;

        protected:

            void DoExit();

        public:

            CProcessSingle(CCustomProcess *AParent, CApplication *AApplication);

            ~CProcessSingle() override = default;

            void Run() override;

            void Reload();

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessMaster --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessMaster: public CApplicationProcess, public CModuleProcess {
            typedef CApplicationProcess inherited;

        protected:

            void BeforeRun() override;
            void AfterRun() override;

            void DoExit();

            bool ReapChildren();

            void StartProcess(CProcessType Type, int Flag);
            void StartProcesses(int Flag);

            void StartCustomProcesses();

            void SignalToProcess(CProcessType Type, int SigNo);
            void SignalToProcesses(int SigNo);

        public:

            CProcessMaster(CCustomProcess* AParent, CApplication *AApplication);;

            ~CProcessMaster() override = default;

            void Run() override;

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessSignaller -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessSignaller: public CApplicationProcess {
            typedef CApplicationProcess inherited;

        private:

            void SignalToProcess(pid_t pid);

        public:

            CProcessSignaller(CCustomProcess* AParent, CApplication *AApplication):
                    inherited(AParent, AApplication, ptSignaller, "signaller") {
            };

            ~CProcessSignaller() override = default;

            void Run() override;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessNewBinary -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessNewBinary: public CApplicationProcess {
            typedef CApplicationProcess inherited;

        public:

            CProcessNewBinary(CCustomProcess* AParent, CApplication *AApplication):
                    inherited(AParent, AApplication, ptNewBinary, "new binary") {
            };

            ~CProcessNewBinary() override = default;

            void Run() override;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessWorker --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessWorker: public CApplicationProcess, public CWorkerProcess {
            typedef CApplicationProcess inherited;

        private:

            void BeforeRun() override;
            void AfterRun() override;

        protected:

            void DoExit();

        public:

            CProcessWorker(CCustomProcess *AParent, CApplication *AApplication);

            ~CProcessWorker() override = default;

            void Run() override;

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessHelper --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessHelper: public CApplicationProcess, public CHelperProcess {
            typedef CApplicationProcess inherited;

        private:

            void BeforeRun() override;
            void AfterRun() override;

        protected:

            void DoExit();

        public:

            CProcessHelper(CCustomProcess* AParent, CApplication *AApplication);

            ~CProcessHelper() override = default;

            void Run() override;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessCustom --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CProcessCustom: public CApplicationProcess, public CModuleProcess {
            typedef CApplicationProcess inherited;

        public:

            CProcessCustom(CCustomProcess* AParent, CApplication *AApplication, LPCTSTR AName):
                    inherited(AParent, AApplication, ptCustom, AName), CModuleProcess() {
            };

            ~CProcessCustom() override = default;

        };

    }
}

using namespace Apostol::Application;
}
#endif //APOSTOL_APPLICATION_HPP

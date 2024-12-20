/*++

Library name:

  apostol-core

Module Name:

  Module.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_MODULE_HPP
#define APOSTOL_MODULE_HPP

#define APOSTOL_INDEX_FILE "index.html"

extern "C++" {

namespace Apostol {

    namespace Module {

        class CModuleProcess;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CHTTPServerConnection *AConnection)> COnMethodHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        LPCTSTR StrWebTime(time_t Time, LPTSTR lpszBuffer, size_t Size);

        //--------------------------------------------------------------------------------------------------------------

        //-- CMethodHandler --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CMethodHandler: CObject {
        private:

            bool m_Allow;
            COnMethodHandlerEvent m_Handler;

        public:

            CMethodHandler(bool Allow, COnMethodHandlerEvent && Handler): CObject(),
                m_Allow(Allow), m_Handler(Handler) {

            };

            bool Allow() const { return m_Allow; };

            void Handler(CHTTPServerConnection *AConnection) {
                if (m_Allow && m_Handler)
                    m_Handler(AConnection);
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CModuleManager;
        //--------------------------------------------------------------------------------------------------------------

        enum CModuleStatus { msUnknown = -1, msDisabled, msEnabled };
        //--------------------------------------------------------------------------------------------------------------
#ifdef WITH_POSTGRESQL
        typedef std::function<void (CHTTPServerConnection *AConnection, CPQPollQuery *APollQuery)> COnApostolModuleSuccessEvent;
        typedef std::function<void (CHTTPServerConnection *AConnection, const Delphi::Exception::Exception &E)> COnApostolModuleFailEvent;
        //--------------------------------------------------------------------------------------------------------------
#endif
        class CApostolModule: public CCollectionItem, public CGlobalComponent {
        private:

            CString m_ModuleName;

            /// Ini file section name
            CString m_SectionName;

            CStringPairs m_Sites;

            mutable CString m_AllowedMethods;
            mutable CString m_AllowedHeaders;

        protected:

            CStringList m_Headers;

            CStringList m_Methods { true };

            CModuleStatus m_ModuleStatus;

            CModuleProcess *m_pModuleProcess;

            virtual void InitMethods() abstract;

            void InitSites(const CSites &Sites);

            virtual void DoHead(CHTTPServerConnection *AConnection);
            virtual void DoGet(CHTTPServerConnection *AConnection);
            virtual void DoOptions(CHTTPServerConnection *AConnection);

            virtual void CORS(CHTTPServerConnection *AConnection);

            virtual void MethodNotAllowed(CHTTPServerConnection *AConnection);

            const CString& GetAllowedMethods() const;
            const CString& GetAllowedHeaders() const;

            virtual void DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args);
            virtual void DoException(CTCPConnection *AConnection, const Delphi::Exception::Exception &E);
            virtual void DoEventHandlerException(CPollEventHandler *AHandler, const Delphi::Exception::Exception &E);
            virtual void DoNoCommandHandler(CSocketEvent *Sender, const CString &Data, CTCPConnection *AConnection);

#ifdef WITH_POSTGRESQL
            virtual void DoPostgresNotify(CPQConnection *AConnection, PGnotify *ANotify);
            virtual void DoPostgresQueryExecuted(CPQPollQuery *APollQuery);
            virtual void DoPostgresQueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E);
#endif
        public:

            explicit CApostolModule(CModuleProcess *AProcess, const CString& ModuleName, const CString& SectionName = CString());

            ~CApostolModule() override = default;

            const CString& ModuleName() const { return m_ModuleName; }
            const CString& SectionName() const { return m_SectionName; }

            CModuleStatus ModuleStatus() const { return m_ModuleStatus; }
            CModuleProcess *ModuleProcess() const{ return m_pModuleProcess; }

            const CString& AllowedMethods() const { return GetAllowedMethods(); };
            const CString& AllowedHeaders() const { return GetAllowedHeaders(); };

            virtual bool Enabled() abstract;
            virtual bool CheckLocation(const CLocation &Location);

            virtual void Initialization(CModuleProcess *AProcess) {};
            virtual void Finalization(CModuleProcess *AProcess) {};

            virtual void BeforeExecute(CModuleProcess *AProcess) {};
            virtual void AfterExecute(CModuleProcess *AProcess) {};

            virtual void Heartbeat(CDateTime Datetime);
            virtual bool Execute(CHTTPServerConnection *AConnection);

            static CString GetHostName();
            static CString GetIPByHostName(const CString &HostName);

            static CString GetUserAgent(CHTTPServerConnection *AConnection);
            static CString GetOrigin(CHTTPServerConnection *AConnection);
            static CString GetProtocol(CHTTPServerConnection *AConnection);
            static CString GetRealIP(CHTTPServerConnection *AConnection);
            static CString GetHost(CHTTPServerConnection *AConnection);

            static bool AllowedLocation(const CString &Path, const CStringList &List);

            CString GetRoot(const CString &Host) const;
            const CString& GetSiteRoot(const CString &Host) const;
            const CStringList& GetSiteConfig(const CString &Host) const;

            CHTTPClient *GetClient(const CString &Host, uint16_t Port);

            CHTTPServer &Server();
            const CHTTPServer &Server() const;
#ifdef WITH_POSTGRESQL
            CPQClient &PQClient(const CString &ConfName);
            const CPQClient &PQClient(const CString &ConfName) const;

            virtual CPQPollQuery *GetQuery(CPollConnection *AConnection, const CString &ConfName);

            static void EnumQuery(CPQResult *APQResult, CPQueryResult& AResult);
            static void QueryToResults(CPQPollQuery *APollQuery, CPQueryResults& AResults);

            CPQPollQuery *ExecSQL(const CStringList &SQL, CPollConnection *AConnection = nullptr,
                COnPQPollQueryExecutedEvent && OnExecuted = nullptr, COnPQPollQueryExceptionEvent && OnException = nullptr,
                const CString &ConfName = {});

            CPQPollQuery *ExecuteSQL(const CStringList &SQL, CHTTPServerConnection *AConnection,
                COnApostolModuleSuccessEvent && OnSuccess, COnApostolModuleFailEvent && OnFail = nullptr,
                const CString &ConfName = {});

            static void PQResultToList(CPQResult *Result, CStringList &List);
            static void PQResultToJson(CPQResult *Result, CString &Json, const CString &Format = CString(), const CString &ObjectName = CString());
#endif
            CStringList &Methods() { return m_Methods; };
            const CStringList &Methods() const { return m_Methods; };

            static void ContentToJson(const CHTTPRequest &Request, CJSON &Json);
            static void ListToJson(const CStringList &List, CString &Json, bool DataArray = false, const CString &ObjectName = CString());

            static void ExceptionToJson(int ErrorCode, const std::exception &e, CString& Json);

            static void ReplyError(CHTTPServerConnection *AConnection, CHTTPReply::CStatusType ErrorCode, const CString &Message);

            void Redirect(CHTTPServerConnection *AConnection, const CString& Location, bool SendNow = false) const;

            static CString TryFiles(const CString &Root, const CStringList &uris, const CString &Location);

            static bool ResourceExists(CString &Resource, const CString &Root, const CString &Path, const CStringList &TryFiles) ;

            bool SendResource(CHTTPServerConnection *AConnection, const CString &Path, LPCTSTR AContentType = nullptr,
                bool SendNow = false, const CStringList& TryFiles = CStringList(), bool SendNotFound = true) const;

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CModuleManager --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CModuleManager: public CCollection {
            typedef CCollection inherited;

        private:

            bool ExecuteModule(CHTTPServerConnection *AConnection, CApostolModule *AModule);

        protected:

            virtual void DoInitialization(CApostolModule *AModule) abstract;
            virtual void DoFinalization(CApostolModule *AModule) abstract;

            virtual void DoBeforeExecuteModule(CApostolModule *AModule) abstract;
            virtual void DoAfterExecuteModule(CApostolModule *AModule) abstract;

        public:

            explicit CModuleManager(): CCollection(this) {

            };

            CString ModulesNames();

            void Initialization();
            void Finalization();

            void HeartbeatModules(CDateTime Datetime);

            void ExecuteModules(CTCPConnection *AConnection);

            int ModuleCount() const { return inherited::Count(); }
            void DeleteModule(int Index) { inherited::Delete(Index); }

            CApostolModule *Modules(int Index) { return (CApostolModule *) inherited::GetItem(Index); }
            void Modules(int Index, CApostolModule *Value) { inherited::SetItem(Index, (CCollectionItem *) Value); }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CModuleProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CModuleProcess: public CServerProcess, public CModuleManager {
        protected:

            void DoInitialization(CApostolModule *AModule) override;
            void DoFinalization(CApostolModule *AModule) override;

            void DoBeforeExecuteModule(CApostolModule *AModule) override;
            void DoAfterExecuteModule(CApostolModule *AModule) override;

            void DoTimer(CPollEventHandler *AHandler) override;
            bool DoExecute(CTCPConnection *AConnection) override;

        public:

            CModuleProcess();

            ~CModuleProcess() override = default;

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CWorkerProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWorkerProcess: public CModuleProcess {
        public:

            CWorkerProcess();

            ~CWorkerProcess() override = default;

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CHelperProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CHelperProcess: public CModuleProcess {
        public:

            CHelperProcess();

            ~CHelperProcess() override = default;

        };

    }
}

using namespace Apostol::Module;
}
//----------------------------------------------------------------------------------------------------------------------

#endif //APOSTOL_MODULE_HPP

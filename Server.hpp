/*++

Library name:

  apostol-core

Module Name:

  Server.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_SERVER_HPP
#define APOSTOL_SERVER_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Server {

        //--------------------------------------------------------------------------------------------------------------

        //-- CServerProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<CHTTPClient * (const CLocation &URL)> COnGetHTTPClientEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CServerProcess: public CObject, public CGlobalComponent {
        private:

            CPollStack m_PollStack;

            CHTTPServer m_Server;
#ifdef WITH_POSTGRESQL
            CPQServer m_PQServer;
#endif
            virtual void UpdateTimer();

        protected:

            CHTTPClientManager m_ClientManager;

            CEPollTimer *m_pTimer;

            int m_TimerInterval;

            void SetTimerInterval(int Value);

            void InitializeCommandHandlers(CCommandHandlers *AHandlers, bool ADisconnect = false);

            void InitializeServer(const CString &Title, const CString &Listen = Config()->Listen(), u_short Port = Config()->Port());
#ifdef WITH_POSTGRESQL
            void InitializePQServer(const CString &Title, u_int Min = Config()->PostgresPollMin(), u_int Max = Config()->PostgresPollMax());
#endif
            virtual void DoOptions(CCommand *ACommand);
            virtual void DoGet(CCommand *ACommand);
            virtual void DoHead(CCommand *ACommand);
            virtual void DoPost(CCommand *ACommand);
            virtual void DoPut(CCommand *ACommand);
            virtual void DoPatch(CCommand *ACommand);
            virtual void DoDelete(CCommand *ACommand);
            virtual void DoTrace(CCommand *ACommand);
            virtual void DoConnect(CCommand *ACommand);

            virtual void DoTimer(CPollEventHandler *AHandler) abstract;
            virtual bool DoExecute(CTCPConnection *AConnection) abstract;

            virtual void DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args);
            virtual void DoAccessLog(CTCPConnection *AConnection);

            virtual void DoServerListenException(CSocketEvent *Sender, const Delphi::Exception::Exception &E);
            virtual void DoServerException(CTCPConnection *AConnection, const Delphi::Exception::Exception &E);
            virtual void DoServerEventHandlerException(CPollEventHandler *AHandler, const Delphi::Exception::Exception &E);

            virtual void DoServerConnected(CObject *Sender);
            virtual void DoServerDisconnected(CObject *Sender);

            virtual void DoClientConnected(CObject *Sender);
            virtual void DoClientDisconnected(CObject *Sender);

            virtual void DoNoCommandHandler(CSocketEvent *Sender, const CString &Data, CTCPConnection *AConnection);

            void InitializeServerHandlers();
#ifdef WITH_POSTGRESQL
            void InitializePQServerHandlers();

            virtual void DoPQServerException(CPQServer *AServer, const Delphi::Exception::Exception &E);
            virtual void DoPQConnectException(CPQConnection *AConnection, const Delphi::Exception::Exception &E);

            virtual void DoPQNotify(CPQConnection *AConnection, PGnotify *ANotify);

            virtual void DoPQError(CPQConnection *AConnection);
            virtual void DoPQStatus(CPQConnection *AConnection);
            virtual void DoPQPollingStatus(CPQConnection *AConnection);

            virtual void DoPQReceiver(CPQConnection *AConnection, const PGresult *AResult);
            virtual void DoPQProcessor(CPQConnection *AConnection, LPCSTR AMessage);

            virtual void DoPQConnect(CObject *Sender);
            virtual void DoPQDisconnect(CObject *Sender);

            virtual void DoPQSendQuery(CPQQuery *AQuery);
            virtual void DoPQResultStatus(CPQResult *AResult);
            virtual void DoPQResult(CPQResult *AResult, ExecStatusType AExecStatus);
#endif
        public:

            CServerProcess();

            ~CServerProcess() override = default;

            void ServerStart();
            void ServerStop();
            void ServerShutDown();

            virtual void Reload();

            CHTTPServer &Server() { return m_Server; };
            const CHTTPServer &Server() const { return m_Server; };
#ifdef WITH_POSTGRESQL
            void PQServerStart(const CString &Name);
            void PQServerStop();

            CPQServer &PQServer() { return m_PQServer; };
            const CPQServer &PQServer() const { return m_PQServer; };

            virtual CPQPollQuery *GetQuery(CPollConnection *AConnection);

            CPQPollQuery *ExecSQL(const CStringList &SQL, CPollConnection *AConnection = nullptr,
                         COnPQPollQueryExecutedEvent && OnExecuted = nullptr,
                         COnPQPollQueryExceptionEvent && OnException = nullptr);
#endif
            CHTTPClientItem *GetClient(const CString &Host, uint16_t Port);

            CHTTPClientManager &ClientManager() { return m_ClientManager; };
            const CHTTPClientManager &ClientManager() const { return m_ClientManager; };

            CPollStack PollStack() const { return m_PollStack; }

            int TimerInterval() const { return m_TimerInterval; }
            void TimerInterval(int Value) { SetTimerInterval(Value); }

            static void LoadProviders(CProviders &Providers);
            static void LoadSites(CSites &Sites);
            static void LoadOAuth2(const CString &FileName, const CString &ProviderName, const CString &ApplicationName,
                CProviders &Providers);
        };
    }
}

using namespace Apostol::Server;
};

#endif //APOSTOL_SERVER_HPP

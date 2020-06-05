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

        class CServerProcess: public CObject, public CGlobalComponent {
        private:

            CPollStack m_PollStack;

            CHTTPServer m_Server;
#ifdef WITH_POSTGRESQL
            CPQServer m_PQServer;
#endif
            virtual void UpdateTimer();

        protected:

            CEPollTimer *m_pTimer;

            int m_TimerInterval;

            void SetTimerInterval(int Value);

            void InitializeCommandHandlers(CCommandHandlers *AHandlers, bool ADisconnect = false);

            void InitializeWorkerServer(const CString &Title);
            void InitializeHelperServer(const CString &Title);
#ifdef WITH_POSTGRESQL
            void InitializePQServer(const CString &Title);
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

            virtual void DoServerListenException(CSocketEvent *Sender, Delphi::Exception::Exception *AException);
            virtual void DoServerException(CTCPConnection *AConnection, Delphi::Exception::Exception *AException);
            virtual void DoServerEventHandlerException(CPollEventHandler *AHandler, Delphi::Exception::Exception *AException);

            virtual void DoServerConnected(CObject *Sender);
            virtual void DoServerDisconnected(CObject *Sender);

            virtual void DoClientConnected(CObject *Sender);
            virtual void DoClientDisconnected(CObject *Sender);

            virtual void DoNoCommandHandler(CSocketEvent *Sender, LPCTSTR AData, CTCPConnection *AConnection);

            void InitializeServerHandlers();
#ifdef WITH_POSTGRESQL
            void InitializePQServerHandlers();

            virtual void DoPQServerException(CPQServer *AServer, Delphi::Exception::Exception *AException);
            virtual void DoPQConnectException(CPQConnection *AConnection, Delphi::Exception::Exception *AException);

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

            void ServerStart();
            void ServerStop();
            void ServerShutDown();

            CHTTPServer &Server() { return m_Server; };
            const CHTTPServer &Server() const { return m_Server; };
#ifdef WITH_POSTGRESQL
            void PQServerStart();
            void PQServerStop();

            CPQServer &PQServer() { return m_PQServer; };
            const CPQServer &PQServer() const { return m_PQServer; };

            virtual CPQPollQuery *GetQuery(CPollConnection *AConnection);

            bool ExecSQL(const CStringList &SQL, CPollConnection *AConnection = nullptr,
                         COnPQPollQueryExecutedEvent && OnExecuted = nullptr,
                         COnPQPollQueryExceptionEvent && OnException = nullptr);
#endif
            CHTTPClient *GetClient(const CString &Host, uint16_t Port);

            CPollStack PollStack() { return m_PollStack; }

            static void DebugRequest(CRequest *ARequest);
            static void DebugReply(CReply *AReply);

            int TimerInterval() const { return m_TimerInterval; }
            void TimerInterval(int Value) { SetTimerInterval(Value); }

            static void LoadAuthParams(CAuthParams &AuthParams);
            static void LoadSites(CSites &Sites);

        };

    }
}

using namespace Apostol::Server;
};

#endif //APOSTOL_SSS_SERVER_HPP

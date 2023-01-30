/*++

Library name:

  apostol-core

Module Name:

  Server.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Server.hpp"

#define NOT_FOUND_CONFIGURATION_NAME _T("PQClient: Not found configuration name: %s.")

extern "C++" {

namespace Apostol {

    namespace Server {

        //--------------------------------------------------------------------------------------------------------------

        //-- CServerProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CServerProcess::CServerProcess(): CObject(), CGlobalComponent() {
            m_pTimer = nullptr;
            m_TimerInterval = 0;

            m_EventHandlers.PollStack().TimeOut(Config()->TimeOut());

            m_Server.AllocateEventHandlers(&m_EventHandlers);
            InitializeServerHandlers();
#ifdef WITH_POSTGRESQL
            m_ConfName = "worker";
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::InitializeCommandHandlers(CCommandHandlers *AHandlers, bool ADisconnect) {
            if (Assigned(AHandlers)) {
                CCommandHandler *pCommand;

                AHandlers->ParseParamsDefault(false);
                AHandlers->DisconnectDefault(ADisconnect);
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                pCommand =AHandlers->Add();
                pCommand->Command() = _T("GET");
                pCommand->OnCommand([this](auto && ACommand) { DoGet(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("POST");
                pCommand->OnCommand([this](auto && ACommand) { DoPost(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("OPTIONS");
                pCommand->OnCommand([this](auto && ACommand) { DoOptions(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("PUT");
                pCommand->OnCommand([this](auto && ACommand) { DoPut(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("DELETE");
                pCommand->OnCommand([this](auto && ACommand) { DoDelete(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("HEAD");
                pCommand->OnCommand([this](auto && ACommand) { DoHead(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("PATCH");
                pCommand->OnCommand([this](auto && ACommand) { DoPatch(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("TRACE");
                pCommand->OnCommand([this](auto && ACommand) { DoTrace(ACommand); });

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("CONNECT");
                pCommand->OnCommand([this](auto && ACommand) { DoConnect(ACommand); });
#else
                pCommand =AHandlers->Add();
                pCommand->Command() = _T("GET");
                pCommand->OnCommand(std::bind(&CServerProcess::DoGet, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("POST");
                pCommand->OnCommand(std::bind(&CServerProcess::DoPost, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("OPTIONS");
                pCommand->OnCommand(std::bind(&CServerProcess::DoOptions, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("PUT");
                pCommand->OnCommand(std::bind(&CServerProcess::DoPut, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("DELETE");
                pCommand->OnCommand(std::bind(&CServerProcess::DoDelete, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("HEAD");
                pCommand->OnCommand(std::bind(&CServerProcess::DoHead, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("PATCH");
                pCommand->OnCommand(std::bind(&CServerProcess::DoPatch, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("TRACE");
                pCommand->OnCommand(std::bind(&CServerProcess::DoTrace, this, _1));

                pCommand = AHandlers->Add();
                pCommand->Command() = _T("CONNECT");
                pCommand->OnCommand(std::bind(&CServerProcess::DoConnect, this, _1));
#endif
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::InitializeServerHandlers() {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            m_Server.OnExecute([this](auto && AConnection) { return DoExecute(AConnection); });

            m_Server.OnVerbose([this](auto && Sender, auto && AConnection, auto && AFormat, auto && args) { DoVerbose(Sender, AConnection, AFormat, args); });
            m_Server.OnAccessLog([this](auto && AConnection) { DoAccessLog(AConnection); });

            m_Server.OnException([this](auto && AConnection, auto && AException) { DoServerException(AConnection, AException); });
            m_Server.OnListenException([this](auto && AConnection, auto && AException) { DoServerListenException(AConnection, AException); });

            m_Server.OnEventHandlerException([this](auto && AHandler, auto && AException) { DoServerEventHandlerException(AHandler, AException); });

            m_Server.OnConnected([this](auto && Sender) { DoServerConnected(Sender); });
            m_Server.OnDisconnected([this](auto && Sender) { DoServerDisconnected(Sender); });

            m_Server.OnNoCommandHandler([this](auto && Sender, auto && AData, auto && AConnection) { DoNoCommandHandler(Sender, AData, AConnection); });
#else
            m_Server.OnExecute(std::bind(&CServerProcess::DoExecute, this, _1));

            m_Server.OnVerbose(std::bind(&CServerProcess::DoVerbose, this, _1, _2, _3, _4));
            m_Server.OnAccessLog(std::bind(&CServerProcess::DoAccessLog, this, _1));

            m_Server.OnException(std::bind(&CServerProcess::DoServerException, this, _1, _2));
            m_Server.OnListenException(std::bind(&CServerProcess::DoServerListenException, this, _1, _2));

            m_Server.OnEventHandlerException(std::bind(&CServerProcess::DoServerEventHandlerException, this, _1, _2));

            m_Server.OnConnected(std::bind(&CServerProcess::DoServerConnected, this, _1));
            m_Server.OnDisconnected(std::bind(&CServerProcess::DoServerDisconnected, this, _1));

            m_Server.OnNoCommandHandler(std::bind(&CServerProcess::DoNoCommandHandler, this, _1, _2, _3));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::SetTimerInterval(int Value) {
            if (m_TimerInterval != Value) {
                m_TimerInterval = Value;
                UpdateTimer();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::UpdateTimer() {
            if (m_pTimer == nullptr) {
                m_pTimer = CEPollTimer::CreateTimer(CLOCK_MONOTONIC, TFD_NONBLOCK);
                m_pTimer->AllocateTimer(m_Server.EventHandlers(), m_TimerInterval, m_TimerInterval);
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                m_pTimer->OnTimer([this](auto && AHandler) { DoTimer(AHandler); });
#else
                m_pTimer->OnTimer(std::bind(&CServerProcess::DoTimer, this, _1));
#endif
            } else {
                m_pTimer->SetTimer(m_TimerInterval, m_TimerInterval);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::InitializeServer(const CString &Title, const CString &Listen, u_short Port) {
            m_Server.ServerName() = Title;

            m_Server.DefaultIP() = Listen;
            m_Server.DefaultPort(Port);

            LoadSites(m_Server.Sites());
            LoadProviders(m_Server.Providers());

            m_Server.InitializeBindings();
            m_Server.ActiveLevel(alBinding);
        }
        //--------------------------------------------------------------------------------------------------------------

#ifdef WITH_POSTGRESQL
        void CServerProcess::InitializePQClientHandlers(CPQClient &PQClient) {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            if (Config()->PostgresNotice()) {
                //m_PQClient.OnReceiver([this](auto && AConnection, auto && AResult) { DoPQReceiver(AConnection, AResult); });
                PQClient.OnProcessor([this](auto && AConnection, auto && AMessage) { DoPQProcessor(AConnection, AMessage); });
            }

            PQClient.OnConnectException([this](auto && AConnection, auto && AException) { DoPQConnectException(AConnection, AException); });
            PQClient.OnServerException([this](auto && AClient, auto && AException) { DoPQClientException(AClient, AException); });

            PQClient.OnEventHandlerException([this](auto && AHandler, auto && AException) { DoServerEventHandlerException(AHandler, AException); });

            PQClient.OnNotify([this](auto && AConnection, auto && ANotify) { DoPQNotify(AConnection, ANotify); });

            PQClient.OnPQError([this](auto && AConnection) { DoPQError(AConnection); });
            PQClient.OnPQTimeOut([this](auto && AConnection) { DoPQTimeOut(AConnection); });
            PQClient.OnPQStatus([this](auto && AConnection) { DoPQStatus(AConnection); });
            PQClient.OnPQPollingStatus([this](auto && AConnection) { DoPQPollingStatus(AConnection); });

            PQClient.OnConnected([this](auto && Sender) { DoPQConnect(Sender); });
            PQClient.OnDisconnected([this](auto && Sender) { DoPQDisconnect(Sender); });
#else
            if (Config()->PostgresNotice()) {
                //PQClient.OnReceiver(std::bind(&CServerProcess::DoPQReceiver, this, _1, _2));
                PQClient.OnProcessor(std::bind(&CServerProcess::DoPQProcessor, this, _1, _2));
            }

            PQClient.OnConnectException(std::bind(&CServerProcess::DoPQConnectException, this, _1, _2));
            PQClient.OnServerException(std::bind(&CServerProcess::DoPQClientException, this, _1, _2));

            PQClient.OnEventHandlerException(std::bind(&CServerProcess::DoServerEventHandlerException, this, _1, _2));

            PQClient.OnNotify(std::bind(&CServerProcess::DoPQNotify, this, _1, _2));

            PQClient.OnPQError(std::bind(&CServerProcess::DoPQError, this, _1));
            PQClient.OnPQTimeOut(std::bind(&CServerProcess::DoPQTimeOut, this, _1));
            PQClient.OnPQStatus(std::bind(&CServerProcess::DoPQStatus, this, _1));
            PQClient.OnPQPollingStatus(std::bind(&CServerProcess::DoPQPollingStatus, this, _1));

            PQClient.OnConnected(std::bind(&CServerProcess::DoPQConnect, this, _1));
            PQClient.OnDisconnected(std::bind(&CServerProcess::DoPQDisconnect, this, _1));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::InitializePQClients(const CString &Title, u_int Min, u_int Max) {
            for (int i = 0; i < Config()->PostgresConnInfo().Count(); i++) {
                const auto &connInfo = Config()->PostgresConnInfo()[i];
                const auto index = m_PQClients.AddPair(connInfo.Name(), CPQClient(Min, Max));

                auto &PQClient = m_PQClients[index].Value();

                PQClient.ConnInfo().ApplicationName() = "'" + Title + "'"; //application_name;
                PQClient.ConnInfo().SetParameters(connInfo.Value());

                PQClient.AllocateEventHandlers(&m_EventHandlers);
                InitializePQClientHandlers(PQClient);
            }
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
        void CServerProcess::ServerStart() {
            m_Server.ActiveLevel(alActive);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::ServerStop() {
            m_Server.ActiveLevel(alBinding);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::ServerShutDown() {
            m_Server.ActiveLevel(alShutDown);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::Reload() {
            Config()->Reload();

            LoadSites(m_Server.Sites());
            LoadProviders(m_Server.Providers());
        }
        //--------------------------------------------------------------------------------------------------------------
#ifdef WITH_POSTGRESQL
        CPQClient &CServerProcess::GetPQClient() {
            return GetPQClient(m_ConfName);
        }
        //--------------------------------------------------------------------------------------------------------------

        const CPQClient &CServerProcess::GetPQClient() const {
            return GetPQClient(m_ConfName);
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQClient &CServerProcess::GetPQClient(const CString &ConfName) {
            int index = m_PQClients.IndexOfName(ConfName);
            if (index == -1)
                throw Delphi::Exception::ExceptionFrm(NOT_FOUND_CONFIGURATION_NAME, ConfName.c_str());
            return m_PQClients[index].Value();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CPQClient &CServerProcess::GetPQClient(const CString &ConfName) const {
            int index = m_PQClients.IndexOfName(ConfName);
            if (index == -1)
                throw Delphi::Exception::ExceptionFrm(NOT_FOUND_CONFIGURATION_NAME, ConfName.c_str());
            return m_PQClients[index].Value();
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQClient &CServerProcess::PQClientStart(const CString &ConfName) {
            auto &PQClient = GetPQClient(ConfName);
            m_ConfName = ConfName;
            PQClient.Active(true);
            return PQClient;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQClient &CServerProcess::PQClientStart(const CString &ConfName, const CEPoll &EPoll) {
            auto &PQClient = GetPQClient(ConfName);
            m_ConfName = ConfName;
            PQClient.AllocateEventHandlers(EPoll);
            PQClient.Active(true);
            return PQClient;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::PQClientsStart() {
            if (Config()->PostgresConnect()) {
                for (int i = 0; i < m_PQClients.Count(); i++) {
                    m_PQClients[i].Value().Active(true);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::PQClientsStop() {
            for (int i = 0; i < m_PQClients.Count(); i++) {
                m_PQClients[i].Value().Active(false);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CServerProcess::GetQuery(CPollConnection *AConnection, const CString &ConfName) {
            auto &pqClient = GetPQClient(ConfName);

            if (!pqClient.Active()) {
                pqClient.Active(true);
            }

            auto pQuery = pqClient.GetQuery();
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            pQuery->OnSendQuery([this](auto && AQuery) { DoPQSendQuery(AQuery); });
            pQuery->OnResultStatus([this](auto && AResult) { DoPQResultStatus(AResult); });
            pQuery->OnResult([this](auto && AResult, auto && AExecStatus) { DoPQResult(AResult, AExecStatus); });
#else
            pQuery->OnSendQuery(std::bind(&CServerProcess::DoPQSendQuery, this, _1));
            pQuery->OnResultStatus(std::bind(&CServerProcess::DoPQResultStatus, this, _1));
            pQuery->OnResult(std::bind(&CServerProcess::DoPQResult, this, _1, _2));
#endif
            pQuery->Binding(AConnection);

            return pQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CServerProcess::GetQuery(CPollConnection *AConnection) {
            return GetQuery(AConnection, m_ConfName);
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CServerProcess::ExecSQL(const CStringList &SQL, CPollConnection *AConnection,
                COnPQPollQueryExecutedEvent &&OnExecuted, COnPQPollQueryExceptionEvent &&OnException) {

            auto pQuery = GetQuery(AConnection);

            if (pQuery == nullptr)
                throw Delphi::Exception::Exception(_T("ExecSQL: Get SQL query failed."));

            if (OnExecuted != nullptr)
                pQuery->OnPollExecuted(static_cast<COnPQPollQueryExecutedEvent &&>(OnExecuted));

            if (OnException != nullptr)
                pQuery->OnException(static_cast<COnPQPollQueryExceptionEvent &&>(OnException));

            pQuery->SQL() = SQL;

            if (pQuery->Start() == POLL_QUERY_START_FAIL) {
                delete pQuery;
                throw Delphi::Exception::Exception(_T("ExecSQL: Start SQL query failed."));
            }

            return pQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQNotify(CPQConnection *AConnection, PGnotify *ANotify) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_NOTICE, _T("ASYNC NOTIFY of '%s' received from backend PID %d"), ANotify->relname, ANotify->be_pid);
            } else {
                Log()->Postgres(APP_LOG_NOTICE, "[%d] [%d] [postgresql://%s@%s:%s/%s] ASYNC NOTIFY of '%s' received from backend PID %d",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), ANotify->relname, ANotify->be_pid);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQReceiver(CPQConnection *AConnection, const PGresult *AResult) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_NOTICE, _T("Receiver message: %s"), PQresultErrorMessage(AResult));
            } else {
                Log()->Postgres(APP_LOG_NOTICE, "[%d] [%d] [postgresql://%s@%s:%s/%s] Receiver message: %s",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), PQresultErrorMessage(AResult));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQProcessor(CPQConnection *AConnection, LPCSTR AMessage) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_NOTICE, _T("Processor message: %s"), AMessage);
            } else {
                Log()->Postgres(APP_LOG_NOTICE, "[%d] [%d] [postgresql://%s@%s:%s/%s] Processor message: %s",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AMessage);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQConnectException(CPQConnection *AConnection, const Delphi::Exception::Exception &E) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_ERR, _T("[%d] [%d] ConnectException: %s"), AConnection->PID(), AConnection->Socket(), E.what());
            } else {
                Log()->Postgres(APP_LOG_ERR, "[%d] [%d] [postgresql://%s@%s:%s/%s] ConnectException: %s",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQClientException(CPQClient *AClient, const Delphi::Exception::Exception &E) {
            Log()->Postgres(APP_LOG_ERR, "ServerException: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQError(CPQConnection *AConnection) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_ERR, _T("[%d] [%d] Error: %s"), AConnection->PID(), AConnection->Socket(), AConnection->GetErrorMessage());
            } else {
                Log()->Postgres(APP_LOG_ERR, "[%d] [%d] [postgresql://%s@%s:%s/%s] Error: %s",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AConnection->GetErrorMessage());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQStatus(CPQConnection *AConnection) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_DEBUG, _T("[%d] [%d] Status: %s"), AConnection->PID(), AConnection->Socket(), AConnection->StatusString());
            } else {
                Log()->Postgres(APP_LOG_DEBUG, "[%d] [%d] [postgresql://%s@%s:%s/%s] Status: %s",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AConnection->StatusString());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQPollingStatus(CPQConnection *AConnection) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_DEBUG, _T("[%d] [%d] PollingStatus: %s"), AConnection->PID(), AConnection->Socket(), AConnection->PollingStatusString());
            } else {
                Log()->Postgres(APP_LOG_DEBUG, "[%d] [%d] [postgresql://%s@%s:%s/%s] PollingStatus: %s",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AConnection->PollingStatusString());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQSendQuery(CPQQuery *AQuery) {
            const auto pConnection = AQuery->Connection();
            const auto& conInfo = pConnection->ConnInfo();
            for (int i = 0; i < AQuery->SQL().Count(); ++i) {
                if (conInfo.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_DEBUG, _T("[%d] [%d] Query: %s"), pConnection->PID(), pConnection->Socket(), AQuery->SQL()[i].c_str());
                } else {
                    Log()->Postgres(APP_LOG_DEBUG, "[%d] [%d] [postgresql://%s@%s:%s/%s] Query: %s",
                                    pConnection->PID(), pConnection->Socket(),
                                    conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AQuery->SQL()[i].c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResultStatus(CPQResult *AResult) {
            const auto pConnection = AResult->Query()->Connection();
            const auto& conInfo = pConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_DEBUG, _T("[%d] [%d] ResultStatus: %s"), pConnection->PID(), pConnection->Socket(), AResult->StatusString());
            } else {
                Log()->Postgres(APP_LOG_DEBUG, "[%d] [%d] [postgresql://%s@%s:%s/%s] ResultStatus: %s",
                                pConnection->PID(), pConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AResult->StatusString());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResult(CPQResult *AResult, ExecStatusType AExecStatus) {
            const auto pConnection = AResult->Query()->Connection();
            const auto& conInfo = pConnection->ConnInfo();
#ifdef _DEBUG
            if (AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_COMMAND_OK) {
                CString jsonString;
                PQResultToJson(AResult, jsonString);
                if (conInfo.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_DEBUG, _T("[%d] [%d] Result: %s"), pConnection->PID(), pConnection->Socket(), jsonString.c_str());
                } else {
                    Log()->Postgres(APP_LOG_DEBUG, "[%d] [%d] [postgresql://%s@%s:%s/%s] Result: %s",
                                    pConnection->PID(), pConnection->Socket(),
                                    conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), jsonString.c_str());
                }
            } else {
                if (conInfo.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_ERR, _T("[%d] [%d] %s"), pConnection->PID(), pConnection->Socket(), AResult->GetErrorMessage());
                } else {
                    Log()->Postgres(APP_LOG_ERR, "[%d] [%d] [postgresql://%s@%s:%s/%s] %s",
                                    pConnection->PID(), pConnection->Socket(),
                                    conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AResult->GetErrorMessage());
                }
            }
#else
            if (!(AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_COMMAND_OK)) {
                if (conInfo.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_ERR, _T("[%d] [%d] %s"), pConnection->PID(), pConnection->Socket(), AResult->GetErrorMessage());
                } else {
                    Log()->Postgres(APP_LOG_ERR, "[%d] [%d] [postgresql://%s@%s:%s/%s] %s",
                                    pConnection->PID(), pConnection->Socket(),
                                    conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(), AResult->GetErrorMessage());
                }
            }
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQConnect(CObject *Sender) {
            auto pConnection = dynamic_cast<CPQConnection *>(Sender);
            if (pConnection != nullptr) {
                const auto& conInfo = pConnection->ConnInfo();
                if (!conInfo.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_NOTICE, "[%d] [%d] [postgresql://%s@%s:%s/%s] Connected.",
                                    pConnection->PID(), pConnection->Socket(),
                                    conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQDisconnect(CObject *Sender) {
            auto pConnection = dynamic_cast<CPQConnection *>(Sender);
            if (pConnection != nullptr) {
                const auto& conInfo = pConnection->ConnInfo();
                if (!conInfo.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_NOTICE, "[%d] [%d] [postgresql://%s@%s:%s/%s] Disconnected.",
                                    pConnection->PID(), pConnection->Socket(),
                                    conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQTimeOut(CPQConnection *AConnection) {
            const auto& conInfo = AConnection->ConnInfo();
            if (conInfo.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_WARN, _T("Connection timeout."));
            } else {
                Log()->Postgres(APP_LOG_WARN, "[%d] [%d] [postgresql://%s@%s:%s/%s] Connection timeout.",
                                AConnection->PID(), AConnection->Socket(),
                                conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
        CHTTPClientItem *CServerProcess::GetClient(const CString &Host, uint16_t Port) {
            auto pClient = m_ClientManager.Add(Host.c_str(), Port);

            pClient->ClientName() = m_Server.ServerName();

            pClient->AllocateEventHandlers(&m_EventHandlers);
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            pClient->OnVerbose([this](auto && Sender, auto && AConnection, auto && AFormat, auto && args) { DoVerbose(Sender, AConnection, AFormat, args); });
            pClient->OnException([this](auto && AConnection, auto && AException) { DoServerException(AConnection, AException); });
            pClient->OnEventHandlerException([this](auto && AHandler, auto && AException) { DoServerEventHandlerException(AHandler, AException); });
            pClient->OnConnected([this](auto && Sender) { DoClientConnected(Sender); });
            pClient->OnDisconnected([this](auto && Sender) { DoClientDisconnected(Sender); });
            pClient->OnNoCommandHandler([this](auto && Sender, auto && AData, auto && AConnection) { DoNoCommandHandler(Sender, AData, AConnection); });
#else
            pClient->OnVerbose(std::bind(&CServerProcess::DoVerbose, this, _1, _2, _3, _4));
            pClient->OnException(std::bind(&CServerProcess::DoServerException, this, _1, _2));
            pClient->OnEventHandlerException(std::bind(&CServerProcess::DoServerEventHandlerException, this, _1, _2));
            pClient->OnConnected(std::bind(&CServerProcess::DoClientConnected, this, _1));
            pClient->OnDisconnected(std::bind(&CServerProcess::DoClientDisconnected, this, _1));
            pClient->OnNoCommandHandler(std::bind(&CServerProcess::DoNoCommandHandler, this, _1, _2, _3));
#endif
            return pClient;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerListenException(CSocketEvent *Sender, const Delphi::Exception::Exception &E) {
            Log()->Error(APP_LOG_ERR, 0, "ServerListenException: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerException(CTCPConnection *AConnection, const Delphi::Exception::Exception &E) {
            Log()->Error(APP_LOG_ERR, 0, "ServerException: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerEventHandlerException(CPollEventHandler *AHandler, const Delphi::Exception::Exception &E) {
            Log()->Error(APP_LOG_ERR, 0, "ServerEventHandlerException: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerConnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPServerConnection *>(Sender);
            if (pConnection != nullptr) {
                auto pSocket = pConnection->Socket();
                if (pSocket != nullptr) {
                    auto pHandle = pSocket->Binding();
                    if (pHandle != nullptr) {
                        Log()->Notice(_T("[%s:%d] Connected."), pHandle->PeerIP(), pHandle->PeerPort());
                    } else {
                        Log()->Notice("Connected.");
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerDisconnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPServerConnection *>(Sender);
            if (pConnection != nullptr) {
                auto pSocket = pConnection->Socket();
                if (pSocket != nullptr) {
                    auto pHandle = pSocket->Binding();
                    if (pHandle != nullptr) {
                        Log()->Notice(_T("[%s:%d] Disconnected."), pHandle->PeerIP(), pHandle->PeerPort());
                    } else {
                        Log()->Notice("Disconnected.");
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoClientConnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPClientConnection *>(Sender);
            if (pConnection != nullptr) {
                auto pSocket = pConnection->Socket();
                if (pSocket != nullptr) {
                    auto pHandle = pSocket->Binding();
                    if (pHandle != nullptr) {
                        Log()->Notice(_T("[%s:%d] Client connected."), pHandle->PeerIP(), pHandle->PeerPort());
                    } else {
                        Log()->Notice(_T("Client connected."));
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoClientDisconnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPClientConnection *>(Sender);
            if (pConnection != nullptr) {
                auto pSocket = pConnection->Socket();
                if (pSocket != nullptr) {
                    auto pHandle = pSocket->Binding();
                    if (pHandle != nullptr) {
                        Log()->Notice(_T("[%s:%d] Client disconnected."), pHandle->PeerIP(), pHandle->PeerPort());
                    } else {
                        Log()->Notice("Client disconnected.");
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args) {
            Log()->Debug(APP_LOG_DEBUG_CORE, AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoAccessLog(CTCPConnection *AConnection) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);

            if (pConnection == nullptr)
                return;

            const auto &caRequest = pConnection->Request();
            const auto &caReply = pConnection->Reply();

            if (caRequest.Method.IsEmpty() || caRequest.URI.IsEmpty())
                return;

            TCHAR szTime[PATH_MAX / 4] = {0};

            time_t wtime = time(nullptr);
            struct tm *wtm = localtime(&wtime);

            if ((wtm != nullptr) && (strftime(szTime, sizeof(szTime), "%d/%b/%Y:%T %z", wtm) != 0)) {

                const auto& referer = caRequest.Headers[_T("Referer")];
                const auto& user_agent = caRequest.Headers[_T("User-Agent")];

                auto pSocket = pConnection->Socket();
                if (pSocket != nullptr) {
                    auto pHandle = pSocket->Binding();
                    if (pHandle != nullptr) {
                        Log()->Access(_T("%s %d %.3f [%s] \"%s %s HTTP/%d.%d\" %d %d \"%s\" \"%s\"\r\n"),
                                      pHandle->PeerIP(), pHandle->PeerPort(),
                                      (Now() - AConnection->Clock()) * MSecsPerDay / MSecsPerSec, szTime,
                                      caRequest.Method.c_str(), caRequest.URI.c_str(), caRequest.VMajor,
                                      caRequest.VMinor,
                                      caReply.Status, caReply.Content.Size(),
                                      referer.IsEmpty() ? "-" : referer.c_str(),
                                      user_agent.IsEmpty() ? "-" : user_agent.c_str());
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoNoCommandHandler(CSocketEvent *Sender, const CString &Data, CTCPConnection *AConnection) {
            Log()->Error(APP_LOG_ERR, 0, "No command handler: %s", Data.IsEmpty() ? "(null)" : Data.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoGet(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPost(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoOptions(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoHead(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPut(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPatch(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoDelete(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoTrace(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoConnect(CCommand *ACommand) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::LoadProviders(CProviders &Providers) {

            const CString FileName(Config()->ConfPrefix() + "oauth2.conf");

            Providers.Clear();

            if (FileExists(FileName.c_str())) {
                const auto& pathOAuth2 = Config()->Prefix() + "oauth2/";
                const auto& pathCerts = Config()->Prefix() + "certs/";

                CIniFile AuthFile(FileName.c_str());
                AuthFile.OnIniFileParseError(OnIniFileParseError);

                CStringList configFiles;
                CString configFile;

                AuthFile.ReadSectionValues("providers", &configFiles);
                for (int i = 0; i < configFiles.Count(); i++) {
                    const auto& providerName = configFiles.Names(i);

                    configFile = configFiles.ValueFromIndex(i);
                    if (!path_separator(configFile.front())) {
                        configFile = pathOAuth2 + configFile;
                    }

                    if (FileExists(configFile.c_str())) {
                        int Index = Providers.AddPair(providerName, CProvider());
                        auto& Provider = Providers[Index].Value();
                        Provider.Name() = providerName;
                        Provider.Params().LoadFromFile(configFile.c_str());
                        const auto& certsFile = pathCerts + providerName;
                        if (FileExists(certsFile.c_str()))
                            Provider.Keys().LoadFromFile(certsFile.c_str());
                        if (providerName == "default")
                            Providers.Default() = Providers[Index];
                    } else {
                        Log()->Error(APP_LOG_WARN, 0, APP_FILE_NOT_FOUND, configFile.c_str());
                    }
                }
            } else {
                Log()->Error(APP_LOG_WARN, 0, APP_FILE_NOT_FOUND, FileName.c_str());
            }

            auto& defaultProvider = Providers.Default();
            if (defaultProvider.Name().IsEmpty()) {
                defaultProvider.Name() = _T("default");

                CJSONObject web;
                web.AddPair(_T("issuers"), CJSONArray("[\"accounts.apostol-crm.ru\"]"));
                web.AddPair(_T("client_id"), _T("apostol-crm.ru"));
                web.AddPair(_T("client_secret"), _T("apostol-crm.ru"));
                web.AddPair(_T("algorithm"), _T("HS256"));
                web.AddPair(_T("auth_uri"), _T("/oauth2/authorize"));
                web.AddPair(_T("token_uri"), _T("/oauth2/token"));
                web.AddPair(_T("redirect_uris"), CJSONArray(CString().Format("[\"http://localhost:%d/oauth2/code\"]", Config()->Port())));

                auto& apps = defaultProvider.Value().Applications();
                apps.AddPair("web", web);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::LoadSites(CSites &Sites) {

            const CString FileName(Config()->ConfPrefix() + "sites.conf");

            Sites.Clear();

            if (FileExists(FileName.c_str())) {
                const CString pathSites(Config()->Prefix() + "sites/");

                CIniFile HostFile(FileName.c_str());
                HostFile.OnIniFileParseError(OnIniFileParseError);

                CStringList configFiles;
                CString configFile;

                HostFile.ReadSectionValues("hosts", &configFiles);
                for (int i = 0; i < configFiles.Count(); i++) {
                    const auto& siteName = configFiles.Names(i);

                    configFile = configFiles.ValueFromIndex(i);
                    if (!path_separator(configFile.front())) {
                        configFile = pathSites + configFile;
                    }

                    if (FileExists(configFile.c_str())) {
                        int Index = Sites.AddPair(siteName, CJSON());
                        auto& Config = Sites[Index].Value();
                        Config.LoadFromFile(configFile.c_str());
                        if (siteName == "default" || siteName == "*")
                            Sites.Default() = Sites[Index];
                    } else {
                        Log()->Error(APP_LOG_WARN, 0, APP_FILE_NOT_FOUND, configFile.c_str());
                    }
                }
            } else {
                Log()->Error(APP_LOG_WARN, 0, APP_FILE_NOT_FOUND, FileName.c_str());
            }

            auto& defaultSite = Sites.Default();
            if (defaultSite.Name().IsEmpty()) {
                defaultSite.Name() = _T("*");

                auto& configJson = defaultSite.Value().Object();

                configJson.AddPair(_T("hosts"), CJSONArray("[\"*\"]"));
                configJson.AddPair(_T("listen"), (int) Config()->Port());
                configJson.AddPair(_T("root"), Config()->DocRoot());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::LoadOAuth2(const CString &FileName, const CString &ProviderName,
                const CString &ApplicationName, CProviders &Providers) {

            int index;

            const auto& pathOAuth2 = Config()->Prefix() + "oauth2/";
            CString configFile(FileName);

            if (!path_separator(configFile.front())) {
                configFile = pathOAuth2 + configFile;
            }

            if (FileExists(configFile.c_str())) {
                CJSONObject Json;
                Json.LoadFromFile(configFile.c_str());

                index = Providers.IndexOfName(ProviderName);
                if (index == -1)
                    index = Providers.AddPair(ProviderName, CProvider(ProviderName));
                auto& Provider = Providers[index].Value();
                Provider.Applications().AddPair(ApplicationName, Json);
            } else {
                Log()->Error(APP_LOG_WARN, 0, APP_FILE_NOT_FOUND, configFile.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}

using namespace Apostol::Server;
};

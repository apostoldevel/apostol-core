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

extern "C++" {

namespace Apostol {

    namespace Server {

        //--------------------------------------------------------------------------------------------------------------

        //-- CServerProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CServerProcess::CServerProcess(): CObject(), CGlobalComponent() {

            m_pTimer = nullptr;
            m_TimerInterval = 0;

            m_PollStack.TimeOut(Config()->TimeOut());

            m_Server.PollStack(&m_PollStack);
            InitializeServerHandlers();
#ifdef WITH_POSTGRESQL
            m_PQServer.PollStack(&m_PollStack);
            InitializePQServerHandlers();
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
#ifdef WITH_POSTGRESQL
        void CServerProcess::InitializePQServerHandlers() {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            if (Config()->PostgresNotice()) {
                //m_PQServer.OnReceiver([this](auto && AConnection, auto && AResult) { DoPQReceiver(AConnection, AResult); });
                m_PQServer.OnProcessor([this](auto && AConnection, auto && AMessage) { DoPQProcessor(AConnection, AMessage); });
            }

            m_PQServer.OnConnectException([this](auto && AConnection, auto && AException) { DoPQConnectException(AConnection, AException); });
            m_PQServer.OnServerException([this](auto && AServer, auto && AException) { DoPQServerException(AServer, AException); });

            m_PQServer.OnEventHandlerException([this](auto && AHandler, auto && AException) { DoServerEventHandlerException(AHandler, AException); });

            m_PQServer.OnNotify([this](auto && AConnection, auto && ANotify) { DoPQNotify(AConnection, ANotify); });

            m_PQServer.OnError([this](auto && AConnection) { DoPQError(AConnection); });
            m_PQServer.OnStatus([this](auto && AConnection) { DoPQStatus(AConnection); });
            m_PQServer.OnPollingStatus([this](auto && AConnection) { DoPQPollingStatus(AConnection); });

            m_PQServer.OnConnected([this](auto && Sender) { DoPQConnect(Sender); });
            m_PQServer.OnDisconnected([this](auto && Sender) { DoPQDisconnect(Sender); });
#else
            if (Config()->PostgresNotice()) {
                //m_PQServer.OnReceiver(std::bind(&CServerProcess::DoPQReceiver, this, _1, _2));
                m_PQServer.OnProcessor(std::bind(&CServerProcess::DoPQProcessor, this, _1, _2));
            }

            m_PQServer.OnConnectException(std::bind(&CServerProcess::DoPQConnectException, this, _1, _2));
            m_PQServer.OnServerException(std::bind(&CServerProcess::DoPQServerException, this, _1, _2));

            m_PQServer.OnEventHandlerException(std::bind(&CServerProcess::DoServerEventHandlerException, this, _1, _2));

            m_PQServer.OnNotify(std::bind(&CServerProcess::DoPQNotify, this, _1, _2));

            m_PQServer.OnError(std::bind(&CServerProcess::DoPQError, this, _1));
            m_PQServer.OnStatus(std::bind(&CServerProcess::DoPQStatus, this, _1));
            m_PQServer.OnPollingStatus(std::bind(&CServerProcess::DoPQPollingStatus, this, _1));

            m_PQServer.OnConnected(std::bind(&CServerProcess::DoPQConnect, this, _1));
            m_PQServer.OnDisconnected(std::bind(&CServerProcess::DoPQDisconnect, this, _1));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
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

        void CServerProcess::InitializeServer(const CString &Title) {
            m_Server.ServerName() = Title;

            m_Server.DefaultIP() = Config()->Listen();
            m_Server.DefaultPort(Config()->Port());

            LoadSites(m_Server.Sites());
            LoadProviders(m_Server.Providers());

            m_Server.InitializeBindings();
            m_Server.ActiveLevel(alBinding);
        }
        //--------------------------------------------------------------------------------------------------------------

#ifdef WITH_POSTGRESQL
        void CServerProcess::InitializePQServer(const CString &Title) {
            m_PQServer.ConnInfo().ApplicationName() = "'" + Title + "'"; //application_name;
            m_PQServer.SizeMin(Config()->PostgresPollMin());
            m_PQServer.SizeMax(Config()->PostgresPollMax());
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
        void CServerProcess::PQServerStart(const CString &Name) {
            if (Config()->PostgresConnect()) {
                m_PQServer.ConnInfo().SetParameters(Config()->PostgresConnInfo()[Name]);
                m_PQServer.Active(true);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::PQServerStop() {
            m_PQServer.Active(false);
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CServerProcess::GetQuery(CPollConnection *AConnection) {
            CPQPollQuery *pQuery = nullptr;

            if (m_PQServer.Active()) {
                pQuery = m_PQServer.GetQuery();
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                pQuery->OnSendQuery([this](auto && AQuery) { DoPQSendQuery(AQuery); });
                pQuery->OnResultStatus([this](auto && AResult) { DoPQResultStatus(AResult); });
                pQuery->OnResult([this](auto && AResult, auto && AExecStatus) { DoPQResult(AResult, AExecStatus); });
#else
                pQuery->OnSendQuery(std::bind(&CServerProcess::DoPQSendQuery, this, _1));
                pQuery->OnResultStatus(std::bind(&CServerProcess::DoPQResultStatus, this, _1));
                pQuery->OnResult(std::bind(&CServerProcess::DoPQResult, this, _1, _2));
#endif
                pQuery->PollConnection(AConnection);
            }

            return pQuery;
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

            if (pQuery->Start() == POLL_QUERY_START_ERROR) {
                delete pQuery;
                throw Delphi::Exception::Exception(_T("ExecSQL: Start SQL query failed."));
            }

            if (AConnection != nullptr)
                AConnection->CloseConnection(false);

            return pQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQNotify(CPQConnection *AConnection, PGnotify *ANotify) {
            const auto& Info = AConnection->ConnInfo();
            if (Info.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_NOTICE, _T("ASYNC NOTIFY of '%s' received from backend PID %d"), ANotify->relname, ANotify->be_pid);
            } else {
                Log()->Postgres(APP_LOG_NOTICE, "[%d] [postgresql://%s@%s:%s/%s] ASYNC NOTIFY of '%s' received from backend PID %d", AConnection->Socket(),
                                Info["user"].c_str(), Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str(), ANotify->relname, ANotify->be_pid);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQReceiver(CPQConnection *AConnection, const PGresult *AResult) {
            const auto& Info = AConnection->ConnInfo();
            if (Info.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_NOTICE, _T("Receiver message: %s"), PQresultErrorMessage(AResult));
            } else {
                Log()->Postgres(APP_LOG_NOTICE, "[%d] [postgresql://%s@%s:%s/%s] Receiver message: %s", AConnection->Socket(),
                                Info["user"].c_str(), Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str(), PQresultErrorMessage(AResult));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQProcessor(CPQConnection *AConnection, LPCSTR AMessage) {
            const auto& Info = AConnection->ConnInfo();
            if (Info.ConnInfo().IsEmpty()) {
                Log()->Postgres(APP_LOG_NOTICE, _T("Processor message: %s"), AMessage);
            } else {
                Log()->Postgres(APP_LOG_NOTICE, "[%d] [postgresql://%s@%s:%s/%s] Processor message: %s", AConnection->Socket(),
                                Info["user"].c_str(), Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str(), AMessage);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQConnectException(CPQConnection *AConnection, const Delphi::Exception::Exception &E) {
            Log()->Postgres(APP_LOG_ERR, "ConnectException: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQServerException(CPQServer *AServer, const Delphi::Exception::Exception &E) {
            Log()->Postgres(APP_LOG_ERR, "ServerException: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQError(CPQConnection *AConnection) {
            Log()->Postgres(APP_LOG_ERR, "Error: %s", AConnection->GetErrorMessage());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQStatus(CPQConnection *AConnection) {
            Log()->Postgres(APP_LOG_DEBUG, "Status: %s", AConnection->StatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQPollingStatus(CPQConnection *AConnection) {
            Log()->Postgres(APP_LOG_DEBUG, "PollingStatus: %s", AConnection->PollingStatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQSendQuery(CPQQuery *AQuery) {
            for (int I = 0; I < AQuery->SQL().Count(); ++I) {
                Log()->Postgres(APP_LOG_DEBUG, "SendQuery: %s", AQuery->SQL()[I].c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResultStatus(CPQResult *AResult) {
            Log()->Postgres(APP_LOG_DEBUG, "ResultStatus: %s", AResult->StatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResult(CPQResult *AResult, ExecStatusType AExecStatus) {
#ifdef _DEBUG
            if (AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_COMMAND_OK) {
                if (AResult->nTuples() > 0) {

                    CString Print;

                    Print = "(";
                    Print += AResult->fName(0);
                    for (int I = 1; I < AResult->nFields(); ++I) {
                        Print += ",";
                        Print += AResult->fName(I);
                    }
                    Print += ")";

                    Log()->Postgres(APP_LOG_DEBUG, "%s", Print.c_str());

                    Print = "(";
                    for (int Row = 0; Row < AResult->nTuples(); ++Row) {

                        if (AResult->GetIsNull(Row, 0)) {
                            Print += "<null>";
                        } else {
                            if (AResult->fFormat(0) == 0) {
                                Print += AResult->GetValue(Row, 0);
                            } else {
                                Print += "<binary>";
                            }
                        }

                        for (int Col = 1; Col < AResult->nFields(); ++Col) {
                            Print += ",";
                            if (AResult->GetIsNull(Row, Col)) {
                                Print += "<null>";
                            } else {
                                if (AResult->fFormat(Col) == 0) {
                                    Print += AResult->GetValue(Row, Col);
                                } else {
                                    Print += "<binary>";
                                }
                            }
                        }

                        break;
                    }

                    Print += ")";

                    Log()->Postgres(APP_LOG_DEBUG, "%s", Print.c_str());
                }
            } else {
                Log()->Postgres(APP_LOG_ERR, "%s", AResult->GetErrorMessage());
            }
#else
            if (!(AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_SINGLE_TUPLE)) {
                Log()->Postgres(APP_LOG_ERR, "%s", AResult->GetErrorMessage());
            }
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQConnect(CObject *Sender) {
            auto pConnection = dynamic_cast<CPQConnection *>(Sender);
            if (pConnection != nullptr) {
                const auto& Info = pConnection->ConnInfo();
                if (!Info.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_INFO, "[%d] [postgresql://%s@%s:%s/%s] Connected.", pConnection->PID(),
                                    Info["user"].c_str(), Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQDisconnect(CObject *Sender) {
            auto pConnection = dynamic_cast<CPQConnection *>(Sender);
            if (pConnection != nullptr) {
                const auto& Info = pConnection->ConnInfo();
                if (!Info.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_INFO, "[%d] [postgresql://%s@%s:%s/%s] Disconnected.", pConnection->PID(),
                                    Info["user"].c_str(), Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
        void CServerProcess::DebugRequest(CHTTPRequest *ARequest) {
            DebugMessage("[%p] Request:\n%s %s HTTP/%d.%d\n", ARequest, ARequest->Method.c_str(), ARequest->URI.c_str(), ARequest->VMajor, ARequest->VMinor);

            for (int i = 0; i < ARequest->Headers.Count(); i++)
                DebugMessage("%s: %s\n", ARequest->Headers[i].Name().c_str(), ARequest->Headers[i].Value().c_str());

            if (!ARequest->Content.IsEmpty())
                DebugMessage("\n%s\n", ARequest->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DebugReply(CHTTPReply *AReply) {
            DebugMessage("[%p] Reply:\nHTTP/%d.%d %d %s\n", AReply, AReply->VMajor, AReply->VMinor, AReply->Status, AReply->StatusText.c_str());

            for (int i = 0; i < AReply->Headers.Count(); i++)
                DebugMessage("%s: %s\n", AReply->Headers[i].Name().c_str(), AReply->Headers[i].Value().c_str());

            if (!AReply->Content.IsEmpty())
                DebugMessage("\n%s\n", AReply->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPClientItem *CServerProcess::GetClient(const CString &Host, uint16_t Port) {
            auto pClient = m_ClientManager.Add(Host.c_str(), Port);

            pClient->ClientName() = m_Server.ServerName();

            pClient->PollStack(&m_PollStack);
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
#ifdef WITH_POSTGRESQL
            auto pPollQuery = m_PQServer.FindQueryByConnection(AConnection);
            if (pPollQuery != nullptr) {
                pPollQuery->PollConnection(nullptr);
            }
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerEventHandlerException(CPollEventHandler *AHandler, const Delphi::Exception::Exception &E) {
            Log()->Error(APP_LOG_ERR, 0, "ServerEventHandlerException: %s", E.what());
#ifdef WITH_POSTGRESQL
            auto pConnection = dynamic_cast<CHTTPServerConnection *>(AHandler->Binding());
            if (pConnection != nullptr) {
                auto pPollQuery = m_PQServer.FindQueryByConnection(pConnection);
                if (pPollQuery != nullptr) {
                    pPollQuery->PollConnection(nullptr);
                }
            }
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerConnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPServerConnection *>(Sender);
            if (pConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client opened connection."), pConnection->Socket()->Binding()->PeerIP(),
                               pConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerDisconnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPServerConnection *>(Sender);
            if (pConnection != nullptr) {
#ifdef WITH_POSTGRESQL
                auto pPollQuery = m_PQServer.FindQueryByConnection(pConnection);
                if (pPollQuery != nullptr) {
                    pPollQuery->PollConnection(nullptr);
                }
#endif
                Log()->Message(_T("Client disconnected."));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoClientConnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPClientConnection *>(Sender);
            if (pConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client connected."), pConnection->Socket()->Binding()->PeerIP(),
                               pConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoClientDisconnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPClientConnection *>(Sender);
            if (pConnection != nullptr) {
                if (pConnection->ClosedGracefully()) {
                    auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());
                    if (pClient != nullptr) {
                        Log()->Message("[%s:%d] Client closed connection.", pClient->Host().c_str(), pClient->Port());
                    }
                } else {
                    Log()->Message(_T("[%s:%d] Client disconnected."), pConnection->Socket()->Binding()->PeerIP(),
                                   pConnection->Socket()->Binding()->PeerPort());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args) {
            Log()->Debug(0, AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoAccessLog(CTCPConnection *AConnection) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);

            if (pConnection == nullptr)
                return;

            if (pConnection->ClosedGracefully())
                return;

            auto pRequest = pConnection->Request();
            auto pReply = pConnection->Reply();

            if (pRequest->Method.IsEmpty())
                return;

            TCHAR szTime[PATH_MAX / 4] = {0};

            time_t wtime = time(nullptr);
            struct tm *wtm = localtime(&wtime);

            if ((wtm != nullptr) && (strftime(szTime, sizeof(szTime), "%d/%b/%Y:%T %z", wtm) != 0)) {

                const auto& referer = pRequest->Headers[_T("Referer")];
                const auto& user_agent = pRequest->Headers[_T("User-Agent")];

                auto pBinding = pConnection->Socket()->Binding();
                if (pBinding != nullptr) {
                    Log()->Access(_T("%s %d %.3f [%s] \"%s %s HTTP/%d.%d\" %d %d \"%s\" \"%s\"\r\n"),
                                  pBinding->PeerIP(), pBinding->PeerPort(),
                                  (Now() - AConnection->Clock()) * MSecsPerDay / MSecsPerSec, szTime,
                                  pRequest->Method.c_str(), pRequest->URI.c_str(), pRequest->VMajor, pRequest->VMinor,
                                  pReply->Status, pReply->Content.Size(),
                                  referer.IsEmpty() ? "-" : referer.c_str(),
                                  user_agent.IsEmpty() ? "-" : user_agent.c_str());
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

        void CServerProcess::FetchAccessToken(const CString &URI, const CString &Assertion,
                COnSocketExecuteEvent && OnDone, COnSocketExceptionEvent && OnFailed) {

            auto OnRequest = [](CHTTPClient *Sender, CHTTPRequest *ARequest) {

                const auto &token_uri = Sender->Data()["token_uri"];
                const auto &grant_type = Sender->Data()["grant_type"];
                const auto &assertion = Sender->Data()["assertion"];

                ARequest->Content = _T("grant_type=");
                ARequest->Content << CHTTPServer::URLEncode(grant_type);

                ARequest->Content << _T("&assertion=");
                ARequest->Content << CHTTPServer::URLEncode(assertion);

                CHTTPRequest::Prepare(ARequest, _T("POST"), token_uri.c_str(), _T("application/x-www-form-urlencoded"));

                DebugRequest(ARequest);
            };

            auto OnException = [](CTCPConnection *Sender, const Delphi::Exception::Exception &E) {

                auto pConnection = dynamic_cast<CHTTPClientConnection *> (Sender);
                auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());

                DebugReply(pConnection->Reply());

                Log()->Error(APP_LOG_ERR, 0, "[%s:%d] %s", pClient->Host().c_str(), pClient->Port(), E.what());
            };

            CLocation token_uri(URI);

            auto pClient = GetClient(token_uri.hostname, token_uri.port);

            pClient->Data().Values("token_uri", token_uri.pathname);
            pClient->Data().Values("grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer");
            pClient->Data().Values("assertion", Assertion);

            pClient->OnRequest(OnRequest);
            pClient->OnExecute(static_cast<COnSocketExecuteEvent &&>(OnDone));
            pClient->OnException(OnFailed == nullptr ? OnException : static_cast<COnSocketExceptionEvent &&>(OnFailed));

            pClient->Active(true);
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
                        Provider.Name = providerName;
                        Provider.Params.LoadFromFile(configFile.c_str());
                        const auto& certsFile = pathCerts + providerName;
                        if (FileExists(certsFile.c_str()))
                            Provider.Keys.LoadFromFile(certsFile.c_str());
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
                web.AddPair(_T("issuers"), CJSONArray("[\"accounts.apostol-web-service.ru\"]"));
                web.AddPair(_T("client_id"), _T("apostol-web-service.ru"));
                web.AddPair(_T("client_secret"), _T("apostol-web-service.ru"));
                web.AddPair(_T("algorithm"), _T("HS256"));
                web.AddPair(_T("auth_uri"), _T("/oauth2/authorize"));
                web.AddPair(_T("token_uri"), _T("/oauth2/token"));
                web.AddPair(_T("redirect_uris"), CJSONArray(CString().Format("[\"http://localhost:%d/oauth2/code\"]", Config()->Port())));

                auto& apps = defaultProvider.Value().Params.Object();
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

    }
}

using namespace Apostol::Server;
};

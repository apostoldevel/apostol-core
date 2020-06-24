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

                CCommandHandler *LCommand;

                AHandlers->ParseParamsDefault(false);
                AHandlers->DisconnectDefault(ADisconnect);
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                LCommand =AHandlers->Add();
                LCommand->Command(_T("GET"));
                LCommand->OnCommand([this](auto && ACommand) { DoGet(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("POST"));
                LCommand->OnCommand([this](auto && ACommand) { DoPost(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("OPTIONS"));
                LCommand->OnCommand([this](auto && ACommand) { DoOptions(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("PUT"));
                LCommand->OnCommand([this](auto && ACommand) { DoPut(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("DELETE"));
                LCommand->OnCommand([this](auto && ACommand) { DoDelete(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("HEAD"));
                LCommand->OnCommand([this](auto && ACommand) { DoHead(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("PATCH"));
                LCommand->OnCommand([this](auto && ACommand) { DoPatch(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("TRACE"));
                LCommand->OnCommand([this](auto && ACommand) { DoTrace(ACommand); });

                LCommand = AHandlers->Add();
                LCommand->Command(_T("CONNECT"));
                LCommand->OnCommand([this](auto && ACommand) { DoConnect(ACommand); });
#else
                LCommand =AHandlers->Add();
                LCommand->Command(_T("GET"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoGet, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("POST"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoPost, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("OPTIONS"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoOptions, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("PUT"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoPut, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("DELETE"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoDelete, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("HEAD"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoHead, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("PATCH"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoPatch, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("TRACE"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoTrace, this, _1));

                LCommand = AHandlers->Add();
                LCommand->Command(_T("CONNECT"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoConnect, this, _1));
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

        void CServerProcess::InitializeWorkerServer(const CString &Title) {
            m_Server.ServerName() = Title;

            m_Server.DefaultIP() = Config()->Listen();
            m_Server.DefaultPort(Config()->WorkerPort());

            LoadSites(m_Server.Sites());
            LoadAuthParams(m_Server.AuthParams());

            m_Server.InitializeBindings();
            m_Server.ActiveLevel(alBinding);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::InitializeHelperServer(const CString &Title) {
            m_Server.ServerName() = Title;

            if (Config()->HelperPort() != 0) {
                m_Server.DefaultIP() = Config()->Listen();
                m_Server.DefaultPort(Config()->HelperPort());

                LoadSites(m_Server.Sites());
                LoadAuthParams(m_Server.AuthParams());

                m_Server.InitializeBindings();
                m_Server.ActiveLevel(alBinding);
            }
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
#ifdef WITH_POSTGRESQL
        void CServerProcess::PQServerStart() {
            if (Config()->PostgresConnect()) {
                m_PQServer.ConnInfo().SetParameters(Config()->PostgresConnInfo());
                m_PQServer.Active(true);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::PQServerStop() {
            m_PQServer.Active(false);
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CServerProcess::GetQuery(CPollConnection *AConnection) {
            CPQPollQuery *LQuery = nullptr;

            if (m_PQServer.Active()) {
                LQuery = m_PQServer.GetQuery();
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                LQuery->OnSendQuery([this](auto && AQuery) { DoPQSendQuery(AQuery); });
                LQuery->OnResultStatus([this](auto && AResult) { DoPQResultStatus(AResult); });
                LQuery->OnResult([this](auto && AResult, auto && AExecStatus) { DoPQResult(AResult, AExecStatus); });
#else
                LQuery->OnSendQuery(std::bind(&CServerProcess::DoPQSendQuery, this, _1));
                LQuery->OnResultStatus(std::bind(&CServerProcess::DoPQResultStatus, this, _1));
                LQuery->OnResult(std::bind(&CServerProcess::DoPQResult, this, _1, _2));
#endif
                LQuery->PollConnection(AConnection);
            }

            return LQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CServerProcess::ExecSQL(const CStringList &SQL, CPollConnection *AConnection,
                COnPQPollQueryExecutedEvent &&OnExecuted, COnPQPollQueryExceptionEvent &&OnException) {

            auto LQuery = GetQuery(AConnection);

            if (LQuery == nullptr)
                throw Delphi::Exception::Exception("ExecSQL: GetQuery() failed!");

            if (OnExecuted != nullptr)
                LQuery->OnPollExecuted(static_cast<COnPQPollQueryExecutedEvent &&>(OnExecuted));

            if (OnException != nullptr)
                LQuery->OnException(static_cast<COnPQPollQueryExceptionEvent &&>(OnException));

            LQuery->SQL() = SQL;

            if (LQuery->Start() != POLL_QUERY_START_ERROR) {
                return true;
            } else {
                delete LQuery;
            }

            Log()->Error(APP_LOG_ALERT, 0, "ExecSQL: Start() failed!");

            return false;
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

        void CServerProcess::DoPQConnectException(CPQConnection *AConnection, Delphi::Exception::Exception *AException) {
            Log()->Postgres(APP_LOG_EMERG, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQServerException(CPQServer *AServer, Delphi::Exception::Exception *AException) {
            Log()->Postgres(APP_LOG_EMERG, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQError(CPQConnection *AConnection) {
            Log()->Postgres(APP_LOG_EMERG, AConnection->GetErrorMessage());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQStatus(CPQConnection *AConnection) {
            Log()->Postgres(APP_LOG_DEBUG, AConnection->StatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQPollingStatus(CPQConnection *AConnection) {
            Log()->Postgres(APP_LOG_DEBUG, AConnection->PollingStatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQSendQuery(CPQQuery *AQuery) {
            for (int I = 0; I < AQuery->SQL().Count(); ++I) {
                Log()->Postgres(APP_LOG_INFO, AQuery->SQL()[I].c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResultStatus(CPQResult *AResult) {
            Log()->Postgres(APP_LOG_DEBUG, AResult->StatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResult(CPQResult *AResult, ExecStatusType AExecStatus) {
#ifdef _DEBUG
            if (AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_SINGLE_TUPLE) {
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
                Log()->Postgres(APP_LOG_EMERG, AResult->GetErrorMessage());
            }
#else
            if (!(AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_SINGLE_TUPLE)) {
                Log()->Postgres(APP_LOG_EMERG, AResult->GetErrorMessage());
            }
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQConnect(CObject *Sender) {
            auto LConnection = dynamic_cast<CPQConnection *>(Sender);
            if (LConnection != nullptr) {
                const auto& Info = LConnection->ConnInfo();
                if (!Info.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_EMERG, "[%d] [postgresql://%s@%s:%s/%s] Connected.", LConnection->PID(),
                                    Info["user"].c_str(), Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQDisconnect(CObject *Sender) {
            auto LConnection = dynamic_cast<CPQConnection *>(Sender);
            if (LConnection != nullptr) {
                const auto& Info = LConnection->ConnInfo();
                if (!Info.ConnInfo().IsEmpty()) {
                    Log()->Postgres(APP_LOG_EMERG, "[%d] [postgresql://%s@%s:%s/%s] Disconnected.", LConnection->PID(),
                                    Info["user"].c_str(), Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
        void CServerProcess::DebugRequest(CRequest *ARequest) {
            DebugMessage("[%p] Request:\n%s %s HTTP/%d.%d\n", ARequest, ARequest->Method.c_str(), ARequest->URI.c_str(), ARequest->VMajor, ARequest->VMinor);

            for (int i = 0; i < ARequest->Headers.Count(); i++)
                DebugMessage("%s: %s\n", ARequest->Headers[i].Name().c_str(), ARequest->Headers[i].Value().c_str());

            if (!ARequest->Content.IsEmpty())
                DebugMessage("\n%s\n", ARequest->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DebugReply(CReply *AReply) {
            DebugMessage("[%p] Reply:\nHTTP/%d.%d %d %s\n", AReply, AReply->VMajor, AReply->VMinor, AReply->Status, AReply->StatusText.c_str());

            for (int i = 0; i < AReply->Headers.Count(); i++)
                DebugMessage("%s: %s\n", AReply->Headers[i].Name().c_str(), AReply->Headers[i].Value().c_str());

            if (!AReply->Content.IsEmpty())
                DebugMessage("\n%s\n", AReply->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPClient *CServerProcess::GetClient(const CString &Host, uint16_t Port) {
            auto LClient = new CHTTPClient(Host.c_str(), Port);

            LClient->ClientName() = m_Server.ServerName();

            LClient->PollStack(m_Server.PollStack());
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            LClient->OnVerbose([this](auto && Sender, auto && AConnection, auto && AFormat, auto && args) { DoVerbose(Sender, AConnection, AFormat, args); });
            LClient->OnException([this](auto && AConnection, auto && AException) { DoServerException(AConnection, AException); });
            LClient->OnEventHandlerException([this](auto && AHandler, auto && AException) { DoServerEventHandlerException(AHandler, AException); });
            LClient->OnConnected([this](auto && Sender) { DoClientConnected(Sender); });
            LClient->OnDisconnected([this](auto && Sender) { DoClientDisconnected(Sender); });
            LClient->OnNoCommandHandler([this](auto && Sender, auto && AData, auto && AConnection) { DoNoCommandHandler(Sender, AData, AConnection); });
#else
            LClient->OnVerbose(std::bind(&CServerProcess::DoVerbose, this, _1, _2, _3, _4));
            LClient->OnException(std::bind(&CServerProcess::DoServerException, this, _1, _2));
            LClient->OnEventHandlerException(std::bind(&CServerProcess::DoServerEventHandlerException, this, _1, _2));
            LClient->OnConnected(std::bind(&CServerProcess::DoClientConnected, this, _1));
            LClient->OnDisconnected(std::bind(&CServerProcess::DoClientDisconnected, this, _1));
            LClient->OnNoCommandHandler(std::bind(&CServerProcess::DoNoCommandHandler, this, _1, _2, _3));
#endif
            return LClient;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerListenException(CSocketEvent *Sender, Delphi::Exception::Exception *AException) {
            Log()->Error(APP_LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerException(CTCPConnection *AConnection,
                                               Delphi::Exception::Exception *AException) {
            Log()->Error(APP_LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerEventHandlerException(CPollEventHandler *AHandler,
                                                           Delphi::Exception::Exception *AException) {
            Log()->Error(APP_LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerConnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *>(Sender);
            if (LConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client opened connection."), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerDisconnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *>(Sender);
            if (LConnection != nullptr) {
#ifdef WITH_POSTGRESQL
                auto LPollQuery = m_PQServer.FindQueryByConnection(LConnection);
                if (LPollQuery != nullptr) {
                    LPollQuery->PollConnection(nullptr);
                }
#endif
                Log()->Message(_T("[%s:%d] Client closed connection."), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoClientConnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPClientConnection *>(Sender);
            if (LConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client connected."), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoClientDisconnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPClientConnection *>(Sender);
            if (LConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client disconnected."), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat,
                                       va_list args) {
            Log()->Debug(0, AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoAccessLog(CTCPConnection *AConnection) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);

            if (LConnection == nullptr || LConnection->Protocol() == pWebSocket)
                return;

            auto LRequest = LConnection->Request();
            auto LReply = LConnection->Reply();

            TCHAR szTime[PATH_MAX / 4] = {0};

            time_t wtime = time(nullptr);
            struct tm *wtm = localtime(&wtime);

            if ((wtm != nullptr) && (strftime(szTime, sizeof(szTime), "%d/%b/%Y:%T %z", wtm) != 0)) {

                const CString &LReferer = LRequest->Headers.Values(_T("Referer"));
                const CString &LUserAgent = LRequest->Headers.Values(_T("User-Agent"));

                auto LBinding = LConnection->Socket()->Binding();
                if (LBinding != nullptr) {
                    Log()->Access(_T("%s %d %8.2f ms [%s] \"%s %s HTTP/%d.%d\" %d %d \"%s\" \"%s\"\r\n"),
                                  LBinding->PeerIP(), LBinding->PeerPort(),
                                  double((clock() - AConnection->Tag()) / (double) CLOCKS_PER_SEC * 1000), szTime,
                                  LRequest->Method.c_str(), LRequest->URI.c_str(), LRequest->VMajor, LRequest->VMinor,
                                  LReply->Status, LReply->Content.Size(),
                                  LReferer.IsEmpty() ? "-" : LReferer.c_str(),
                                  LUserAgent.IsEmpty() ? "-" : LUserAgent.c_str());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoNoCommandHandler(CSocketEvent *Sender, LPCTSTR AData, CTCPConnection *AConnection) {
            Log()->Error(APP_LOG_EMERG, 0, "No command handler: %s", AData);
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

        void CServerProcess::LoadAuthParams(CAuthParams &AuthParams) {

            const CString FileName(Config()->ConfPrefix() + "auth.conf");

            auto OnIniFileParseError = [&FileName](Pointer Sender, LPCTSTR lpszSectionName, LPCTSTR lpszKeyName,
                                                   LPCTSTR lpszValue, LPCTSTR lpszDefault, int Line)
            {
                if ((lpszValue == nullptr) || (lpszValue[0] == '\0')) {
                    if ((lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                        Log()->Error(APP_LOG_EMERG, 0, ConfMsgEmpty, lpszSectionName, lpszKeyName, FileName.c_str(), Line);
                } else {
                    if ((lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                        Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue, lpszSectionName, lpszKeyName, lpszValue,
                                     FileName.c_str(), Line);
                    else
                        Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue _T(" - ignored and set by default: \"%s\""), lpszSectionName, lpszKeyName, lpszValue,
                                     FileName.c_str(), Line, lpszDefault);
                }
            };

            AuthParams.Clear();

            if (FileExists(FileName.c_str())) {
                const auto& pathAuth = Config()->Prefix() + "auth/";
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
                        configFile = pathAuth + configFile;
                    }

                    if (FileExists(configFile.c_str())) {
                        int Index = AuthParams.AddPair(providerName, CAuthParam());
                        auto& Param = AuthParams[Index].Value();
                        Param.Provider = providerName;
                        Param.Params.LoadFromFile(configFile.c_str());
                        const auto& certsFile = pathCerts + providerName;
                        if (FileExists(certsFile.c_str()))
                            Param.Keys.LoadFromFile(certsFile.c_str());
                        if (providerName == "default")
                            AuthParams.Default() = AuthParams[Index];
                    } else {
                        Log()->Error(APP_LOG_EMERG, 0, APP_FILE_NOT_FOUND, configFile.c_str());
                    }
                }
            } else {
                Log()->Error(APP_LOG_EMERG, 0, APP_FILE_NOT_FOUND, FileName.c_str());
            }

            auto& defaultAuth = AuthParams.Default();
            if (defaultAuth.Name().IsEmpty()) {
                defaultAuth.Name() = _T("default");
                auto& Params = defaultAuth.Value().Params.Object();
                Params.AddPair(_T("issuers"), CJSONArray("apostol-web-service.ru"));
                Params.AddPair(_T("audience"), _T("apostol-web-service.ru"));
                Params.AddPair(_T("algorithm"), _T("HS256"));
                Params.AddPair(_T("auth_uri"), _T("/oauth2/auth"));
                Params.AddPair(_T("token_uri"), _T("/oauth2/token"));
                Params.AddPair(_T("redirect_uri"), CJSONArray("http://localhost:4977/oauth2/code"));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::LoadSites(CSites &Sites) {

            const CString FileName(Config()->ConfPrefix() + "sites.conf");

            auto OnIniFileParseError = [&FileName](Pointer Sender, LPCTSTR lpszSectionName, LPCTSTR lpszKeyName,
                                                   LPCTSTR lpszValue, LPCTSTR lpszDefault, int Line)
            {
                if ((lpszValue == nullptr) || (lpszValue[0] == '\0')) {
                    if ((lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                        Log()->Error(APP_LOG_EMERG, 0, ConfMsgEmpty, lpszSectionName, lpszKeyName, FileName.c_str(), Line);
                } else {
                    if ((lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                        Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue, lpszSectionName, lpszKeyName, lpszValue,
                                     FileName.c_str(), Line);
                    else
                        Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue _T(" - ignored and set by default: \"%s\""), lpszSectionName, lpszKeyName, lpszValue,
                                     FileName.c_str(), Line, lpszDefault);
                }
            };

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
                        if (siteName == "default")
                            Sites.Default() = Sites[Index];
                    } else {
                        Log()->Error(APP_LOG_EMERG, 0, APP_FILE_NOT_FOUND, configFile.c_str());
                    }
                }
            } else {
                Log()->Error(APP_LOG_EMERG, 0, APP_FILE_NOT_FOUND, FileName.c_str());
            }

            auto& defaultSite = Sites.Default();
            if (defaultSite.Name().IsEmpty()) {
                defaultSite.Name() = _T("*");
                auto& configJson = defaultSite.Value().Object();
                configJson.AddPair(_T("hosts"), CJSONArray("[\"*\"]"));
                configJson.AddPair(_T("listen"), (int) Config()->WorkerPort());
                configJson.AddPair(_T("root"), Config()->DocRoot());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}

using namespace Apostol::Server;
};

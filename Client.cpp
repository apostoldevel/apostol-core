/*++

Program name:

 apostol-core

Module Name:

  Client.cpp

Notices:

  WebSocket Client

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Client.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Client {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebSocketMessageHandler ----------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWebSocketMessageHandler::CWebSocketMessageHandler(CWebSocketMessageHandlerManager *AManager,
                                                           const CWSMessage &Message, COnMessageHandlerEvent &&Handler): CCollectionItem(AManager), m_Handler(Handler) {
            m_Message = Message;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebSocketMessageHandler::Handler(CWebSocketConnection *AConnection) {
            if (m_Handler != nullptr) {
                m_Handler(this, AConnection);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebSocketMessageHandlerManager ---------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWebSocketMessageHandler *CWebSocketMessageHandlerManager::Get(int Index) const {
            return dynamic_cast<CWebSocketMessageHandler *> (inherited::GetItem(Index));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebSocketMessageHandlerManager::Set(int Index, CWebSocketMessageHandler *Value) {
            inherited::SetItem(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        CWebSocketMessageHandler *CWebSocketMessageHandlerManager::Add(CWebSocketConnection *AConnection,
                                                                       const CWSMessage &Message, COnMessageHandlerEvent &&Handler, uint32_t Key) {

            if (AConnection != nullptr && AConnection->Connected() && !AConnection->ClosedGracefully()) {
                auto pHandler = new CWebSocketMessageHandler(this, Message, static_cast<COnMessageHandlerEvent &&> (Handler));

                CString sResult;
                CWSProtocol::Response(Message, sResult);

                auto &WSReply = AConnection->WSReply();

                WSReply.Clear();
                WSReply.SetPayload(sResult, Key);

                AConnection->SendWebSocket(true);

                return pHandler;
            }

            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CWebSocketMessageHandler *CWebSocketMessageHandlerManager::FindMessageById(const CString &Value) const {
            CWebSocketMessageHandler *pHandler;

            for (int i = 0; i < Count(); ++i) {
                pHandler = Get(i);
                if (pHandler->Message().UniqueId == Value)
                    return pHandler;
            }

            return nullptr;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomWebSocketClient ------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCustomWebSocketClient::CCustomWebSocketClient(): CHTTPClient(), CGlobalComponent() {
            m_pConnection = nullptr;
            m_pTimer = nullptr;
            m_Key = GetUID(16);
            m_ErrorCount = -1;
            m_TimerInterval = 0;
            m_UpdateConnected = false;
            m_OnMessage = nullptr;
            m_OnError = nullptr;
            m_OnHeartbeat = nullptr;
            m_OnTimeOut = nullptr;
            m_OnWebSocketError = nullptr;
            m_OnProtocolError = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CCustomWebSocketClient::CCustomWebSocketClient(const CLocation &URI): CHTTPClient(URI.hostname, URI.port), CGlobalComponent(), m_URI(URI) {
            m_pConnection = nullptr;
            m_pTimer = nullptr;
            m_Key = GetUID(16);
            m_ErrorCount = -1;
            m_TimerInterval = 0;
            m_UpdateConnected = false;
            m_OnMessage = nullptr;
            m_OnError = nullptr;
            m_OnHeartbeat = nullptr;
            m_OnTimeOut = nullptr;
            m_OnWebSocketError = nullptr;
            m_OnProtocolError = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CCustomWebSocketClient::~CCustomWebSocketClient() {
            SwitchConnection(nullptr);
            delete m_pTimer;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CCustomWebSocketClient::Connected() const {
            if (Assigned(m_pConnection)) {
                return m_pConnection->Connected();
            }
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::AddToConnection(CWebSocketClientConnection *AConnection) {
            if (Assigned(AConnection)) {
                AConnection->Object(this);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DeleteFromConnection(CWebSocketClientConnection *AConnection) {
            if (Assigned(AConnection)) {
                AConnection->Object(nullptr);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SwitchConnection(CWebSocketClientConnection *AConnection) {
            if (m_pConnection != AConnection) {
                if (Assigned(AConnection)) {
                    if (Assigned(m_pConnection)) {
                        m_pConnection->Disconnect();
                        DeleteFromConnection(m_pConnection);
                    }
                    AddToConnection(AConnection);
                }
                m_pConnection = AConnection;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SetUpdateConnected(bool Value) {
            if (Value != m_UpdateConnected) {
                m_UpdateConnected = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::IncErrorCount() {
            if (m_ErrorCount == UINT32_MAX)
                m_ErrorCount = 0;
            m_ErrorCount++;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SetTimerInterval(int Value) {
            if (m_TimerInterval != Value) {
                m_TimerInterval = Value;
                UpdateTimer();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::UpdateTimer() {
            if (m_pTimer == nullptr) {
                m_pTimer = CEPollTimer::CreateTimer(CLOCK_MONOTONIC, TFD_NONBLOCK);
                m_pTimer->AllocateTimer(m_pEventHandlers, m_TimerInterval, m_TimerInterval);
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                m_pTimer->OnTimer([this](auto && AHandler) { DoTimer(AHandler); });
#else
                m_pTimer->OnTimer(std::bind(&CCustomWebSocketClient::DoTimer, this, _1));
#endif
            } else {
                m_pTimer->SetTimer(m_TimerInterval, m_TimerInterval);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::Heartbeat(CDateTime Now) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoTimer(CPollEventHandler *AHandler) {
            uint64_t exp;

            auto pTimer = dynamic_cast<CEPollTimer *> (AHandler->Binding());
            pTimer->Read(&exp, sizeof(uint64_t));

            Heartbeat(AHandler->TimeStamp());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoDebugWait(CObject *Sender) {
            auto pConnection = dynamic_cast<CWebSocketClientConnection *> (Sender);
            if (Assigned(pConnection))
                DebugRequest(pConnection->Request());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoDebugRequest(CObject *Sender) {
            auto pConnection = dynamic_cast<CWebSocketClientConnection *> (Sender);
            if (Assigned(pConnection)) {
                if (pConnection->Protocol() == pHTTP) {
                    DebugRequest(pConnection->Request());
                } else {
                    WSDebug(pConnection->WSRequest());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoDebugReply(CObject *Sender) {
            auto pConnection = dynamic_cast<CWebSocketClientConnection *> (Sender);
            if (Assigned(pConnection)) {
                if (pConnection->Protocol() == pHTTP) {
                    DebugReply(pConnection->Reply());
                } else {
                    WSDebug(pConnection->WSReply());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoPing(CObject *Sender) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoPong(CObject *Sender) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoConnectStart(CIOHandlerSocket *AIOHandler, CPollEventHandler *AHandler) {
            auto pConnection = new CWebSocketClientConnection(this);
            pConnection->IOHandler(AIOHandler);
            pConnection->AutoFree(false);
            AHandler->Binding(pConnection);
            SwitchConnection(pConnection);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoConnect(CPollEventHandler *AHandler) {
            auto pConnection = dynamic_cast<CWebSocketClientConnection *> (AHandler->Binding());

            if (pConnection == nullptr) {
                AHandler->Stop();
                return;
            }

            try {
                auto pIOHandler = (CIOHandlerSocket *) pConnection->IOHandler();

                if (pIOHandler->Binding()->CheckConnection()) {
                    ClearErrorCount();
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                    pConnection->OnDisconnected([this](auto &&Sender) { DoDisconnected(Sender); });
                    pConnection->OnWaitRequest([this](auto &&Sender) { DoDebugWait(Sender); });
                    pConnection->OnRequest([this](auto &&Sender) { DoDebugRequest(Sender); });
                    pConnection->OnReply([this](auto &&Sender) { DoDebugReply(Sender); });
                    pConnection->OnPing([this](auto &&Sender) { DoPing(Sender); });
                    pConnection->OnPong([this](auto &&Sender) { DoPong(Sender); });
#else
                    pConnection->OnDisconnected(std::bind(&CCustomWebSocketClient::DoDisconnected, this, _1));
                    pConnection->OnWaitRequest(std::bind(&CCustomWebSocketClient::DoDebugWait, this, _1));
                    pConnection->OnRequest(std::bind(&CCustomWebSocketClient::DoDebugRequest, this, _1));
                    pConnection->OnReply(std::bind(&CCustomWebSocketClient::DoDebugReply, this, _1));
                    pConnection->OnPing(std::bind(&CCustomWebSocketClient::DoPing, this, _1));
                    pConnection->OnPong(std::bind(&CCustomWebSocketClient::DoPong, this, _1));
#endif
                    AHandler->Start(etIO);

                    pConnection->URI() = m_URI;
                    pConnection->Session() = m_Session;

                    DoConnected(pConnection);
                    Handshake(pConnection);
                }
            } catch (Delphi::Exception::Exception &E) {
                IncErrorCount();
                AHandler->Stop();
                SwitchConnection(nullptr);
                throw ESocketError(E.ErrorCode(), "Connection failed ");
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoWebSocket(CHTTPClientConnection *AConnection) {

            const auto &caWSRequest = AConnection->WSRequest();
            auto &WSReply = AConnection->WSReply();

            WSReply.Clear();

            CWSMessage jmRequest;
            CWSMessage jmResponse;

            const CString csRequest(caWSRequest.Payload());

            CWSProtocol::Request(csRequest, jmRequest);

            try {
                if (jmRequest.MessageTypeId == WSProtocol::mtCall) {
                    int i;
                    CWebSocketClientActionHandler *pHandler;
                    for (i = 0; i < m_Actions.Count(); ++i) {
                        pHandler = (CWebSocketClientActionHandler *) m_Actions.Objects(i);
                        if (pHandler->Allow()) {
                            const auto &action = m_Actions.Strings(i);
                            if (action == jmRequest.Action) {
                                CWSProtocol::PrepareResponse(jmRequest, jmResponse);
                                DoMessage(jmRequest);
                                pHandler->Handler(this, jmRequest, jmResponse);
                                break;
                            }
                        }
                    }

                    if (i == m_Actions.Count()) {
                        SendNotSupported(jmRequest.UniqueId, CString().Format("Action %s not supported.", jmRequest.Action.c_str()));
                    }
                } else {
                    auto pHandler = m_Messages.FindMessageById(jmRequest.UniqueId);
                    if (Assigned(pHandler)) {
                        jmRequest.Action = pHandler->Message().Action;
                        DoMessage(jmRequest);
                        pHandler->Handler(AConnection);
                        delete pHandler;
                    }
                }
            } catch (std::exception &e) {
                Log()->Error(APP_LOG_ERR, 0, e.what());
                SendInternalError(jmRequest.UniqueId, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoHTTP(CHTTPClientConnection *AConnection) {
            const auto &caReply = AConnection->Reply();

            if (caReply.Status == CHTTPReply::switching_protocols) {
#ifdef _DEBUG
                WSDebugConnection(AConnection);
#endif
                AConnection->SwitchingProtocols(pWebSocket);
            } else {
                DoWebSocketError(AConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CCustomWebSocketClient::DoExecute(CTCPConnection *AConnection) {
            auto pConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
            if (pConnection->Protocol() == pWebSocket) {
                DoWebSocket(pConnection);
            } else {
                DoHTTP(pConnection);
            }
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::Initialize() {
            SetTimerInterval(1000);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SetURI(const CLocation &URI) {
            m_URI = URI;
            m_Host = URI.hostname;
            m_Port = URI.port;
#ifdef WITH_SSL
            m_UsedSSL = m_Port == HTTP_SSL_PORT;
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::Handshake(CWebSocketClientConnection *AConnection) {
            auto &Request = AConnection->Request();

            Request.AddHeader("Sec-WebSocket-Version", "13");
            Request.AddHeader("Sec-WebSocket-Key", base64_encode(m_Key));
            Request.AddHeader("Upgrade", "websocket");

            CHTTPRequest::Prepare(Request, _T("GET"), m_URI.href().c_str(), nullptr, "Upgrade");

            AConnection->SendRequest(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::Ping() {
            if (Assigned(m_pConnection)) {
                if (m_pConnection->Connected()) {
                    m_pConnection->SendWebSocketPing(true);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SendMessage(const CWSMessage &Message) {
            CString sResponse;
            DoMessage(Message);
            CWSProtocol::Response(Message, sResponse);
            chASSERT(m_pConnection);
            if (m_pConnection != nullptr && m_pConnection->Connected()) {
                m_pConnection->WSReply().SetPayload(sResponse, (uint32_t) MsEpoch());
                m_pConnection->SendWebSocket(true);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CWSMessage CCustomWebSocketClient::RequestToMessage(CWebSocketConnection *AWSConnection) {
            const auto &caWSRequest = AWSConnection->WSRequest();
            CWSMessage Message;
            const CString Payload(caWSRequest.Payload());
            CWSProtocol::Request(Payload, Message);
            AWSConnection->ConnectionStatus(csRequestOk);
            return Message;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SendMessage(const CWSMessage &Message, COnMessageHandlerEvent &&Handler) {
            m_Messages.Add(m_pConnection, Message, static_cast<COnMessageHandlerEvent &&> (Handler), (uint32_t) MsEpoch());
            DoMessage(Message);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SendNotSupported(const CString &UniqueId, const CString &ErrorDescription, const CJSON &Payload) {
            SendMessage(CWSProtocol::CallError(UniqueId, 404, ErrorDescription, Payload));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SendProtocolError(const CString &UniqueId, const CString &ErrorDescription, const CJSON &Payload) {
            SendMessage(CWSProtocol::CallError(UniqueId, 400, ErrorDescription, Payload));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::SendInternalError(const CString &UniqueId, const CString &ErrorDescription, const CJSON &Payload) {
            SendMessage(CWSProtocol::CallError(UniqueId, 500, ErrorDescription, Payload));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoHeartbeat() {
            if (m_OnHeartbeat != nullptr) {
                m_OnHeartbeat(this);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoTimeOut() {
            if (m_OnTimeOut != nullptr) {
                m_OnTimeOut(this);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoMessage(const CWSMessage &Message) {
            if (m_OnMessage != nullptr) {
                m_OnMessage(this, Message);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoError(int Code, const CString &Message) {
            if (m_OnError != nullptr) {
                m_OnError(this, Code, Message);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomWebSocketClient::DoWebSocketError(CTCPConnection *AConnection) {
            if (m_OnWebSocketError != nullptr)
                m_OnWebSocketError(AConnection);
        }

    }
}

}

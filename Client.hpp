/*++

Program name:

 apostol-core

Module Name:

  Client.hpp

Notices:

  WebSocket Client

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_CLIENT_HPP
#define APOSTOL_CLIENT_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Client {

        class CCustomWebSocketClient;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CCustomWebSocketClient *AClient, const CWSMessage &Request, CWSMessage &Response)> COnWebSocketClientActionHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CWebSocketClientActionHandler: CObject {
        private:

            bool m_Allow;

            COnWebSocketClientActionHandlerEvent m_Handler;

        public:

            CWebSocketClientActionHandler(bool Allow, COnWebSocketClientActionHandlerEvent && Handler): CObject(), m_Allow(Allow), m_Handler(Handler) {

            };

            bool Allow() const { return m_Allow; };

            void Handler(CCustomWebSocketClient *AClient, const CWSMessage &Request, CWSMessage &Response) {
                if (m_Allow && m_Handler)
                    m_Handler(AClient, Request, Response);
            }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebSocketMessageHandler ----------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebSocketMessageHandlerManager;
        class CWebSocketMessageHandler;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CWebSocketMessageHandler *Handler, CWebSocketConnection *Connection)> COnMessageHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CWebSocketMessageHandler: public CCollectionItem {
        private:

            CWSMessage m_Message;

            COnMessageHandlerEvent m_Handler;

        public:

            CWebSocketMessageHandler(CWebSocketMessageHandlerManager *AManager, const CWSMessage &Message, COnMessageHandlerEvent && Handler);

            const CWSMessage &Message() const { return m_Message; }

            void Handler(CWebSocketConnection *AConnection);

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebSocketMessageHandlerManager ---------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebSocketMessageHandlerManager: public CCollection {
            typedef CCollection inherited;

        private:

            CWebSocketMessageHandler *Get(int Index) const;
            void Set(int Index, CWebSocketMessageHandler *Value);

        public:

            CWebSocketMessageHandlerManager(): CCollection(this) {

            }

            CWebSocketMessageHandler *Add(CWebSocketConnection *AConnection, const CWSMessage &Message, COnMessageHandlerEvent &&Handler, uint32_t Key = 0);

            CWebSocketMessageHandler *First() { return Get(0); };
            CWebSocketMessageHandler *Last() { return Get(Count() - 1); };

            CWebSocketMessageHandler *FindMessageById(const CString &Value) const;

            CWebSocketMessageHandler *Handlers(int Index) const { return Get(Index); }
            void Handlers(int Index, CWebSocketMessageHandler *Value) { Set(Index, Value); }

            CWebSocketMessageHandler *operator[] (int Index) const override { return Handlers(Index); };

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebSocketClientConnection --------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebSocketClient;
        //--------------------------------------------------------------------------------------------------------------

        class CWebSocketClientConnection: public CHTTPClientConnection {
            typedef CHTTPClientConnection inherited;

        private:

            CLocation m_URI {};

            CString m_Session {};

        public:

            explicit CWebSocketClientConnection(CPollSocketClient *AClient) : CHTTPClientConnection(AClient) {
                TimeOut(INFINITE);
                CloseConnection(false);
            }

            ~CWebSocketClientConnection() override = default;

            CWebSocketClient *WebSocketClient() { return (CWebSocketClient *) CHTTPClientConnection::Client(); };

            CLocation &URI() { return m_URI; };
            const CLocation &URI() const { return m_URI; };

            CString &Session() { return m_Session; };
            const CString &Session() const { return m_Session; };

        }; // CWebSocketClientConnection

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomWebSocketClient ------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CObject *Sender, const CWSMessage &Message)> COnWebSocketClientMessage;
        typedef std::function<void (CObject *Sender, int Code, const CString &Message)> COnWebSocketClientError;
        //--------------------------------------------------------------------------------------------------------------

        class CCustomWebSocketClient: public CHTTPClient, public CGlobalComponent  {
        private:

            CWebSocketClientConnection *m_pConnection;

            CEPollTimer *m_pTimer;

            CLocation m_URI;

            CString m_Key;
            CString m_Session;

            CStringList m_Actions {true};

            uint32_t m_ErrorCount;

            int m_TimerInterval;

            CNotifyEvent m_OnHeartbeat;
            CNotifyEvent m_OnTimeOut;

            COnWebSocketClientMessage m_OnMessage;
            COnWebSocketClientError m_OnError;

            COnSocketConnectionEvent m_OnWebSocketError;
            COnSocketConnectionEvent m_OnProtocolError;

            void Handshake(CWebSocketClientConnection *AConnection);

            void AddToConnection(CWebSocketClientConnection *AConnection);
            void DeleteFromConnection(CWebSocketClientConnection *AConnection);

            void SetUpdateConnected(bool Value);

            void SetTimerInterval(int Value);
            void UpdateTimer();

            void DoDebugWait(CObject *Sender);
            void DoDebugRequest(CObject *Sender);
            void DoDebugReply(CObject *Sender);

            virtual void DoPing(CObject *Sender);
            virtual void DoPong(CObject *Sender);

        protected:

            bool m_UpdateConnected;

            CWebSocketMessageHandlerManager m_Messages;

            void DoWebSocket(CHTTPClientConnection *AConnection);
            void DoHTTP(CHTTPClientConnection *AConnection);

            void DoConnectStart(CIOHandlerSocket *AIOHandler, CPollEventHandler *AHandler) override;
            void DoConnect(CPollEventHandler *AHandler) override;
            bool DoExecute(CTCPConnection *AConnection) override;

            void DoTimer(CPollEventHandler *AHandler);

            virtual void Heartbeat(CDateTime Now);

            void DoHeartbeat();
            void DoTimeOut();

            virtual void DoMessage(const CWSMessage &Message);
            virtual void DoError(int Code, const CString &Message);
            virtual void DoWebSocketError(CTCPConnection *AConnection);

        public:

            CCustomWebSocketClient();

            explicit CCustomWebSocketClient(const CLocation &URI);

            ~CCustomWebSocketClient() override;

            void Initialize() override;

            bool Connected() const;

            void Ping();

            void SwitchConnection(CWebSocketClientConnection *AConnection);

            void SendMessage(const CWSMessage &Message, bool bSendNow = false);
            void SendMessage(const CWSMessage &Message, COnMessageHandlerEvent &&Handler);

            void SendNotSupported(const CString &UniqueId, const CString &ErrorDescription, const CJSON &Payload = {});
            void SendProtocolError(const CString &UniqueId, const CString &ErrorDescription, const CJSON &Payload = {});
            void SendInternalError(const CString &UniqueId, const CString &ErrorDescription, const CJSON &Payload = {});

            void IncErrorCount();
            void ClearErrorCount() { m_ErrorCount = 0; };

            uint32_t ErrorCount() const { return m_ErrorCount; }

            static CWSMessage RequestToMessage(CWebSocketConnection *AWSConnection);

            CWebSocketClientConnection *Connection() { return m_pConnection; };

            CWebSocketMessageHandlerManager &Messages() { return m_Messages; };
            const CWebSocketMessageHandlerManager &Messages() const { return m_Messages; };

            void UpdateConnected(bool Value) { SetUpdateConnected(Value); };
            bool UpdateConnected() const { return m_UpdateConnected; };

            void SetURI(const CLocation &Location);
            const CLocation &URI() const { return m_URI; }

            CString &Key() { return m_Key; }
            const CString &Key() const { return m_Key; }

            CString &Session() { return m_Session; }
            const CString &Session() const { return m_Session; }

            CStringList &Actions() { return m_Actions; }
            const CStringList &Actions() const { return m_Actions; }

            int TimerInterval() const { return m_TimerInterval; }
            void TimerInterval(int Value) { SetTimerInterval(Value); }

            const CNotifyEvent &OnHeartbeat() const { return m_OnHeartbeat; }
            void OnHeartbeat(CNotifyEvent && Value) { m_OnHeartbeat = Value; }

            const CNotifyEvent &OnTimeOut() const { return m_OnTimeOut; }
            void OnTimeOut(CNotifyEvent && Value) { m_OnTimeOut = Value; }

            const COnWebSocketClientMessage &OnMessage() const { return m_OnMessage; }
            void OnMessage(COnWebSocketClientMessage && Value) { m_OnMessage = Value; }

            const COnWebSocketClientError &OnError() const { return m_OnError; }
            void OnError(COnWebSocketClientError && Value) { m_OnError = Value; }

            const COnSocketConnectionEvent &OnProtocolError() { return m_OnProtocolError; }
            void OnProtocolError(COnSocketConnectionEvent && Value) { m_OnProtocolError = Value; }

            const COnSocketConnectionEvent &OnWebSocketError() { return m_OnWebSocketError; }
            void OnWebSocketError(COnSocketConnectionEvent && Value) { m_OnWebSocketError = Value; }

        };

    }
}

using namespace Apostol::Client;
}

#endif //APOSTOL_CLIENT_HPP

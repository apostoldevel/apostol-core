/*++

Library name:

  apostol-core

Module Name:

  Token.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Token.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include "jwt.h"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    CString CToken::CreateToken(const CProvider &Provider, const CString &Application) {
        auto token = jwt::create()
                .set_issuer(Provider.Issuer(Application))
                .set_audience(Provider.ClientId(Application))
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{3600})
                .sign(jwt::algorithm::hs256{std::string(Provider.Secret(Application))});

        return token;
    }
    //------------------------------------------------------------------------------------------------------------------

    CString CToken::CreateGoogleToken(const CProvider &Provider, const CString &Application) {

        const auto &private_key = std::string(Provider.Applications()[Application]["private_key"].AsString());

        const auto &kid = std::string(Provider.Applications()[Application]["private_key_id"].AsString());
        const auto &public_key = std::string(Provider.PublicKey(kid));

        const auto &iss = std::string(Provider.Applications()[Application]["client_email"].AsString());
        const auto &aud = std::string("https://oauth2.googleapis.com/token");
        const auto &scope = std::string("https://www.googleapis.com/auth/firebase.messaging");

        auto token = jwt::create()
                .set_issuer(iss)
                .set_audience(aud)
                .set_payload_claim("scope", jwt::claim(scope))
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{3600})
                .sign(jwt::algorithm::rs256{public_key, private_key});

        return token;
    }
    //------------------------------------------------------------------------------------------------------------------

    void CToken::FetchAccessToken(const CString &URI, const CString &Assertion, COnGetHTTPClientEvent &&OnClient,
                                  COnSocketExecuteEvent &&OnDone, COnSocketExceptionEvent &&OnFail) {

        auto OnRequest = [](CHTTPClient *Sender, CHTTPRequest &Request) {

            const auto &token_uri = Sender->Data()["token_uri"];
            const auto &assertion = Sender->Data()["assertion"];

            Request.Content = _T("grant_type=");
            Request.Content << CHTTPServer::URLEncode("urn:ietf:params:oauth:grant-type:jwt-bearer");

            Request.Content << _T("&assertion=");
            Request.Content << CHTTPServer::URLEncode(assertion);

            CHTTPRequest::Prepare(Request, _T("POST"), token_uri.c_str(), _T("application/x-www-form-urlencoded"));

            DebugRequest(Request);
        };

        auto OnException = [](CTCPConnection *Sender, const Delphi::Exception::Exception &E) {

            const auto pConnection = dynamic_cast<CHTTPClientConnection *> (Sender);

            if (pConnection != nullptr) {
                const auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());
                if (pClient != nullptr) {
                    DebugReply(pConnection->Reply());

                    Log()->Error(APP_LOG_ERR, 0, "[%s:%d] %s", pClient->Host().c_str(), pClient->Port(), E.what());
                }
            }
        };

        const CLocation token_uri(URI);

        const auto pClient = OnClient(token_uri);

        pClient->Data().Values("token_uri", token_uri.pathname);
        pClient->Data().Values("assertion", Assertion);

        pClient->OnRequest(OnRequest);
        pClient->OnExecute(static_cast<COnSocketExecuteEvent &&>(OnDone));
        pClient->OnException(OnFail == nullptr ? OnException : static_cast<COnSocketExceptionEvent &&>(OnFail));

        pClient->AutoFree(true);
        pClient->Active(true);
    }
    //------------------------------------------------------------------------------------------------------------------

    void CToken::ExchangeAccessToken(const CString &URI, const CString &ClientId, const CString &Secret,
            const CString &Token, COnGetHTTPClientEvent &&OnClient, COnSocketExecuteEvent &&OnDone, COnSocketExceptionEvent &&OnFail) {

        auto OnRequest = [](CHTTPClient *Sender, CHTTPRequest &Request) {

            const auto &token_uri = Sender->Data()["token_uri"];
            const auto &client_id = Sender->Data()["client_id"];
            const auto &client_secret = Sender->Data()["client_secret"];
            const auto &subject_token = Sender->Data()["subject_token"];

            Request.Content = _T("client_id=");
            Request.Content << CHTTPServer::URLEncode(client_id);

            Request.Content << _T("&client_secret=");
            Request.Content << CHTTPServer::URLEncode(client_secret);

            Request.Content << _T("&grant_type=");
            Request.Content << CHTTPServer::URLEncode("urn:ietf:params:oauth:grant-type:token-exchange");

            Request.Content << _T("&subject_token=");
            Request.Content << CHTTPServer::URLEncode(subject_token);

            Request.Content << _T("&subject_token_type=");
            Request.Content << CHTTPServer::URLEncode("urn:ietf:params:oauth:token-type:jwt");

            CHTTPRequest::Prepare(Request, _T("POST"), token_uri.c_str(), _T("application/x-www-form-urlencoded"));

            DebugRequest(Request);
        };

        auto OnException = [](CTCPConnection *Sender, const Delphi::Exception::Exception &E) {

            const auto pConnection = dynamic_cast<CHTTPClientConnection *> (Sender);

            if (pConnection != nullptr) {
                const auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());
                if (pClient != nullptr) {
                    DebugReply(pConnection->Reply());

                    Log()->Error(APP_LOG_ERR, 0, "[%s:%d] %s", pClient->Host().c_str(), pClient->Port(), E.what());
                }
            }
        };

        const CLocation token_uri(URI);

        const auto pClient = OnClient(token_uri);

        pClient->Data().Values("token_uri", token_uri.pathname);
        pClient->Data().Values("client_id", ClientId);
        pClient->Data().Values("client_secret", Secret);
        pClient->Data().Values("subject_token", Token);

        pClient->OnRequest(OnRequest);
        pClient->OnExecute(static_cast<COnSocketExecuteEvent &&>(OnDone));
        pClient->OnException(OnFail == nullptr ? OnException : static_cast<COnSocketExceptionEvent &&>(OnFail));

        pClient->AutoFree(true);
        pClient->Active(true);
    }
    //------------------------------------------------------------------------------------------------------------------
}

};
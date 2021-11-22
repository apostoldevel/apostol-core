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

        auto pClient = OnClient(token_uri);

        pClient->Data().Values("token_uri", token_uri.pathname);
        pClient->Data().Values("grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer");
        pClient->Data().Values("assertion", Assertion);

        pClient->OnRequest(OnRequest);
        pClient->OnExecute(static_cast<COnSocketExecuteEvent &&>(OnDone));
        pClient->OnException(OnFail == nullptr ? OnException : static_cast<COnSocketExceptionEvent &&>(OnFail));

        pClient->Active(true);
    }
    //------------------------------------------------------------------------------------------------------------------
}

};
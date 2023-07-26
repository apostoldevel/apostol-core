/*++

Library name:

  apostol-core

Module Name:

  Token.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_TOKEN_HPP
#define APOSTOL_TOKEN_HPP
//----------------------------------------------------------------------------------------------------------------------

#define SYSTEM_PROVIDER_NAME "system"
#define SERVICE_APPLICATION_NAME "service"

#define GOOGLE_PROVIDER_NAME "google"
#define FIREBASE_APPLICATION_NAME "firebase"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    struct CCleanToken {

        const CString Header {};
        const CString Payload {};

        const bool Valid;

        explicit CCleanToken(const CString& Header, const CString& Payload, bool Valid):
                Header(Header), Payload(Payload), Valid(Valid) {

        }

        template<typename T, typename E>
        CString Sign(const T& algorithm, E& ec) const {
            if (!Valid)
                throw CAuthorizationError("Clean Token has not valid.");
            const auto& header = base64urlEncoding(Header);
            const auto& payload = base64urlEncoding(Payload);
            CString token = header + "." + payload;
            return token + "." + base64urlEncoding(algorithm.sign(token, ec));
        }

    };
    //--------------------------------------------------------------------------------------------------------------

    class CToken: public CGlobalComponent {
    public:

        CToken() = default;
        ~CToken() override = default;

        static CString CreateToken(const CProvider& Provider, const CString &Application);
        static CString CreateGoogleToken(const CProvider& Provider, const CString &Application);

        static void FetchAccessToken(const CString &URI, const CString &Assertion, COnGetHTTPClientEvent && OnClient,
            COnSocketExecuteEvent && OnDone, COnSocketExceptionEvent && OnFail = nullptr);

        static void ExchangeAccessToken(const CString &URI, const CString &ClientId, const CString &Secret,
            const CString &Token, COnGetHTTPClientEvent && OnClient, COnSocketExecuteEvent && OnDone, COnSocketExceptionEvent && OnFail = nullptr);

    };

}

};

#endif //APOSTOL_TOKEN_HPP

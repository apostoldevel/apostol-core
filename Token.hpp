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

    class CToken: public CGlobalComponent {
    public:

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

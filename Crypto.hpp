/*++

Library name:

  apostol-core

Crypto Name:

  Crypto.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_CRYPTO_HPP
#define APOSTOL_CRYPTO_HPP

#define APOSTOL_MODULE_UID_LENGTH    42

extern "C++" {

namespace Apostol {

    namespace Crypto {

        CString B2A_HEX(const CString &Data, bool UpperCache = false);
        CString HMAC_SHA256(const CString &Key, const CString &Data);
        //--------------------------------------------------------------------------------------------------------------

        CString SHA1(const CString &Data, bool bHex = false);
        CString SHA224(const CString &Data, bool bHex = false);
        CString SHA256(const CString &Data, bool bHex = false);
        CString SHA384(const CString &Data, bool bHex = false);
        CString SHA512(const CString &Data, bool bHex = false);
        //--------------------------------------------------------------------------------------------------------------

        CString GetUID(unsigned int len);
        CString ApostolUID();

    }
}

using namespace Apostol::Crypto;
}
//----------------------------------------------------------------------------------------------------------------------

#endif //APOSTOL_CRYPTO_HPP

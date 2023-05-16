/*++

Library name:

  apostol-core

Module Name:

  Crypto.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Crypto.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include <random>
//----------------------------------------------------------------------------------------------------------------------

#include <openssl/sha.h>
#include <openssl/hmac.h>
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Crypto {

        CString B2A_HEX(const CString& Data, bool UpperCache) {
            const static CString HL = "0123456789abcdef";
            const static CString HU = "0123456789ABCDEF";
            CString Result;
            for (int i = 0; i < Data.Size(); ++i) {
                unsigned char ch = Data[i];
                if (UpperCache) {
                    Result += HU[(ch >> 4) & 0x0F];
                    Result += HU[ch & 0x0F];
                } else {
                    Result += HL[(ch >> 4) & 0x0F];
                    Result += HL[ch & 0x0F];
                }
            }
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString HMAC_SHA256(const CString &Key, const CString &Data) {
            unsigned char* digest;
            digest = HMAC(EVP_sha256(), Key.Data(), (int) Key.Size(), (unsigned char *) Data.Data(), Data.Size(), nullptr, nullptr);
            return B2A_HEX( { (LPCTSTR) digest, SHA256_DIGEST_LENGTH} );
        }
        //--------------------------------------------------------------------------------------------------------------

        CString SHA1(const CString &Data) {
            CString Digest;
            Digest.SetLength(SHA_DIGEST_LENGTH);
            ::SHA1((unsigned char *) Data.Data(), Data.Size(), (unsigned char *) Digest.Data());
            return B2A_HEX(Digest);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString SHA224(const CString &Data) {
            CString Digest;
            Digest.SetLength(SHA224_DIGEST_LENGTH);
            ::SHA224((unsigned char *) Data.Data(), Data.Size(), (unsigned char *) Digest.Data());
            return B2A_HEX(Digest);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString SHA256(const CString &Data) {
            CString Digest;
            Digest.SetLength(SHA256_DIGEST_LENGTH);
            ::SHA256((unsigned char *) Data.Data(), Data.Size(), (unsigned char *) Digest.Data());
            return B2A_HEX(Digest);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString SHA384(const CString &Data) {
            CString Digest;
            Digest.SetLength(SHA384_DIGEST_LENGTH);
            ::SHA224((unsigned char *) Data.Data(), Data.Size(), (unsigned char *) Digest.Data());
            return B2A_HEX(Digest);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString SHA512(const CString &Data) {
            CString Digest;
            Digest.SetLength(SHA512_DIGEST_LENGTH);
            ::SHA512((unsigned char *) Data.Data(), Data.Size(), (unsigned char *) Digest.Data());
            return B2A_HEX(Digest);
        }
        //--------------------------------------------------------------------------------------------------------------

        unsigned char random_char() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            return static_cast<unsigned char>(dis(gen));
        }
        //--------------------------------------------------------------------------------------------------------------

        CString GetUID(unsigned int len) {
            CString S;

            S.SetLength(len + 1);

            for (unsigned int i = 0; i < len / 2; i++) {
                unsigned char rc = random_char();
                ByteToHexStr(S.Data() + i * 2 * sizeof(unsigned char), S.Size(), &rc, 1);
            }

            return S;
        }
        //--------------------------------------------------------------------------------------------------------------

//      A0000-P0000-O0000-S0000-T0000-O0000-L00000
//      012345678901234567890123456789012345678901
//      0         1         2         3         4
        CString ApostolUID() {

            CString S(GetUID(APOSTOL_MODULE_UID_LENGTH));

            S[ 0] = 'A';
            S[ 5] = '-';
            S[ 6] = 'P';
            S[11] = '-';
            S[12] = 'O';
            S[17] = '-';
            S[18] = 'S';
            S[23] = '-';
            S[24] = 'T';
            S[29] = '-';
            S[30] = 'O';
            S[35] = '-';
            S[36] = 'L';

            return S;
        }
    }
}
}

/*++

Library name:

  apostol-core

Module Name:

  Module.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Module.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include <sstream>
#include <random>
//----------------------------------------------------------------------------------------------------------------------

#include <openssl/sha.h>
#include <openssl/hmac.h>
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Module {

        CString b2a_hex(const unsigned char *byte_arr, int size) {
            const static CString HexCodes = "0123456789abcdef";
            CString hexString;
            for ( int i = 0; i < size; ++i ) {
                unsigned char BinValue = byte_arr[i];
                hexString += HexCodes[(BinValue >> 4) & 0x0F];
                hexString += HexCodes[BinValue & 0x0F];
            }
            return hexString;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString hmac_sha256(const CString &key, const CString &data) {
            unsigned char* digest;
            digest = HMAC(EVP_sha256(), key.data(), key.length(), (unsigned char *) data.data(), data.length(), nullptr, nullptr);
            return b2a_hex( digest, SHA256_DIGEST_LENGTH );
        }
        //--------------------------------------------------------------------------------------------------------------

        CString SHA1(const CString &data) {
            CString digest;
            digest.SetLength(SHA_DIGEST_LENGTH);
            ::SHA1((unsigned char *) data.data(), data.length(), (unsigned char *) digest.Data());
            return digest;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString LongToString(unsigned long Value) {
            TCHAR szString[_INT_T_LEN + 1] = {0};
            sprintf(szString, "%lu", Value);
            return CString(szString);
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCTSTR StrWebTime(time_t Time, LPTSTR lpszBuffer, size_t Size) {
            struct tm *gmt;

            gmt = gmtime(&Time);

            if ((gmt != nullptr) && (strftime(lpszBuffer, Size, "%a, %d %b %Y %T %Z", gmt) != 0)) {
                return lpszBuffer;
            }

            return nullptr;
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

        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApostolModule::CApostolModule(CModuleProcess *AProcess, const CString& ModuleName, const CString& SectionName):
            CCollectionItem(AProcess), CGlobalComponent(), m_pModuleProcess(AProcess), m_ModuleName(ModuleName),
            m_SectionName(SectionName) {

            m_ModuleStatus = msUnknown;

            m_pMethods = CStringList::Create(true);

            m_Headers.Add("Content-Type");
            m_Headers.Add("X-Requested-With");
        }
        //--------------------------------------------------------------------------------------------------------------

        CApostolModule::~CApostolModule() {
            delete m_pMethods;
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPServer &CApostolModule::Server() {
            return m_pModuleProcess->Server();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CHTTPServer &CApostolModule::Server() const {
            return m_pModuleProcess->Server();
        }
#ifdef WITH_POSTGRESQL
        CPQClient &CApostolModule::PQClient() {
            return m_pModuleProcess->GetPQClient();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CPQClient &CApostolModule::PQClient() const {
            return m_pModuleProcess->GetPQClient();
        }
#endif
        const CString &CApostolModule::GetAllowedMethods() const {
            if (m_AllowedMethods.IsEmpty()) {
                if (m_pMethods->Count() > 0) {
                    CMethodHandler *pHandler;
                    for (int i = 0; i < m_pMethods->Count(); ++i) {
                        pHandler = (CMethodHandler *) m_pMethods->Objects(i);
                        if (pHandler->Allow()) {
                            if (m_AllowedMethods.IsEmpty())
                                m_AllowedMethods = m_pMethods->Strings(i);
                            else
                                m_AllowedMethods += _T(", ") + m_pMethods->Strings(i);
                        }
                    }
                }
            }

            return m_AllowedMethods;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CApostolModule::GetAllowedHeaders() const {
            if (m_AllowedHeaders.IsEmpty()) {
                for (int i = 0; i < m_Headers.Count(); ++i) {
                    if (m_AllowedHeaders.IsEmpty())
                        m_AllowedHeaders = m_Headers.Strings(i);
                    else
                        m_AllowedHeaders += _T(", ") + m_Headers.Strings(i);
                }
            }

            return m_AllowedHeaders;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::MethodNotAllowed(CHTTPServerConnection *AConnection) {
            auto pReply = AConnection->Reply();

            CHTTPReply::GetStockReply(pReply, CHTTPReply::not_allowed);

            if (!AllowedMethods().IsEmpty())
                pReply->AddHeader(_T("Allow"), AllowedMethods());

            AConnection->SendReply();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::InitSites(const CSites &Sites) {

            auto InitConfig = [](const CJSON &Config, CStringList &Data) {
                Data.AddPair("root", Config["root"].AsString());
                Data.AddPair("listen", Config["listen"].AsString());

                const auto& caIdentifier = Config["oauth2"]["identifier"].AsString();
                Data.AddPair("oauth2.identifier", caIdentifier.IsEmpty() ? "/oauth/identifier" : caIdentifier);

                const auto& caSecret = Config["oauth2"]["secret"].AsString();
                Data.AddPair("oauth2.secret", caSecret.IsEmpty() ? "/oauth/secret" : caSecret);

                const auto& caCallback = Config["oauth2"]["callback"].AsString();
                Data.AddPair("oauth2.callback", caCallback.IsEmpty() ? "/oauth/callback" : caCallback);

                const auto& caError = Config["oauth2"]["error"].AsString();
                Data.AddPair("oauth2.error", caError.IsEmpty() ? "/oauth/error" : caError);

                const auto& caDebug = Config["oauth2"]["debug"].AsString();
                Data.AddPair("oauth2.debug", caDebug.IsEmpty() ? "/oauth/debug" : caDebug);
            };

            int Index;
            for (int i = 0; i < Sites.Count(); ++i) {
                const auto& Config = Sites[i].Value();
                const auto& Root = Config["root"].AsString();
                if (!Root.IsEmpty()) {
                    const auto& Hosts = Config["hosts"];
                    if (!Hosts.IsNull()) {
                        for (int l = 0; l < Hosts.Count(); ++l) {
                            Index = m_Sites.AddPair(Hosts[l].AsString(), Root);
                            InitConfig(Config, m_Sites[Index].Data());
                        }
                    } else {
                        Index = m_Sites.AddPair(Sites[i].Name(), Root);
                        InitConfig(Config, m_Sites[Index].Data());
                    }
                }
            }

            m_Sites.AddPair("*", Config()->DocRoot());
            InitConfig(Sites.Default().Value(), m_Sites.Last().Data());
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CApostolModule::GetRoot(const CString &Host) const {
            auto Index = m_Sites.IndexOfName(Host);
            if (Index == -1)
                return m_Sites["*"];
            return m_Sites[Index].Value();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CStringList &CApostolModule::GetSiteConfig(const CString &Host) const {
            auto Index = m_Sites.IndexOfName(Host);
            if (Index == -1)
                return m_Sites.Pairs("*").Data();
            return m_Sites[Index].Data();
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetHostName() {
            CString sResult;
            sResult.SetLength(NI_MAXHOST);
            if (GStack->GetHostName(sResult.Data(), sResult.Size())) {
                sResult.Truncate();
            }
            return sResult;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetIPByHostName(const CString &HostName) {
            CString sResult;
            if (!HostName.IsEmpty()) {
                sResult.SetLength(16);
                GStack->GetIPByName(HostName.c_str(), sResult.Data(), sResult.Size());
            }
            return sResult;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetUserAgent(CHTTPServerConnection *AConnection) {
            auto pServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto pRequest = AConnection->Request();
            const auto& agent = pRequest->Headers[_T("User-Agent")];
            return agent.IsEmpty() ? pServer->ServerName() : agent;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetOrigin(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            const auto& caOrigin = pRequest->Headers[_T("Origin")];
            return caOrigin.IsEmpty() ? pRequest->Location.Origin() : caOrigin;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetProtocol(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            const auto& caProtocol = pRequest->Headers[_T("X-Forwarded-Proto")];
            return caProtocol.IsEmpty() ? pRequest->Location.protocol : caProtocol;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetRealIP(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            CString sHost;

            const auto& caRealIP = pRequest->Headers[_T("X-Real-IP")];
            if (!caRealIP.IsEmpty()) {
                sHost = caRealIP;
            } else {
                auto pSocket = AConnection->Socket();
                if (pSocket != nullptr) {
                    auto pHandle = pSocket->Binding();
                    if (pHandle != nullptr) {
                        sHost = pHandle->PeerIP();
                    }
                }
            }

            return sHost;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetHost(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            const CString sHost(pRequest->Headers[_T("Host")]);
            return sHost.IsEmpty() ? "localhost" : sHost;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ExceptionToJson(int ErrorCode, const std::exception &e, CString& Json) {
            Json.Clear();
            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(e.what()).c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ContentToJson(CHTTPRequest *ARequest, CJSON &Json) {

            const auto& caContentType = ARequest->Headers[_T("Content-Type")].Lower();

            if (caContentType.Find("application/x-www-form-urlencoded") != CString::npos) {

                const CStringList &formData = ARequest->FormData;

                auto& jsonObject = Json.Object();
                for (int i = 0; i < formData.Count(); ++i) {
                    jsonObject.AddPair(formData.Names(i), formData.ValueFromIndex(i));
                }

            } else if (caContentType.Find("multipart/form-data") != CString::npos) {

                CFormData formData;
                CHTTPRequestParser::ParseFormData(ARequest, formData);

                auto& jsonObject = Json.Object();
                for (int i = 0; i < formData.Count(); ++i) {
                    jsonObject.AddPair(formData[i].Name, formData[i].Data);
                }

            } else if (caContentType.Find("application/json") != CString::npos) {

                Json << ARequest->Content;

            } else {

                auto& jsonObject = Json.Object();
                for (int i = 0; i < ARequest->Params.Count(); ++i) {
                    jsonObject.AddPair(ARequest->Params.Names(i), CHTTPServer::URLDecode(ARequest->Params.ValueFromIndex(i)));
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ListToJson(const CStringList &List, CString &Json, bool DataArray, const CString &ObjectName) {

            const auto bResultObject = !ObjectName.IsEmpty();

            DataArray = bResultObject || DataArray || List.Count() > 1;

            const auto emptyData = DataArray ? _T("[]") : _T("{}");

            if (List.Count() == 0) {
                Json = bResultObject ? CString().Format("{\"%s\": %s}", ObjectName.c_str(), emptyData) : emptyData;
                return;
            }

            if (bResultObject)
                Json.Format("{\"%s\": ", ObjectName.c_str());

            if (DataArray)
                Json += _T("[");

            for (int i = 0; i < List.Count(); ++i) {
                const auto& Line = List[i];
                if (!Line.IsEmpty()) {
                    if (i > 0)
                        Json += _T(",");
                    Json += Line;
                }
            }

            if (DataArray)
                Json += _T("]");

            if (bResultObject)
                Json += _T("}");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ReplyError(CHTTPServerConnection *AConnection, CHTTPReply::CStatusType ErrorCode, const CString &Message) {
            auto pReply = AConnection->Reply();

            pReply->ContentType = CHTTPReply::json;

            if (ErrorCode == CHTTPReply::unauthorized) {
                CHTTPReply::AddUnauthorized(pReply, AConnection->Data()["Authorization"] != "Basic", "invalid_client", Message.c_str());
            }

            pReply->Content.Clear();
            pReply->Content.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(Message).c_str());

            AConnection->SendReply(ErrorCode, nullptr, true);

            Log()->Error(ErrorCode == CHTTPReply::internal_server_error ? APP_LOG_EMERG : APP_LOG_ERR, 0, _T("ReplyError: %s"), Message.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::Redirect(CHTTPServerConnection *AConnection, const CString& Location, bool SendNow) {
            auto pReply = AConnection->Reply();

            pReply->Content.Clear();
            pReply->AddHeader(_T("Location"), Location);

            AConnection->Data().Values("redirect", CString());
            AConnection->Data().Values("redirect_error", CString());

            AConnection->SendStockReply(CHTTPReply::moved_temporarily, SendNow);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::TryFiles(const CString &Root, const CStringList &uris, const CString &Location) {
            for (int i = 0; i < uris.Count(); i++) {
                const auto& uri = Root + uris[i];
                if (FileExists(uri.c_str())) {
                    return uri;
                }
            }
            return Root + Location;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::SendResource(CHTTPServerConnection *AConnection, const CString &Path,
                LPCTSTR AContentType, bool SendNow, const CStringList& TryFiles) const {

            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            CString sRoot(GetRoot(pRequest->Location.Host()));

            if (!path_separator(sRoot.front())) {
                sRoot = Config()->Prefix() + sRoot;
            }

            CString sResource(sRoot);

            sResource += Path;

            if (!path_separator(sResource.back()) && DirectoryExists(sResource.c_str())) {
                sResource += '/';
            }

            if (path_separator(sResource.back())) {
                sResource += APOSTOL_INDEX_FILE;
            }

            if (TryFiles.Count() != 0 && !FileExists(sResource.c_str())) {
                sResource = CApostolModule::TryFiles(sRoot, TryFiles, Path);
            }

            if (!FileExists(sResource.c_str())) {
                AConnection->SendStockReply(CHTTPReply::not_found, SendNow);
                return;
            }

            CString sFileExt;
            TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

            sFileExt = ExtractFileExt(szBuffer, sResource.c_str());

            if (AContentType == nullptr) {
                AContentType = Mapping::ExtToType(sFileExt.c_str());
            }

            auto sModified = StrWebTime(FileAge(sResource.c_str()), szBuffer, sizeof(szBuffer));
            if (sModified != nullptr) {
                pReply->AddHeader(_T("Last-Modified"), sModified);
            }

            pReply->Content.LoadFromFile(sResource.c_str());
            AConnection->SendReply(CHTTPReply::ok, AContentType, SendNow);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoHead(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            CString sPath(pRequest->Location.pathname);

            // Request sPath must be absolute and not contain "..".
            if (sPath.empty() || sPath.front() != '/' || sPath.find("..") != CString::npos) {
                AConnection->SendStockReply(CHTTPReply::bad_request);
                return;
            }

            // If path ends in slash.
            if (sPath.back() == '/') {
                sPath += "index.html";
            }

            CString sResource(GetRoot(pRequest->Location.Host()));
            sResource += sPath;

            if (!FileExists(sResource.c_str())) {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

            auto contentType = Mapping::ExtToType(ExtractFileExt(szBuffer, sResource.c_str()));
            if (contentType != nullptr) {
                pReply->AddHeader(_T("Content-Type"), contentType);
            }

            auto fileSize = FileSize(sResource.c_str());
            pReply->AddHeader(_T("Content-Length"), IntToStr((int) fileSize, szBuffer, sizeof(szBuffer)));

            auto LModified = StrWebTime(FileAge(sResource.c_str()), szBuffer, sizeof(szBuffer));
            if (LModified != nullptr)
                pReply->AddHeader(_T("Last-Modified"), LModified);

            AConnection->SendReply(CHTTPReply::no_content);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoOptions(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            CHTTPReply::GetStockReply(pReply, CHTTPReply::no_content);

            if (!AllowedMethods().IsEmpty())
                pReply->AddHeader(_T("Allow"), AllowedMethods());

            AConnection->SendReply();
#ifdef _DEBUG
            if (pRequest->URI == _T("/reload"))
                GApplication->SignalProcess()->SignalReload();

            if (pRequest->URI == _T("/quit"))
                GApplication->SignalProcess()->SignalQuit();
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoGet(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();

            CString sPath(pRequest->Location.pathname);

            // Request sPath must be absolute and not contain "..".
            if (sPath.empty() || sPath.front() != '/' || sPath.find(_T("..")) != CString::npos) {
                AConnection->SendStockReply(CHTTPReply::bad_request);
                return;
            }

            SendResource(AConnection, sPath);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::CORS(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            const auto& caRequestHeaders = pRequest->Headers;
            auto& aReplyHeaders = pReply->Headers;

            const auto& caOrigin = caRequestHeaders[_T("origin")];
            if (!caOrigin.IsEmpty()) {
                aReplyHeaders.AddPair(_T("Access-Control-Allow-Origin"), caOrigin);
                aReplyHeaders.AddPair(_T("Access-Control-Allow-Methods"), AllowedMethods());
                aReplyHeaders.AddPair(_T("Access-Control-Allow-Headers"), AllowedHeaders());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPClient *CApostolModule::GetClient(const CString &Host, uint16_t Port) {
            return m_pModuleProcess->GetClient(Host, Port);
        }
        //--------------------------------------------------------------------------------------------------------------
#ifdef WITH_POSTGRESQL
        void CApostolModule::PQResultToList(CPQResult *Result, CStringList &List) {
            for (int row = 0; row < Result->nTuples(); ++row) {
                List.Add(Result->GetValue(row, 0));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::PQResultToJson(CPQResult *Result, CString &Json, const CString &Format, const CString &ObjectName) {

            const auto bResultObject = !ObjectName.IsEmpty();
            const auto bDataArray = bResultObject || Format == "array" || Result->nTuples() > 1;
            const auto emptyData = bDataArray ? _T("[]") : _T("{}");

            if (Result->nTuples() == 0) {
                if (Format == "null") {
                    Json = Format;
                } else {
                    Json = bResultObject ? CString().Format("{\"%s\": %s}", ObjectName.c_str(), emptyData) : emptyData;
                }

                return;
            }

            if (bResultObject)
                Json.Format("{\"%s\": ", ObjectName.c_str());

            if (bDataArray)
                Json += _T("[");

            for (int row = 0; row < Result->nTuples(); ++row) {
                if (!Result->GetIsNull(row, 0)) {
                    if (row > 0)
                        Json += _T(",");
                    Json.Append(Result->GetValue(row, 0), Result->GetLength(row, 0));
                } else {
                    Json += bDataArray ? _T("null") : _T("{}");
                }
            }

            if (bDataArray)
                Json += _T("]");

            if (bResultObject)
                Json += _T("}");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::EnumQuery(CPQResult *APQResult, CPQueryResult& AResult) {
            CStringList cFields;

            for (int i = 0; i < APQResult->nFields(); ++i) {
                cFields.Add(APQResult->fName(i));
            }

            for (int row = 0; row < APQResult->nTuples(); ++row) {
                AResult.Add(CStringPairs());
                for (int col = 0; col < APQResult->nFields(); ++col) {
                    if (APQResult->GetIsNull(row, col)) {
                        AResult.Last().AddPair(cFields[col].c_str(), _T(""));
                    } else {
                        if (APQResult->fFormat(col) == 0) {
                            AResult.Last().AddPair(cFields[col].c_str(), APQResult->GetValue(row, col));
                        } else {
                            AResult.Last().AddPair(cFields[col].c_str(), _T("<binary>"));
                        }
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::QueryToResults(CPQPollQuery *APollQuery, CPQueryResults& AResults) {
            CPQResult *pResult = nullptr;
            int Index;
            for (int i = 0; i < APollQuery->ResultCount(); ++i) {
                pResult = APollQuery->Results(i);
                if (pResult->ExecStatus() == PGRES_TUPLES_OK || pResult->ExecStatus() == PGRES_SINGLE_TUPLE) {
                    Index = AResults.Add(CPQueryResult());
                    EnumQuery(pResult, AResults[Index]);
                } else {
                    throw Delphi::Exception::EDBError(pResult->GetErrorMessage());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CApostolModule::GetQuery(CPollConnection *AConnection) {
            CPQPollQuery *pQuery = m_pModuleProcess->GetQuery(AConnection);

            if (Assigned(pQuery)) {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                pQuery->OnPollExecuted([this](auto && APollQuery) { DoPostgresQueryExecuted(APollQuery); });
                pQuery->OnException([this](auto && APollQuery, auto && AException) { DoPostgresQueryException(APollQuery, AException); });
#else
                pQuery->OnPollExecuted(std::bind(&CApostolModule::DoPostgresQueryExecuted, this, _1));
                pQuery->OnException(std::bind(&CApostolModule::DoPostgresQueryException, this, _1, _2));
#endif
            }

            return pQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CApostolModule::ExecSQL(const CStringList &SQL, CPollConnection *AConnection,
                COnPQPollQueryExecutedEvent &&OnExecuted, COnPQPollQueryExceptionEvent &&OnException) {

            auto pQuery = GetQuery(AConnection);

            if (pQuery == nullptr)
                throw Delphi::Exception::Exception(_T("ExecSQL: Get SQL query failed."));

            if (OnExecuted != nullptr)
                pQuery->OnPollExecuted(static_cast<COnPQPollQueryExecutedEvent &&>(OnExecuted));

            if (OnException != nullptr)
                pQuery->OnException(static_cast<COnPQPollQueryExceptionEvent &&>(OnException));

            pQuery->SQL() = SQL;

            if (pQuery->Start() == POLL_QUERY_START_FAIL) {
                delete pQuery;
                throw Delphi::Exception::Exception(_T("ExecSQL: Start SQL query failed."));
            }

            return pQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresNotify(CPQConnection *AConnection, PGnotify *ANotify) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());

            auto pReply = pConnection->Reply();
            auto pResult = APollQuery->Results(0);

            CHTTPReply::CStatusType status = CHTTPReply::internal_server_error;

            try {
                if (pResult->ExecStatus() != PGRES_TUPLES_OK)
                    throw Delphi::Exception::EDBError(pResult->GetErrorMessage());

                status = CHTTPReply::ok;
                Postgres::PQResultToJson(pResult, pReply->Content);
            } catch (Delphi::Exception::Exception &E) {
                ExceptionToJson(status, E, pReply->Content);
                Log()->Error(APP_LOG_ERR, 0, "%s", E.what());
            }

            pConnection->SendReply(status, nullptr, true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());
            auto pReply = pConnection->Reply();

            CHTTPReply::CStatusType status = CHTTPReply::internal_server_error;

            ExceptionToJson(status, E, pReply->Content);
            pConnection->SendStockReply(status, true);

            Log()->Error(APP_LOG_ERR, 0, "%s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
        bool CApostolModule::Execute(CHTTPServerConnection *AConnection) {

            auto pServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            if (!CheckLocation(pRequest->Location))
                return false;

            if (m_Sites.Count() == 0)
                InitSites(pServer->Sites());
#ifdef _DEBUG
            DebugConnection(AConnection);
#endif
            pReply->Clear();
            pReply->ContentType = CHTTPReply::html;

            int i;
            CMethodHandler *pHandler;
            for (i = 0; i < m_pMethods->Count(); ++i) {
                pHandler = (CMethodHandler *) m_pMethods->Objects(i);
                if (pHandler->Allow()) {
                    const CString& Method = m_pMethods->Strings(i);
                    if (Method == pRequest->Method) {
                        CORS(AConnection);
                        pHandler->Handler(AConnection);
                        break;
                    }
                }
            }

            if (i == m_pMethods->Count()) {
                AConnection->SendStockReply(CHTTPReply::not_implemented);
            }

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApostolModule::CheckLocation(const CLocation &Location) {
            return !Location.pathname.IsEmpty();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::Heartbeat(CDateTime Datetime) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args) {
            Log()->Debug(APP_LOG_DEBUG_CORE, AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoException(CTCPConnection *AConnection, const Delphi::Exception::Exception &E) {
            Log()->Error(APP_LOG_ERR, 0, "Exception: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoEventHandlerException(CPollEventHandler *AHandler, const Delphi::Exception::Exception &E) {
            Log()->Error(APP_LOG_ERR, 0, "EventHandlerException: %s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoNoCommandHandler(CSocketEvent *Sender, const CString &Data, CTCPConnection *AConnection) {
            Log()->Error(APP_LOG_ERR, 0, "No command handler: %s", Data.IsEmpty() ? "(null)" : Data.c_str());
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CModuleProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CModuleProcess::CModuleProcess(): CServerProcess(), CModuleManager() {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleProcess::DoInitialization(CApostolModule *AModule) {
            AModule->Initialization(this);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleProcess::DoFinalization(CApostolModule *AModule) {
            AModule->Finalization(this);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleProcess::DoBeforeExecuteModule(CApostolModule *AModule) {
            AModule->BeforeExecute(this);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleProcess::DoAfterExecuteModule(CApostolModule *AModule) {
            AModule->AfterExecute(this);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleProcess::DoTimer(CPollEventHandler *AHandler) {
            uint64_t exp;

            auto pTimer = dynamic_cast<CEPollTimer *> (AHandler->Binding());
            pTimer->Read(&exp, sizeof(uint64_t));

            try {
                HeartbeatModules(AHandler->TimeStamp());
            } catch (Delphi::Exception::Exception &E) {
                DoServerEventHandlerException(AHandler, E);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CModuleProcess::DoExecute(CTCPConnection *AConnection) {
            auto pConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);

            try {
                clock_t start = clock();

                ExecuteModules(pConnection);

                Log()->Debug(APP_LOG_DEBUG_CORE, _T("[Module] Runtime: %.2f ms."), (double) (clock() - start) / (double) CLOCKS_PER_SEC * 1000);
            } catch (Delphi::Exception::Exception &E) {
                DoServerException(pConnection, E);
            }

            return true;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CWorkerProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWorkerProcess::CWorkerProcess() {
            CreateWorkers(this);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CHelperProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CHelperProcess::CHelperProcess() {
            CreateHelpers(this);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CModuleManager --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CModuleManager::Initialization() {
            for (int i = 0; i < ModuleCount(); i++) {
                auto Module = Modules(i);
                if (Module->Enabled())
                    DoInitialization(Module);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleManager::Finalization() {
            for (int i = 0; i < ModuleCount(); i++) {
                auto Module = Modules(i);
                if (Module->Enabled())
                    DoFinalization(Module);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CModuleManager::ModulesNames() {
            if (ModuleCount() == 0)
                return "empty";

            CString Names;
            int Count = 0;
            for (int i = 0; i < ModuleCount(); i++) {
                auto Module = Modules(i);
                if (Module->Enabled()) {
                    if (Count > 0)
                        Names << ", ";
                    Names << "\"" + Modules(i)->ModuleName() + "\"";
                    Count++;
                }
            }

            return Names.IsEmpty() ? "idle" : Names;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleManager::HeartbeatModules(CDateTime Datetime) {
            for (int i = 0; i < ModuleCount(); i++) {
                auto Module = Modules(i);
                if (Module->Enabled())
                    Module->Heartbeat(Datetime);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CModuleManager::ExecuteModule(CHTTPServerConnection *AConnection, CApostolModule *AModule) {
            bool Result = AModule->Enabled();
            if (Result) {
                DoBeforeExecuteModule(AModule);
                try {
                    Result = AModule->Execute(AConnection);
                } catch (...) {
                    AConnection->SendStockReply(CHTTPReply::internal_server_error);
                }
                DoAfterExecuteModule(AModule);
            }
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleManager::ExecuteModules(CTCPConnection *AConnection) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);
            if (pConnection == nullptr)
                return;

            int Index = 0;
            while (Index < ModuleCount() && !ExecuteModule(pConnection, Modules(Index))) {
                Index++;
            }

            if (Index == ModuleCount()) {
                pConnection->SendStockReply(CHTTPReply::forbidden);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}

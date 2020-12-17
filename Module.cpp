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
            CString S(len, ' ');

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
        CPQServer &CApostolModule::PQServer() {
            return m_pModuleProcess->PQServer();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CPQServer &CApostolModule::PQServer() const {
            return m_pModuleProcess->PQServer();
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

                const auto& Identifier = Config["oauth2"]["identifier"].AsString();
                Data.AddPair("oauth2.identifier", Identifier.IsEmpty() ? "/oauth/identifier" : Identifier);

                const auto& Callback = Config["oauth2"]["callback"].AsString();
                Data.AddPair("oauth2.callback", Callback.IsEmpty() ? "/oauth/callback" : Callback);

                const auto& Error = Config["oauth2"]["error"].AsString();
                Data.AddPair("oauth2.error", Error.IsEmpty() ? "/oauth/error" : Error);
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
            const auto& LAgent = pRequest->Headers[_T("User-Agent")];
            return LAgent.IsEmpty() ? pServer->ServerName() : LAgent;
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

        CString CApostolModule::GetHost(CHTTPServerConnection *AConnection) {
            auto pRequest = AConnection->Request();
            CString sHost;

            const auto& caRealIP = pRequest->Headers[_T("X-Real-IP")];
            if (!caRealIP.IsEmpty()) {
                sHost = caRealIP;
            } else {
                auto pBinding = AConnection->Socket()->Binding();
                if (pBinding != nullptr) {
                    sHost = pBinding->PeerIP();
                }
            }

            return sHost;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ExceptionToJson(int ErrorCode, const Delphi::Exception::Exception &E, CString& Json) {
            Json.Clear();
            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(E.what()).c_str());
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

            Log()->Error(ErrorCode == CHTTPReply::internal_server_error ? APP_LOG_EMERG : APP_LOG_NOTICE, 0, _T("ReplyError: %s"), Message.c_str());
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

            const auto &caRoot = GetRoot(pRequest->Location.Host());

            CString sResource(caRoot);
            sResource += Path;

            if (TryFiles.Count() != 0) {
                sResource = CApostolModule::TryFiles(caRoot, TryFiles, Path);
            }

            if (sResource.back() != '/' && DirectoryExists(sResource.c_str())) {
                sResource += '/';
            }

            if (sResource.back() == '/') {
                sResource += APOSTOL_INDEX_FILE;
            }

            if (FileExists(sResource.c_str())) {

                CString sFileExt;
                TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

                sFileExt = ExtractFileExt(szBuffer, sResource.c_str());

                if (AContentType == nullptr) {
                    AContentType = Mapping::ExtToType(sFileExt.c_str());
                }

                auto LModified = StrWebTime(FileAge(sResource.c_str()), szBuffer, sizeof(szBuffer));
                if (LModified != nullptr) {
                    pReply->AddHeader(_T("Last-Modified"), LModified);
                }

                pReply->Content.LoadFromFile(sResource.c_str());
                AConnection->SendReply(CHTTPReply::ok, AContentType, SendNow);

                return;
            }

            AConnection->SendStockReply(CHTTPReply::not_found, SendNow);
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
            for (int Row = 0; Row < Result->nTuples(); ++Row) {
                List.Add(Result->GetValue(Row, 0));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::PQResultToJson(CPQResult *Result, CString &Json, bool DataArray, const CString &ObjectName) {

            const auto bResultObject = !ObjectName.IsEmpty();

            DataArray = bResultObject || DataArray || Result->nTuples() > 1;

            const auto emptyData = DataArray ? _T("[]") : _T("{}");

            if (Result->nTuples() == 0) {
                Json = bResultObject ? CString().Format("{\"%s\": %s}", ObjectName.c_str(), emptyData) : emptyData;
                return;
            }

            if (bResultObject)
                Json.Format("{\"%s\": ", ObjectName.c_str());

            if (DataArray)
                Json += _T("[");

            for (int Row = 0; Row < Result->nTuples(); ++Row) {
                if (!Result->GetIsNull(Row, 0)) {
                    if (Row > 0)
                        Json += _T(",");
                    Json += Result->GetValue(Row, 0);
                } else {
                    Json += DataArray ? _T("null") : _T("{}");
                }
            }

            if (DataArray)
                Json += _T("]");

            if (bResultObject)
                Json += _T("}");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::EnumQuery(CPQResult *APQResult, CPQueryResult& AResult) {
            CStringList cFields;

            for (int I = 0; I < APQResult->nFields(); ++I) {
                cFields.Add(APQResult->fName(I));
            }

            for (int Row = 0; Row < APQResult->nTuples(); ++Row) {
                AResult.Add(CStringPairs());
                for (int Col = 0; Col < APQResult->nFields(); ++Col) {
                    if (APQResult->GetIsNull(Row, Col)) {
                        AResult.Last().AddPair(cFields[Col].c_str(), _T(""));
                    } else {
                        if (APQResult->fFormat(Col) == 0) {
                            AResult.Last().AddPair(cFields[Col].c_str(), APQResult->GetValue(Row, Col));
                        } else {
                            AResult.Last().AddPair(cFields[Col].c_str(), _T("<binary>"));
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

            if (pQuery->Start() == POLL_QUERY_START_ERROR) {
                delete pQuery;
                throw Delphi::Exception::Exception(_T("ExecSQL: Start SQL query failed."));
            }

            if (AConnection != nullptr)
                AConnection->CloseConnection(false);

            return pQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->PollConnection());

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
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }

            pConnection->SendReply(status, nullptr, true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->PollConnection());
            auto pReply = pConnection->Reply();

            CHTTPReply::CStatusType status = CHTTPReply::internal_server_error;

            ExceptionToJson(status, E, pReply->Content);
            pConnection->SendStockReply(status, true);

            Log()->Error(APP_LOG_EMERG, 0, E.what());
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

        void CApostolModule::Heartbeat() {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::WSDebugRequest(CWebSocket *ARequest) {

            size_t delta = 0;
            size_t size = ARequest->Size();

            if (size > MaxFormatStringLength) {
                delta = size - MaxFormatStringLength;
                size = MaxFormatStringLength;
            }

            CString sPayload((LPCTSTR) ARequest->Payload()->Memory() + delta, size);

            DebugMessage("[FIN: %#x; OP: %#x; MASK: %#x LEN: %d] [%d] [%d] [%d] [%d] Request:\n%s\n",
                         ARequest->Frame().FIN, ARequest->Frame().Opcode, ARequest->Frame().Mask, ARequest->Frame().Length,
                         ARequest->Size(), ARequest->Payload()->Size(), delta, size, sPayload.c_str()
            );
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::WSDebugReply(CWebSocket *AReply) {

            size_t delta = 0;
            size_t size = AReply->Size();

            if (size > MaxFormatStringLength) {
                delta = size - MaxFormatStringLength;
                size = MaxFormatStringLength;
            }

            CString sPayload((LPCTSTR) AReply->Payload()->Memory() + delta, size);

            DebugMessage("[FIN: %#x; OP: %#x; MASK: %#x] [%d] [%d] Request:\n%s\n",
                         AReply->Frame().FIN, AReply->Frame().Opcode, AReply->Frame().Mask,
                         AReply->Size(), AReply->Payload()->Size(), sPayload.c_str()
            );
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::WSDebugConnection(CHTTPServerConnection *AConnection) {

            if (AConnection->ClosedGracefully())
                return;

            DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] "), AConnection, AConnection->Socket()->Binding()->PeerIP(),
                         AConnection->Socket()->Binding()->PeerPort(), AConnection->Socket()->Binding()->Handle());

            WSDebugRequest(AConnection->WSRequest());

            static auto OnRequest = [](CObject *Sender) {
                auto pConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (pConnection->Socket()->Connected()) {
                    auto pBinding = pConnection->Socket()->Binding();
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnRequest] "), pConnection, pBinding->PeerIP(),
                                 pBinding->PeerPort(), pBinding->Handle());
                }

                WSDebugRequest(pConnection->WSRequest());
            };

            static auto OnWaitRequest = [](CObject *Sender) {
                auto pConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (pConnection->Socket()->Connected()) {
                    auto pBinding = pConnection->Socket()->Binding();
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnWaitRequest] "), pConnection,
                                 pBinding->PeerIP(),
                                 pBinding->PeerPort(), pBinding->Handle());
                }

                WSDebugRequest(pConnection->WSRequest());
            };

            static auto OnReply = [](CObject *Sender) {
                auto pConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (pConnection->Socket()->Connected()) {
                    auto pBinding = pConnection->Socket()->Binding();
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnReply] "), pConnection, pBinding->PeerIP(),
                                 pBinding->PeerPort(), pBinding->Handle());
                }

                WSDebugReply(pConnection->WSReply());
            };

            AConnection->OnWaitRequest(OnWaitRequest);
            //AConnection->OnRequest(OnRequest);
            AConnection->OnReply(OnReply);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DebugRequest(CHTTPRequest *ARequest) {
            DebugMessage(_T("[%p] Request:\n%s %s HTTP/%d.%d\n"), ARequest, ARequest->Method.c_str(), ARequest->URI.c_str(), ARequest->VMajor, ARequest->VMinor);

            for (int i = 0; i < ARequest->Headers.Count(); i++)
                DebugMessage(_T("%s: %s\n"), ARequest->Headers[i].Name().c_str(), ARequest->Headers[i].Value().c_str());

            if (!ARequest->Content.IsEmpty())
                DebugMessage(_T("\n%s\n"), ARequest->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DebugReply(CHTTPReply *AReply) {
            DebugMessage(_T("[%p] Reply:\nHTTP/%d.%d %d %s\n"), AReply, AReply->VMajor, AReply->VMinor, AReply->Status, AReply->StatusText.c_str());

            for (int i = 0; i < AReply->Headers.Count(); i++)
                DebugMessage(_T("%s: %s\n"), AReply->Headers[i].Name().c_str(), AReply->Headers[i].Value().c_str());

            if (!AReply->Content.IsEmpty())
                DebugMessage(_T("\n%s\n"), AReply->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DebugConnection(CHTTPServerConnection *AConnection) {

            DebugMessage(_T("\n[%p] [%s:%d] [%d] "), AConnection, AConnection->Socket()->Binding()->PeerIP(),
                         AConnection->Socket()->Binding()->PeerPort(), AConnection->Socket()->Binding()->Handle());

            DebugRequest(AConnection->Request());

            static auto OnReply = [](CObject *Sender) {
                auto pConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (pConnection != nullptr && !pConnection->ClosedGracefully()) {
                    auto pBinding = pConnection->Socket()->Binding();
                    if (pBinding->HandleAllocated()) {
                        DebugMessage(_T("\n[%p] [%s:%d] [%d] "), pConnection, pBinding->PeerIP(),
                                     pBinding->PeerPort(), pBinding->Handle());
                    }
                }

                DebugReply(pConnection->Reply());
            };

            AConnection->OnReply(OnReply);
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
                HeartbeatModules();
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

                Log()->Debug(0, _T("[Module] Runtime: %.2f ms."), (double) (clock() - start) / (double) CLOCKS_PER_SEC * 1000);
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

        void CModuleManager::HeartbeatModules() {
            for (int i = 0; i < ModuleCount(); i++) {
                auto Module = Modules(i);
                if (Module->Enabled())
                    Module->Heartbeat();
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

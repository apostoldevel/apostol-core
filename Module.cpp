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
            CString HexString;
            for ( int i = 0; i < size; ++i ) {
                unsigned char BinValue = byte_arr[i];
                HexString += HexCodes[(BinValue >> 4) & 0x0F];
                HexString += HexCodes[BinValue & 0x0F];
            }
            return HexString;
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
#ifdef WITH_POSTGRESQL
        //--------------------------------------------------------------------------------------------------------------

        //-- CJob ------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJob::CJob(CCollection *ACCollection) : CCollectionItem(ACCollection) {
            m_Identity = ApostolUID();
            m_pPollQuery = nullptr;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJobManager -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::Get(int Index) {
            return (CJob *) inherited::GetItem(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJobManager::Set(int Index, CJob *Value) {
            inherited::SetItem(Index, (CCollectionItem *) Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::Add(CPQPollQuery *Query) {
            auto LJob = new CJob(this);
            LJob->PollQuery(Query);
            return LJob;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::FindJobById(const CString &Id) {
            CJob *LJob = nullptr;
            for (int I = 0; I < Count(); ++I) {
                LJob = Get(I);
                if (LJob->Identity() == Id)
                    return LJob;
            }
            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::FindJobByQuery(CPQPollQuery *Query) {
            CJob *LJob = nullptr;
            for (int I = 0; I < Count(); ++I) {
                LJob = Get(I);
                if (LJob->PollQuery() == Query)
                    return LJob;
            }
            return nullptr;
        }
#endif
        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApostolModule::CApostolModule(CModuleProcess *AProcess, const CString& ModuleName): CCollectionItem(AProcess),
            CGlobalComponent(), m_pModuleProcess(AProcess), m_ModuleName(ModuleName) {

            m_ModuleStatus = msUnknown;
            m_Sniffer = false;

            m_Version = -1;
#ifdef WITH_POSTGRESQL
            m_pJobs = new CJobManager();
#endif
            m_pMethods = CStringList::Create(true);
            m_Headers.Add("Content-Type");
        }
        //--------------------------------------------------------------------------------------------------------------

        CApostolModule::~CApostolModule() {
#ifdef WITH_POSTGRESQL
            delete m_pJobs;
#endif
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
                    CMethodHandler *Handler;
                    for (int i = 0; i < m_pMethods->Count(); ++i) {
                        Handler = (CMethodHandler *) m_pMethods->Objects(i);
                        if (Handler->Allow()) {
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
            auto LReply = AConnection->Reply();

            CReply::GetStockReply(LReply, CReply::not_allowed);

            if (!AllowedMethods().IsEmpty())
                LReply->AddHeader(_T("Allow"), AllowedMethods());

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
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CApostolModule::GetRoot(const CString &Host) const {
            auto Index = m_Sites.IndexOfName(Host);
            if (Index == -1)
                return m_Sites["*"].Value();
            return m_Sites[Index].Value();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CStringList &CApostolModule::GetSiteConfig(const CString &Host) const {
            auto Index = m_Sites.IndexOfName(Host);
            if (Index == -1)
                return m_Sites["*"].Data();
            return m_Sites[Index].Data();
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetUserAgent(CHTTPServerConnection *AConnection) {
            auto LServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto LRequest = AConnection->Request();
            const auto& LAgent = LRequest->Headers.Values(_T("User-Agent"));
            return LAgent.IsEmpty() ? LServer->ServerName() : LAgent;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetOrigin(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            const auto& LOrigin = LRequest->Headers.Values(_T("Origin"));
            return LOrigin.IsEmpty() ? LRequest->Location.Origin() : LOrigin;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetProtocol(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            const auto& LProtocol = LRequest->Headers.Values(_T("X-Forwarded-Proto"));
            return LProtocol.IsEmpty() ? LRequest->Location.protocol : LProtocol;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetHost(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            CString Host;

            const auto& LRealIP = LRequest->Headers.Values(_T("X-Real-IP"));
            if (!LRealIP.IsEmpty()) {
                Host = LRealIP;
            } else {
                auto LBinding = AConnection->Socket()->Binding();
                if (LBinding != nullptr) {
                    Host = LBinding->PeerIP();
                }
            }

            return Host;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ExceptionToJson(int ErrorCode, const std::exception &e, CString& Json) {
            Json.Clear();
            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(e.what()).c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ContentToJson(CRequest *ARequest, CJSON &Json) {

            const auto& ContentType = ARequest->Headers.Values(_T("Content-Type")).Lower();

            if (ContentType.Find("application/x-www-form-urlencoded") != CString::npos) {

                const CStringList &formData = ARequest->FormData;

                auto& jsonObject = Json.Object();
                for (int i = 0; i < formData.Count(); ++i) {
                    jsonObject.AddPair(formData.Names(i), formData.ValueFromIndex(i));
                }

            } else if (ContentType.Find("multipart/form-data") != CString::npos) {

                CFormData formData;
                CRequestParser::ParseFormData(ARequest, formData);

                auto& jsonObject = Json.Object();
                for (int i = 0; i < formData.Count(); ++i) {
                    jsonObject.AddPair(formData[i].Name, formData[i].Data);
                }

            } else if (ContentType.Find("application/json") != CString::npos) {

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

            const auto ResultObject = !ObjectName.IsEmpty();

            DataArray = ResultObject || DataArray || List.Count() > 1;

            const auto EmptyData = DataArray ? _T("[]") : _T("{}");

            if (List.Count() == 0) {
                Json = ResultObject ? CString().Format("{\"%s\": %s}", ObjectName.c_str(), EmptyData) : EmptyData;
                return;
            }

            if (ResultObject)
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

            if (ResultObject)
                Json += _T("}");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::Redirect(CHTTPServerConnection *AConnection, const CString& Location, bool SendNow) {
            auto LReply = AConnection->Reply();

            LReply->Content.Clear();
            LReply->AddHeader(_T("Location"), Location);

            AConnection->Data().Values("redirect", CString());
            AConnection->Data().Values("redirect_error", CString());

            AConnection->SendStockReply(CReply::moved_temporarily, SendNow);
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

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            const auto &Root = GetRoot(LRequest->Location.Host());

            CString LResource(Root);
            LResource += Path;

            if (TryFiles.Count() != 0) {
                LResource = CApostolModule::TryFiles(Root, TryFiles, Path);
            }

            if (LResource.back() != '/' && DirectoryExists(LResource.c_str())) {
                LResource += '/';
            }

            if (LResource.back() == '/') {
                LResource += APOSTOL_INDEX_FILE;
            }

            if (FileExists(LResource.c_str())) {

                CString LFileExt;
                TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

                LFileExt = ExtractFileExt(szBuffer, LResource.c_str());

                if (AContentType == nullptr) {
                    AContentType = Mapping::ExtToType(LFileExt.c_str());
                }

                auto LModified = StrWebTime(FileAge(LResource.c_str()), szBuffer, sizeof(szBuffer));
                if (LModified != nullptr) {
                    LReply->AddHeader(_T("Last-Modified"), LModified);
                }

                LReply->Content.LoadFromFile(LResource.c_str());
                AConnection->SendReply(CReply::ok, AContentType, SendNow);

                return;
            }

            AConnection->SendStockReply(CReply::not_found, SendNow);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoHead(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            CString LPath(LRequest->Location.pathname);

            // Request path must be absolute and not contain "..".
            if (LPath.empty() || LPath.front() != '/' || LPath.find("..") != CString::npos) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            // If path ends in slash.
            if (LPath.back() == '/') {
                LPath += "index.html";
            }

            CString LResource(GetRoot(LRequest->Location.Host()));
            LResource += LPath;

            if (!FileExists(LResource.c_str())) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

            auto contentType = Mapping::ExtToType(ExtractFileExt(szBuffer, LResource.c_str()));
            if (contentType != nullptr) {
                LReply->AddHeader(_T("Content-Type"), contentType);
            }

            auto fileSize = FileSize(LResource.c_str());
            LReply->AddHeader(_T("Content-Length"), IntToStr((int) fileSize, szBuffer, sizeof(szBuffer)));

            auto LModified = StrWebTime(FileAge(LResource.c_str()), szBuffer, sizeof(szBuffer));
            if (LModified != nullptr)
                LReply->AddHeader(_T("Last-Modified"), LModified);

            AConnection->SendReply(CReply::no_content);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoOptions(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            CReply::GetStockReply(LReply, CReply::no_content);

            if (!AllowedMethods().IsEmpty())
                LReply->AddHeader(_T("Allow"), AllowedMethods());

            AConnection->SendReply();
#ifdef _DEBUG
            if (LRequest->URI == _T("/reload"))
                GApplication->SignalProcess()->SignalReload();

            if (LRequest->URI == _T("/quit"))
                GApplication->SignalProcess()->SignalQuit();
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoGet(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();

            CString LPath(LRequest->Location.pathname);

            // Request path must be absolute and not contain "..".
            if (LPath.empty() || LPath.front() != '/' || LPath.find(_T("..")) != CString::npos) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            SendResource(AConnection, LPath);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::CORS(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            const CHeaders& LRequestHeaders = LRequest->Headers;
            CHeaders& LReplyHeaders = LReply->Headers;

            const CString& Origin = LRequestHeaders.Values(_T("origin"));
            if (!Origin.IsEmpty()) {
                LReplyHeaders.AddPair(_T("Access-Control-Allow-Origin"), Origin);
                LReplyHeaders.AddPair(_T("Access-Control-Allow-Methods"), AllowedMethods());
                LReplyHeaders.AddPair(_T("Access-Control-Allow-Headers"), AllowedHeaders());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPClient *CApostolModule::GetClient(const CString &Host, uint16_t Port) {
            return m_pModuleProcess->GetClient(Host.c_str(), Port);
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

            const auto ResultObject = !ObjectName.IsEmpty();

            DataArray = ResultObject || DataArray || Result->nTuples() > 1;

            const auto EmptyData = DataArray ? _T("[]") : _T("{}");

            if (Result->nTuples() == 0) {
                Json = ResultObject ? CString().Format("{\"%s\": %s}", ObjectName.c_str(), EmptyData) : EmptyData;
                return;
            }

            if (ResultObject)
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

            if (ResultObject)
                Json += _T("}");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::EnumQuery(CPQResult *APQResult, CPQueryResult& AResult) {
            CStringList LFields;

            for (int I = 0; I < APQResult->nFields(); ++I) {
                LFields.Add(APQResult->fName(I));
            }

            for (int Row = 0; Row < APQResult->nTuples(); ++Row) {
                AResult.Add(CStringPairs());
                for (int Col = 0; Col < APQResult->nFields(); ++Col) {
                    if (APQResult->GetIsNull(Row, Col)) {
                        AResult.Last().AddPair(LFields[Col].c_str(), _T(""));
                    } else {
                        if (APQResult->fFormat(Col) == 0) {
                            AResult.Last().AddPair(LFields[Col].c_str(), APQResult->GetValue(Row, Col));
                        } else {
                            AResult.Last().AddPair(LFields[Col].c_str(), _T("<binary>"));
                        }
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::QueryToResults(CPQPollQuery *APollQuery, CPQueryResults& AResults) {
            CPQResult *LResult = nullptr;
            int Index;
            for (int i = 0; i < APollQuery->ResultCount(); ++i) {
                LResult = APollQuery->Results(i);
                if (LResult->ExecStatus() == PGRES_TUPLES_OK || LResult->ExecStatus() == PGRES_SINGLE_TUPLE) {
                    Index = AResults.Add(CPQueryResult());
                    EnumQuery(LResult, AResults[Index]);
                } else {
                    throw Delphi::Exception::EDBError(LResult->GetErrorMessage());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApostolModule::StartQuery(CHTTPServerConnection *AConnection, const CStringList& SQL) {

            auto LQuery = m_Version == 2 ? GetQuery(nullptr) : GetQuery(AConnection);

            if (LQuery == nullptr)
                throw Delphi::Exception::Exception(_T("StartQuery: GetQuery() failed!"));

            LQuery->SQL() = SQL;

            if (LQuery->Start() != POLL_QUERY_START_ERROR) {
                if (m_Version == 2) {
                    auto LJob = m_pJobs->Add(LQuery);
                    LJob->Data() = AConnection->Data();

                    auto LReply = AConnection->Reply();
                    LReply->Content = _T("{\"identity\":" "\"") + LJob->Identity() + _T("\"}");

                    AConnection->SendReply(CReply::accepted);
                    AConnection->CloseConnection(true);
                } else {
                    // Wait query result...
                    AConnection->CloseConnection(false);
                }

                return true;
            } else {
                delete LQuery;
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CApostolModule::GetQuery(CPollConnection *AConnection) {
            CPQPollQuery *LQuery = m_pModuleProcess->GetQuery(AConnection);

            if (Assigned(LQuery)) {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
                LQuery->OnPollExecuted([this](auto && APollQuery) { DoPostgresQueryExecuted(APollQuery); });
                LQuery->OnException([this](auto && APollQuery, auto && AException) { DoPostgresQueryException(APollQuery, AException); });
#else
                LQuery->OnPollExecuted(std::bind(&CApostolModule::DoPostgresQueryExecuted, this, _1));
                LQuery->OnException(std::bind(&CApostolModule::DoPostgresQueryException, this, _1, _2));
#endif
            }

            return LQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApostolModule::ExecSQL(const CStringList &SQL, CPollConnection *AConnection,
                COnPQPollQueryExecutedEvent &&OnExecuted, COnPQPollQueryExceptionEvent &&OnException) {

            auto LQuery = GetQuery(AConnection);

            if (LQuery == nullptr)
                throw Delphi::Exception::Exception(_T("ExecSQL: GetQuery() failed!"));

            if (OnExecuted != nullptr)
                LQuery->OnPollExecuted(static_cast<COnPQPollQueryExecutedEvent &&>(OnExecuted));

            if (OnException != nullptr)
                LQuery->OnException(static_cast<COnPQPollQueryExceptionEvent &&>(OnException));

            LQuery->SQL() = SQL;

            if (LQuery->Start() != POLL_QUERY_START_ERROR) {
                return true;
            } else {
                delete LQuery;
            }

            Log()->Error(APP_LOG_ALERT, 0, _T("ExecSQL: StartQuery() failed!"));

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {
            clock_t start = clock();

            auto LConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->PollConnection());

            auto LReply = LConnection->Reply();
            auto LResult = APollQuery->Results(0);

            CReply::CStatusType LStatus = CReply::internal_server_error;

            try {
                if (LResult->ExecStatus() != PGRES_TUPLES_OK)
                    throw Delphi::Exception::EDBError(LResult->GetErrorMessage());

                LStatus = CReply::ok;
                Postgres::PQResultToJson(LResult, LReply->Content);
            } catch (Delphi::Exception::Exception &E) {
                ExceptionToJson(LStatus, E, LReply->Content);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }

            LConnection->SendReply(LStatus, nullptr, true);

            log_debug1(APP_LOG_DEBUG_CORE, Log(), 0, _T("Query executed runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) {

            auto LConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->PollConnection());
            auto LReply = LConnection->Reply();

            CReply::CStatusType LStatus = CReply::internal_server_error;

            ExceptionToJson(LStatus, *AException, LReply->Content);
            LConnection->SendStockReply(LStatus, true);

            Log()->Error(APP_LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
        void CApostolModule::Execute(CHTTPServerConnection *AConnection) {
            auto LServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            if (m_Sites.Count() == 0)
                InitSites(LServer->Sites());
#ifdef _DEBUG
            DebugConnection(AConnection);
#endif
            LReply->Clear();
            LReply->ContentType = CReply::html;

            int i;
            CMethodHandler *Handler;
            for (i = 0; i < m_pMethods->Count(); ++i) {
                Handler = (CMethodHandler *) m_pMethods->Objects(i);
                if (Handler->Allow()) {
                    const CString& Method = m_pMethods->Strings(i);
                    if (Method == LRequest->Method) {
                        CORS(AConnection);
                        Handler->Handler(AConnection);
                        break;
                    }
                }
            }

            if (i == m_pMethods->Count()) {
                AConnection->SendStockReply(CReply::not_implemented);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApostolModule::CheckConnection(CHTTPServerConnection *AConnection) {
            return Enabled();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::Heartbeat() {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::WSDebugRequest(CWebSocket *ARequest) {

            size_t Delta = 0;
            size_t Size = ARequest->Size();

            if (Size > MaxFormatStringLength) {
                Delta = Size - MaxFormatStringLength;
                Size = MaxFormatStringLength;
            }

            CString Payload((LPCTSTR) ARequest->Payload()->Memory() + Delta, Size);

            DebugMessage("[FIN: %#x; OP: %#x; MASK: %#x] [%d] [%d] Request:\n%s\n",
                         ARequest->Frame().FIN, ARequest->Frame().Opcode, ARequest->Frame().Mask,
                         ARequest->Size(), ARequest->Payload()->Size(), Payload.c_str()
            );
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::WSDebugReply(CWebSocket *AReply) {

            size_t Delta = 0;
            size_t Size = AReply->Size();

            if (Size > MaxFormatStringLength) {
                Delta = Size - MaxFormatStringLength;
                Size = MaxFormatStringLength;
            }

            CString Payload((LPCTSTR) AReply->Payload()->Memory() + Delta, Size);

            DebugMessage("[FIN: %#x; OP: %#x; MASK: %#x] [%d] [%d] Request:\n%s\n",
                         AReply->Frame().FIN, AReply->Frame().Opcode, AReply->Frame().Mask,
                         AReply->Size(), AReply->Payload()->Size(), Payload.c_str()
            );
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::WSDebugConnection(CHTTPServerConnection *AConnection) {

            DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] "), AConnection, AConnection->Socket()->Binding()->PeerIP(),
                         AConnection->Socket()->Binding()->PeerPort(), AConnection->Socket()->Binding()->Handle());

            WSDebugRequest(AConnection->WSRequest());

            static auto OnRequest = [](CObject *Sender) {
                auto LConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (LConnection->Socket()->Connected()) {
                    auto LBinding = LConnection->Socket()->Binding();
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnRequest] "), LConnection, LBinding->PeerIP(),
                                 LBinding->PeerPort(), LBinding->Handle());
                }

                WSDebugRequest(LConnection->WSRequest());
            };

            static auto OnWaitRequest = [](CObject *Sender) {
                auto LConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (LConnection->Socket()->Connected()) {
                    auto LBinding = LConnection->Socket()->Binding();
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnWaitRequest] "), LConnection,
                                 LBinding->PeerIP(),
                                 LBinding->PeerPort(), LBinding->Handle());
                }

                WSDebugRequest(LConnection->WSRequest());
            };

            static auto OnReply = [](CObject *Sender) {
                auto LConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (LConnection->Socket()->Connected()) {
                    auto LBinding = LConnection->Socket()->Binding();
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnReply] "), LConnection, LBinding->PeerIP(),
                                 LBinding->PeerPort(), LBinding->Handle());
                }

                WSDebugReply(LConnection->WSReply());
            };

            AConnection->OnWaitRequest(OnWaitRequest);
            //AConnection->OnRequest(OnRequest);
            AConnection->OnReply(OnReply);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DebugRequest(CRequest *ARequest) {
            DebugMessage(_T("[%p] Request:\n%s %s HTTP/%d.%d\n"), ARequest, ARequest->Method.c_str(), ARequest->URI.c_str(), ARequest->VMajor, ARequest->VMinor);

            for (int i = 0; i < ARequest->Headers.Count(); i++)
                DebugMessage(_T("%s: %s\n"), ARequest->Headers[i].Name().c_str(), ARequest->Headers[i].Value().c_str());

            if (!ARequest->Content.IsEmpty())
                DebugMessage(_T("\n%s\n"), ARequest->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DebugReply(CReply *AReply) {
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
                auto LConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                if (LConnection->Socket()->Connected()) {
                    auto LBinding = LConnection->Socket()->Binding();
                    if (LBinding->HandleAllocated()) {
                        DebugMessage(_T("\n[%p] [%s:%d] [%d] "), LConnection, LBinding->PeerIP(),
                                     LBinding->PeerPort(), LBinding->Handle());
                    }
                }

                DebugReply(LConnection->Reply());
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

            auto LTimer = dynamic_cast<CEPollTimer *> (AHandler->Binding());
            LTimer->Read(&exp, sizeof(uint64_t));

            try {
                HeartbeatModules();
            } catch (Delphi::Exception::Exception &E) {
                DoServerEventHandlerException(AHandler, &E);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CModuleProcess::DoExecute(CTCPConnection *AConnection) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);

            try {
                clock_t start = clock();

                ExecuteModules(LConnection);

                Log()->Debug(0, _T("[Module] Runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
            } catch (Delphi::Exception::Exception &E) {
                DoServerException(LConnection, &E);
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

        void CModuleManager::HeartbeatModules() {
            for (int i = 0; i < ModuleCount(); i++) {
                auto Module = Modules(i);
                if (Module->Enabled())
                    Module->Heartbeat();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleManager::ExecuteModule(CHTTPServerConnection *AConnection, CApostolModule *AModule) {
            DoBeforeExecuteModule(AModule);
            try {
                AModule->Execute(AConnection);
            } catch (...) {
                AConnection->SendStockReply(CReply::internal_server_error);
                DoAfterExecuteModule(AModule);
                throw;
            }
            DoAfterExecuteModule(AModule);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleManager::ExecuteModules(CTCPConnection *AConnection) {

            auto LConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);
            auto LRequest = LConnection->Request();

            int Index = 0;
            while (Index < ModuleCount()) {
                const auto Module = Modules(Index);
                if (Module->Enabled() && Module->CheckConnection(LConnection)) {
                    ExecuteModule(LConnection, Module);
                    if (!Module->Sniffer())
                        break;
                }
                Index++;
            }

            if (Index == ModuleCount()) {
                LConnection->SendStockReply(CReply::forbidden);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}

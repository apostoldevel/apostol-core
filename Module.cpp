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

extern "C++" {

namespace Apostol {

    namespace Module {

        LPCTSTR StrWebTime(time_t Time, LPTSTR lpszBuffer, size_t Size) {
            struct tm *gmt;

            gmt = gmtime(&Time);

            if ((gmt != nullptr) && (strftime(lpszBuffer, Size, "%a, %d %b %Y %T %Z", gmt) != 0)) {
                return lpszBuffer;
            }

            return nullptr;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApostolModule::CApostolModule(CModuleProcess *AProcess, const CString& ModuleName, const CString& SectionName):
            CCollectionItem(AProcess), CGlobalComponent(), m_pModuleProcess(AProcess), m_ModuleName(ModuleName),
            m_SectionName(SectionName) {

            m_ModuleStatus = msUnknown;

            m_Headers.Add("Content-Type");
            m_Headers.Add("X-Requested-With");
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPServer &CApostolModule::Server() {
            return m_pModuleProcess->Server();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CHTTPServer &CApostolModule::Server() const {
            return m_pModuleProcess->Server();
        }
        //--------------------------------------------------------------------------------------------------------------
#ifdef WITH_POSTGRESQL
        CPQClient &CApostolModule::PQClient() {
            return m_pModuleProcess->GetPQClient();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CPQClient &CApostolModule::PQClient() const {
            return m_pModuleProcess->GetPQClient();
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQClient &CApostolModule::PQClient(const CString &ConfName) {
            return m_pModuleProcess->GetPQClient(ConfName);
        }
        //--------------------------------------------------------------------------------------------------------------

        const CPQClient &CApostolModule::PQClient(const CString &ConfName) const {
            return m_pModuleProcess->GetPQClient(ConfName);
        }
#endif
        const CString &CApostolModule::GetAllowedMethods() const {
            if (m_AllowedMethods.IsEmpty()) {
                if (m_Methods.Count() > 0) {
                    CMethodHandler *pHandler;
                    for (int i = 0; i < m_Methods.Count(); ++i) {
                        pHandler = (CMethodHandler *) m_Methods.Objects(i);
                        if (pHandler->Allow()) {
                            if (m_AllowedMethods.IsEmpty())
                                m_AllowedMethods = m_Methods.Strings(i);
                            else
                                m_AllowedMethods += _T(", ") + m_Methods.Strings(i);
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
            auto &Reply = AConnection->Reply();

            CHTTPReply::InitStockReply(Reply, CHTTPReply::not_allowed);

            if (!AllowedMethods().IsEmpty())
                Reply.AddHeader(_T("Allow"), AllowedMethods());

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

        CString CApostolModule::GetRoot(const CString &Host) const {
            CString Result(GetSiteRoot(Host));

            if (!path_separator(Result.front())) {
                Result = Config()->Prefix() + Result;
            }

            if (path_separator(Result.back())) {
                Result.SetLength(Result.Length() - 1);
            }

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CApostolModule::GetSiteRoot(const CString &Host) const {
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
            const auto &caRequest = AConnection->Request();
            const auto &agent = caRequest.Headers[_T("User-Agent")];
            return agent.IsEmpty() ? pServer->ServerName() : agent;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetOrigin(CHTTPServerConnection *AConnection) {
            const auto &caRequest = AConnection->Request();
            const auto &caOrigin = caRequest.Headers[_T("Origin")];
            return caOrigin.IsEmpty() ? caRequest.Location.Origin() : caOrigin;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetProtocol(CHTTPServerConnection *AConnection) {
            const auto &caRequest = AConnection->Request();
            const auto &caProtocol = caRequest.Headers[_T("X-Forwarded-Proto")];
            return caProtocol.IsEmpty() ? caRequest.Location.protocol : caProtocol;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CApostolModule::GetRealIP(CHTTPServerConnection *AConnection) {
            const auto &caRequest = AConnection->Request();
            CString sHost;

            const auto &caRealIP = caRequest.Headers[_T("X-Real-IP")];
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
            const auto &caRequest = AConnection->Request();
            const CString sHost(caRequest.Headers[_T("Host")]);
            return sHost.IsEmpty() ? "localhost" : sHost;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ExceptionToJson(int ErrorCode, const std::exception &e, CString& Json) {
            Json.Clear();
            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(e.what()).c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::ContentToJson(const CHTTPRequest &Request, CJSON &Json) {

            const auto &caContentType = Request.Headers[_T("Content-Type")].Lower();

            if (caContentType.Find("application/x-www-form-urlencoded") != CString::npos) {

                const auto &formData = Request.FormData;

                auto& jsonObject = Json.Object();
                for (int i = 0; i < formData.Count(); ++i) {
                    jsonObject.AddPair(formData.Names(i), formData.ValueFromIndex(i));
                }

            } else if (caContentType.Find("multipart/form-data") != CString::npos) {

                CFormData formData;
                CHTTPRequestParser::ParseFormData(Request, formData);

                auto& jsonObject = Json.Object();
                for (int i = 0; i < formData.Count(); ++i) {
                    jsonObject.AddPair(formData[i].Name, formData[i].Data);
                }

            } else if (caContentType.Find("application/json") != CString::npos) {

                Json << Request.Content;

            } else {

                auto& jsonObject = Json.Object();
                for (int i = 0; i < Request.Params.Count(); ++i) {
                    jsonObject.AddPair(Request.Params.Names(i), CHTTPServer::URLDecode(Request.Params.ValueFromIndex(i)));
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
            Log()->Error(ErrorCode == CHTTPReply::internal_server_error ? APP_LOG_EMERG : APP_LOG_ERR, 0, _T("ReplyError: %s"), Message.c_str());

            if (AConnection == nullptr)
                return;

            auto &Reply = AConnection->Reply();

            Reply.ContentType = CHTTPReply::json;

            if (ErrorCode == CHTTPReply::unauthorized) {
                CHTTPReply::AddUnauthorized(Reply, AConnection->Data()["Authorization"] != "Basic", "invalid_client", Message.c_str());
            }

            Reply.Content.Clear();
            Reply.Content.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(Message).c_str());

            AConnection->CloseConnection(true);
            AConnection->SendReply(ErrorCode, nullptr, true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::Redirect(CHTTPServerConnection *AConnection, const CString& Location, bool SendNow) const {
            auto &Reply = AConnection->Reply();

            Reply.Content.Clear();
            Reply.AddHeader(_T("Location"), Location);

            AConnection->Data().Values("redirect", CString());
            AConnection->Data().Values("redirect_error", CString());

            AConnection->SendStockReply(CHTTPReply::moved_temporarily, SendNow, GetRoot(GetHost(AConnection)));
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

        bool CApostolModule::ResourceExists(CString &Resource, const CString &Root, const CString &Path, const CStringList &TryFiles) {
            Resource = Root;
            Resource += Path;

            if (DirectoryExists(Resource.c_str())) {
                if (!path_separator(Resource.back())) {
                    Resource += '/';
                }
                Resource += APOSTOL_INDEX_FILE;
            } else if (path_separator(Resource.back())) {
                Resource.SetLength(Resource.Length() - 1);
            }

            if (TryFiles.Count() != 0 && !FileExists(Resource.c_str())) {
                Resource = CApostolModule::TryFiles(Root, TryFiles, Path);
            }

            return FileExists(Resource.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApostolModule::SendResource(CHTTPServerConnection *AConnection, const CString &Path,
                LPCTSTR AContentType, bool SendNow, const CStringList& TryFiles, bool SendNotFound) const {

            const auto &caRequest = AConnection->Request();
            auto &Reply = AConnection->Reply();

            const CString sRoot(GetRoot(GetHost(AConnection)));
            CString sResource;

            if (!ResourceExists(sResource, sRoot, Path, TryFiles)) {
                if (SendNotFound) {
                    AConnection->SendStockReply(CHTTPReply::not_found, SendNow, sRoot);
                }
                return false;
            }

            TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

            if (AContentType == nullptr) {
                CString sFileExt;
                sFileExt = ExtractFileExt(szBuffer, sResource.c_str());
                AContentType = Mapping::ExtToType(sFileExt.c_str());
            }

            auto sModified = StrWebTime(FileAge(sResource.c_str()), szBuffer, sizeof(szBuffer));
            if (sModified != nullptr) {
                Reply.AddHeader(_T("Last-Modified"), sModified);
            }

#if (OPENSSL_VERSION_NUMBER >= 0x30000000L) && defined(BIO_get_ktls_send)
            AConnection->SendFileReply(sResource.c_str(), AContentType);
#else
            if (AConnection->IOHandler()->UsedSSL()) {
                Reply.Content.LoadFromFile(sResource.c_str());
                AConnection->SendReply(CHTTPReply::ok, AContentType, SendNow);
            } else {
                AConnection->SendFileReply(sResource.c_str(), AContentType);
            }
#endif
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoHead(CHTTPServerConnection *AConnection) {
            const auto &caRequest = AConnection->Request();
            auto &Reply = AConnection->Reply();

            CString sPath(caRequest.Location.pathname);

            // Request sPath must be absolute and not contain "..".
            if (sPath.empty() || sPath.front() != '/' || sPath.find("..") != CString::npos) {
                AConnection->SendStockReply(CHTTPReply::bad_request, false, GetRoot(GetHost(AConnection)));
                return;
            }

            // If path ends in slash.
            if (sPath.back() == '/') {
                sPath += "index.html";
            }

            CString sResource(GetRoot(caRequest.Location.Host()));
            sResource += sPath;

            if (!FileExists(sResource.c_str())) {
                AConnection->SendStockReply(CHTTPReply::not_found, false, GetRoot(GetHost(AConnection)));
                return;
            }

            TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

            auto contentType = Mapping::ExtToType(ExtractFileExt(szBuffer, sResource.c_str()));
            if (contentType != nullptr) {
                Reply.AddHeader(_T("Content-Type"), contentType);
            }

            auto fileSize = FileSize(sResource.c_str());
            Reply.AddHeader(_T("Content-Length"), IntToStr((int) fileSize, szBuffer, sizeof(szBuffer)));

            auto LModified = StrWebTime(FileAge(sResource.c_str()), szBuffer, sizeof(szBuffer));
            if (LModified != nullptr)
                Reply.AddHeader(_T("Last-Modified"), LModified);

            AConnection->SendReply(CHTTPReply::no_content);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoOptions(CHTTPServerConnection *AConnection) {
            const auto &caRequest = AConnection->Request();
            auto &Reply = AConnection->Reply();

            CHTTPReply::InitStockReply(Reply, CHTTPReply::no_content);

            if (!AllowedMethods().IsEmpty())
                Reply.AddHeader(_T("Allow"), AllowedMethods());

            AConnection->SendReply();
#ifdef _DEBUG
            if (caRequest.URI == _T("/reload"))
                GApplication->SignalProcess()->SignalReload();

            if (caRequest.URI == _T("/quit"))
                GApplication->SignalProcess()->SignalQuit();
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoGet(CHTTPServerConnection *AConnection) {
            const auto &caRequest = AConnection->Request();

            CString sPath(caRequest.Location.pathname);

            // Request sPath must be absolute and not contain "..".
            if (sPath.empty() || sPath.front() != '/' || sPath.find(_T("..")) != CString::npos) {
                AConnection->SendStockReply(CHTTPReply::bad_request, false, GetRoot(GetHost(AConnection)));
                return;
            }

            SendResource(AConnection, sPath);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::CORS(CHTTPServerConnection *AConnection) {
            const auto &caRequest = AConnection->Request();
            auto &Reply = AConnection->Reply();

            const auto& caRequestHeaders = caRequest.Headers;
            auto &ReplyHeaders = Reply.Headers;

            const auto& caOrigin = caRequestHeaders[_T("origin")];
            if (!caOrigin.IsEmpty()) {
                ReplyHeaders.AddPair(_T("Access-Control-Allow-Origin"), caOrigin);
                ReplyHeaders.AddPair(_T("Access-Control-Allow-Methods"), AllowedMethods());
                ReplyHeaders.AddPair(_T("Access-Control-Allow-Headers"), AllowedHeaders());
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

        CPQPollQuery *CApostolModule::ExecuteSQL(const CStringList &SQL, CHTTPServerConnection *AConnection,
                COnApostolModuleSuccessEvent &&OnSuccess, COnApostolModuleFailEvent &&OnFail) {

            auto OnExecuted = [OnSuccess](CPQPollQuery *APollQuery) {
                auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());
                if (pConnection != nullptr && pConnection->Connected()) {
                    OnSuccess(pConnection, APollQuery);
                }
            };

            auto OnException = [OnFail](CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {
                auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());
                if (pConnection != nullptr && pConnection->Connected()) {
                    OnFail(pConnection, E);
                }
            };

            CPQPollQuery *pQuery = nullptr;

            try {
                pQuery = ExecSQL(SQL, AConnection, OnExecuted, OnException);
            } catch (Delphi::Exception::Exception &E) {
                OnFail(AConnection, E);
            }

            return pQuery;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresNotify(CPQConnection *AConnection, PGnotify *ANotify) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());

            auto &Reply = pConnection->Reply();
            auto pResult = APollQuery->Results(0);

            CHTTPReply::CStatusType status = CHTTPReply::internal_server_error;

            try {
                if (pResult->ExecStatus() != PGRES_TUPLES_OK)
                    throw Delphi::Exception::EDBError(pResult->GetErrorMessage());

                status = CHTTPReply::ok;
                Postgres::PQResultToJson(pResult, Reply.Content);
            } catch (Delphi::Exception::Exception &E) {
                ExceptionToJson(status, E, Reply.Content);
                Log()->Error(APP_LOG_ERR, 0, "%s", E.what());
            }

            pConnection->SendReply(status, nullptr, true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApostolModule::DoPostgresQueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {

            auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());
            auto &Reply = pConnection->Reply();

            CHTTPReply::CStatusType status = CHTTPReply::internal_server_error;

            ExceptionToJson(status, E, Reply.Content);
            pConnection->SendStockReply(status, true, GetRoot(GetHost(pConnection)));

            Log()->Error(APP_LOG_ERR, 0, "%s", E.what());
        }
        //--------------------------------------------------------------------------------------------------------------
#endif
        bool CApostolModule::Execute(CHTTPServerConnection *AConnection) {

            auto pServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            const auto &caRequest = AConnection->Request();
            auto &Reply = AConnection->Reply();

            if (!CheckLocation(caRequest.Location))
                return false;

            if (m_Sites.Count() == 0)
                InitSites(pServer->Sites());
#ifdef _DEBUG
            DebugConnection(AConnection);
#endif
            Reply.Clear();
            Reply.ContentType = CHTTPReply::html;

            int i;
            CMethodHandler *pHandler;
            for (i = 0; i < m_Methods.Count(); ++i) {
                pHandler = (CMethodHandler *) m_Methods.Objects(i);
                if (pHandler->Allow()) {
                    const CString& Method = m_Methods.Strings(i);
                    if (Method == caRequest.Method) {
                        CORS(AConnection);
                        pHandler->Handler(AConnection);
                        break;
                    }
                }
            }

            if (i == m_Methods.Count()) {
                AConnection->SendStockReply(CHTTPReply::not_implemented, false, GetRoot(GetHost(AConnection)));
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
                ExecuteModules(pConnection);
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
                pConnection->SendStockReply(CHTTPReply::not_implemented);
            }
        }

    }
}
}

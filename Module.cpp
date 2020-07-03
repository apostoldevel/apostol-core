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
        const CString &CApostolModule::GetAllowedMethods(CString &AllowedMethods) const {
            if (AllowedMethods.IsEmpty()) {
                if (m_pMethods->Count() > 0) {
                    CMethodHandler *Handler;
                    for (int i = 0; i < m_pMethods->Count(); ++i) {
                        Handler = (CMethodHandler *) m_pMethods->Objects(i);
                        if (Handler->Allow()) {
                            if (AllowedMethods.IsEmpty())
                                AllowedMethods = m_pMethods->Strings(i);
                            else
                                AllowedMethods += _T(", ") + m_pMethods->Strings(i);
                        }
                    }
                }
            }
            return AllowedMethods;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CApostolModule::GetAllowedHeaders(CString &AllowedHeaders) const {
            if (AllowedHeaders.IsEmpty()) {
                for (int i = 0; i < m_Headers.Count(); ++i) {
                    if (AllowedHeaders.IsEmpty())
                        AllowedHeaders = m_Headers.Strings(i);
                    else
                        AllowedHeaders += _T(", ") + m_Headers.Strings(i);
                }
            }
            return AllowedHeaders;
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

        void CApostolModule::InitRoots(const CSites &Sites) {
            for (int i = 0; i < Sites.Count(); ++i) {
                const auto& Site = Sites[i];
                if (Site.Name() != "default") {
                    const auto& Hosts = Site.Value()["hosts"];
                    const auto& Root = Site.Value()["root"].AsString();
                    if (!Hosts.IsNull()) {
                        for (int l = 0; l < Hosts.Count(); ++l)
                            m_Roots.AddPair(Hosts[l].AsString(), Root);
                    } else {
                        m_Roots.AddPair(Site.Name(), Root);
                    }
                }
            }

            if (Sites.Default().Value().IsObject()) {
                m_Roots.AddPair("*", Sites.Default().Value()["root"].AsString());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CApostolModule::GetRoot(const CString &Host) const {
            auto Index = m_Roots.IndexOfName(Host);
            if (Index == -1)
                return m_Roots["*"].Value();
            return m_Roots[Index].Value();
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

        void CApostolModule::ListToJson(const CStringList &List, CString &Json, bool IsArray) {
            IsArray = IsArray || List.Count() > 1;

            if (IsArray)
                Json = _T("[");

            for (int i = 0; i < List.Count(); ++i) {
                const auto& Line = List[i];
                if (!Line.IsEmpty()) {
                    if (i > 0)
                        Json += _T(",");
                    Json += Line;
                }
            }

            if (IsArray)
                Json += _T("]");
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

        void CApostolModule::SendResource(CHTTPServerConnection *AConnection, const CString &Path,
                LPCTSTR AContentType, bool SendNow) const {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            const auto &Root = GetRoot(LRequest->Location.Host());

            CString LResource(Root);
            LResource += Path;

            if (LResource.back() == '/') {
                LResource += "index.html";
            }

            TCHAR szBuffer[MAX_BUFFER_SIZE + 1] = {0};

            if (AContentType == nullptr) {
                CString fileExt = ExtractFileExt(szBuffer, LResource.c_str());
                if (fileExt == LResource) {
                    fileExt = _T(".html");
                    LResource += fileExt;
                }
                AContentType = Mapping::ExtToType(fileExt.c_str());
            }

            if (!FileExists(LResource.c_str())) {
                CString NotFound;
                NotFound.Format("File not found: %s", LResource.c_str());

                CString errorLocation("/error/404");
                if (FileExists(CString(Root + errorLocation + ".html").c_str())) {
                    errorLocation << "?error=not_found";
                    errorLocation << "&error_description=" + base64_encode(Path);
                    Redirect(AConnection, errorLocation);
                } else {
                    AConnection->SendStockReply(CReply::not_found);
                }

                Log()->Error(APP_LOG_WARN, 0, NotFound.c_str());
                return;
            }

            auto lastModified = StrWebTime(FileAge(LResource.c_str()), szBuffer, sizeof(szBuffer));
            if (lastModified != nullptr)
                LReply->AddHeader(_T("Last-Modified"), lastModified);

            LReply->Content.LoadFromFile(LResource.c_str());
            AConnection->SendReply(CReply::ok, AContentType, SendNow);
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

            auto lastModified = StrWebTime(FileAge(LResource.c_str()), szBuffer, sizeof(szBuffer));
            if (lastModified != nullptr)
                LReply->AddHeader(_T("Last-Modified"), lastModified);

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
            if (LRequest->URI == _T("/quit"))
                GApplication->SignalProcess()->Quit();
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

        void CApostolModule::PQResultToJson(CPQResult *Result, CString &Json, bool IsArray) {
            IsArray = IsArray || Result->nTuples() > 1;

            if (Result->nTuples() == 0) {
                Json = IsArray ? _T("[]") : _T("{}");
                return;
            }

            if (IsArray)
                Json = _T("[");

            for (int Row = 0; Row < Result->nTuples(); ++Row) {
                if (!Result->GetIsNull(Row, 0)) {
                    if (Row > 0)
                        Json += _T(",");
                    Json += Result->GetValue(Row, 0);
                } else {
                    Json = IsArray ? _T("[]") : _T("{}");
                }
            }

            if (IsArray)
                Json += _T("]");
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
#endif

        void CApostolModule::Execute(CHTTPServerConnection *AConnection) {
            auto LServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            if (m_Roots.Count() == 0)
                InitRoots(LServer->Sites());
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

        void CApostolModule::Heartbeat() {

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
                auto LBinding = LConnection->Socket()->Binding();

                if (Assigned(LBinding)) {
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] "), LConnection, LBinding->PeerIP(),
                                 LBinding->PeerPort(), LBinding->Handle());
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
            bool Result = false;
            auto LConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);

            try {
                clock_t start = clock();

                Result = ExecuteModules(LConnection);

                Log()->Debug(0, _T("[Module] Runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
            } catch (Delphi::Exception::Exception &E) {
                DoServerException(LConnection, &E);
            }

            return Result;
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
                if (Module->IsEnabled())
                    DoInitialization(Module);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleManager::Finalization() {
            for (int i = 0; i < ModuleCount(); i++) {
                auto Module = Modules(i);
                if (Module->IsEnabled())
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
                if (Module->IsEnabled()) {
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
                if (Module->IsEnabled())
                    Module->Heartbeat();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CModuleManager::ExecuteModules(CTCPConnection *AConnection) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *> (AConnection);

            auto LRequest = LConnection->Request();

            const auto& UserAgent = LRequest->Headers.Values(_T("User-Agent"));

            int Index = 0;
            while (Index < ModuleCount() && !Modules(Index)->CheckUserAgent(UserAgent))
                Index++;

            if (Index == ModuleCount()) {
                LConnection->SendStockReply(CReply::forbidden);
                return false;
            }

            auto LModule = Modules(Index);

            DoBeforeExecuteModule(LModule);
            try {
                LModule->Execute(LConnection);
            } catch (...) {
                LConnection->SendStockReply(CReply::bad_request);
                DoAfterExecuteModule(LModule);
                throw;
            }
            DoAfterExecuteModule(LModule);

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}

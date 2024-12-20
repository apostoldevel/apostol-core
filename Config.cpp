/*++

Library name:

  apostol-core

Module Name:

  Config.cpp

Notices:

  Config

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Config.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Config {

        static unsigned long GConfigCount = 0;
        //--------------------------------------------------------------------------------------------------------------

        CConfig *GConfig = nullptr;
        //--------------------------------------------------------------------------------------------------------------

        inline void AddConfig() {
            if (GConfigCount == 0) {
                GConfig = CConfig::Create();
            }

            GConfigCount++;
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void RemoveConfig() {
            GConfigCount--;

            if (GConfigCount == 0) {
                GConfig->Destroy();
                GConfig = nullptr;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        inline char * GetCwd() {

            char  S[PATH_MAX];
            char *P = getcwd(S, PATH_MAX);

            if (P == nullptr) {
                throw EOSError(errno, _T("[emerg]: getcwd() failed: "));
            }

            return P;
        }
        //--------------------------------------------------------------------------------------------------------------

        inline char * GetHomeDir(uid_t uid) {
            struct passwd  *pwd;

            errno = 0;
            pwd = getpwuid(uid);
            if (pwd == nullptr) {
                throw EOSError(errno, "getpwuid(\"%d\") failed.", uid);
            }

            return pwd->pw_dir;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomConfig ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCustomConfig::CCustomConfig(): CObject() {
            m_pCommands = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CCustomConfig::~CCustomConfig() {
            CCustomConfig::Clear();
            delete m_pCommands;
        }
        //--------------------------------------------------------------------------------------------------------------

        CConfigCommand *CCustomConfig::Get(int Index) {
            if (Assigned(m_pCommands))
                return (CConfigCommand *) m_pCommands->Objects(Index);

            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomConfig::Put(int Index, CConfigCommand *ACommand) {
            if (!Assigned(m_pCommands))
                m_pCommands = new CStringList(true);

            m_pCommands->InsertObject(Index, ACommand->Ident(), ACommand);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CCustomConfig::GetCount() {
            if (Assigned(m_pCommands))
                return m_pCommands->Count();
            return 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomConfig::Clear() {
            if (Assigned(m_pCommands))
                m_pCommands->Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        int CCustomConfig::Add(CConfigCommand *ACommand) {
            int Index = GetCount();
            Put(Index, ACommand);
            return Index;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomConfig::Delete(int Index) {
            if (Assigned(m_pCommands))
                m_pCommands->Delete(Index);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CConfigComponent ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CConfigComponent::CConfigComponent() {
            AddConfig();
        }
        //--------------------------------------------------------------------------------------------------------------

        CConfigComponent::~CConfigComponent() {
            RemoveConfig();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CConfig ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CConfig::CConfig(CLog *ALog): CCustomConfig(), m_Flags{ false, false, false, false } {

            m_pIniFile = nullptr;

            m_pLog = ALog;
            m_uErrorCount = 0;

            m_nWorkers = 0;
            m_nProcessors = sysconf(_SC_NPROCESSORS_ONLN);

            m_nPort = 0;

            m_nTimeOut = INFINITE;
            m_nConnectTimeOut = 0;

            m_nLimitNoFile = static_cast<uint32_t>(-1);

            m_fMaster = false;
            m_fHelper = false;
            m_fDaemon = false;

            m_fPostgresConnect = false;
            m_fPostgresNotice = false;

            m_nPostgresPollMin = 5;
            m_nPostgresPollMax = 10;
        }
        //--------------------------------------------------------------------------------------------------------------

        CConfig::~CConfig() {
            delete m_pIniFile;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetUser(LPCTSTR AValue) {
            if (m_sUser != AValue) {
                if (AValue != nullptr) {
                    m_sUser = AValue;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetGroup(LPCTSTR AValue) {
            if (m_sGroup != AValue) {
                if (AValue != nullptr) {
                    m_sGroup = AValue;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetListen(LPCTSTR AValue) {
            if (m_sListen != AValue) {
                if (AValue != nullptr) {
                    m_sListen = AValue;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetPrefix(LPCTSTR AValue) {
            if (m_sPrefix != AValue) {

                if (AValue != nullptr) {
                    m_sPrefix = AValue;
                } else {
                    m_sPrefix = GetCwd();
                }

                if (m_sPrefix.front() == '~') {
                    CString S = m_sPrefix.SubString(1);
                    m_sPrefix = GetHomeDir(getuid());
                    m_sPrefix << S;
                }

                if (!path_separator(m_sPrefix.back())) {
                    m_sPrefix += '/';
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetConfPrefix(LPCTSTR AValue) {
            if (m_sConfPrefix != AValue) {

                if (AValue != nullptr)
                    m_sConfPrefix = AValue;
                else
                    m_sConfPrefix = m_sPrefix;

                if (m_sConfPrefix.empty()) {
                    m_sConfPrefix = GetCwd();
                }

                if (!path_separator(m_sConfPrefix.front())) {
                    m_sConfPrefix = m_sPrefix + m_sConfPrefix;
                }

                if (!path_separator(m_sConfPrefix.back())) {
                    m_sConfPrefix += '/';
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetCachePrefix(LPCTSTR AValue) {
            if (m_sCachePrefix != AValue) {

                if (AValue != nullptr)
                    m_sCachePrefix = AValue;
                else
                    m_sCachePrefix = m_sPrefix;

                if (m_sCachePrefix.empty()) {
                    m_sCachePrefix = GetCwd();
                }

                if (!path_separator(m_sCachePrefix.front())) {
                    m_sCachePrefix = m_sPrefix + m_sCachePrefix;
                }

                if (!path_separator(m_sCachePrefix.back())) {
                    m_sCachePrefix += '/';
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetConfFile(LPCTSTR AValue) {
            if (m_sConfFile != AValue) {
                m_sConfFile = AValue;
                if (!path_separator(m_sConfFile.front())) {
                    m_sConfFile = m_sPrefix + m_sConfFile;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetConfParam(LPCTSTR AValue) {
            if (m_sConfParam != AValue) {
                m_sConfParam = AValue;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetSignal(LPCTSTR AValue) {
            if (m_sSignal != AValue) {
                m_sSignal = AValue;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetPidFile(LPCTSTR AValue) {
            if (m_sPidFile != AValue) {
                m_sPidFile = AValue;
                if (!path_separator(m_sPidFile.front())) {
                    m_sPidFile = m_sPrefix + m_sPidFile;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetLockFile(LPCTSTR AValue) {
            if (m_sLockFile != AValue) {
                m_sLockFile = AValue;
                if (!path_separator(m_sLockFile.front())) {
                    m_sLockFile = m_sPrefix + m_sLockFile;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetErrorLog(LPCTSTR AValue) {
            if (m_sErrorLog != AValue) {
                m_sErrorLog = AValue;
                if (!path_separator(m_sErrorLog.front())) {
                    m_sErrorLog = m_sPrefix + m_sErrorLog;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetAccessLog(LPCTSTR AValue) {
            if (m_sAccessLog != AValue) {
                m_sAccessLog = AValue;
                if (!path_separator(m_sAccessLog.front())) {
                    m_sAccessLog = m_sPrefix + m_sAccessLog;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetPostgresLog(LPCTSTR AValue) {
            if (m_sPostgresLog != AValue) {
                m_sPostgresLog = AValue;
                if (!path_separator(m_sPostgresLog.front())) {
                    m_sPostgresLog = m_sPrefix + m_sPostgresLog;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetStreamLog(LPCTSTR AValue) {
            if (m_sStreamLog != AValue) {
                m_sStreamLog = AValue;
                if (!path_separator(m_sStreamLog.front())) {
                    m_sStreamLog = m_sPrefix + m_sStreamLog;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetDocRoot(LPCTSTR AValue) {
            if (m_sDocRoot != AValue) {
                if (AValue != nullptr)
                    m_sDocRoot = AValue;
                else
                    m_sDocRoot = m_sPrefix;

                if (m_sDocRoot.empty()) {
                    m_sDocRoot = GetCwd();
                }

                if (!path_separator(m_sDocRoot.front())) {
                    m_sDocRoot = m_sPrefix + m_sDocRoot;
                }

                if (!path_separator(m_sDocRoot.back())) {
                    m_sDocRoot += '/';
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetLocale(LPCTSTR AValue) {
            if (m_sLocale != AValue) {
                m_sLocale = AValue;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        uint32_t CConfig::GetWorkers() const {
            if (m_nWorkers == 0) {
                return m_nProcessors <= 0 ? 1 : m_nProcessors;
            }

            return m_nWorkers;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetWorkers(uint32_t AValue) {
            if (m_nWorkers != AValue) {
                m_nWorkers = AValue;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetDefault() {
            m_uErrorCount = 0;

            m_nPort = 4977;

            m_nTimeOut = 5000;
            m_nConnectTimeOut = 5;

            m_fMaster = true;
            m_fHelper = false;
            m_fDaemon = true;

            m_fPostgresConnect = false;
            m_fPostgresNotice = false;

            m_nPostgresPollMin = 5;
            m_nPostgresPollMax = 10;

            SetUser(m_sUser.empty() ? APP_DEFAULT_USER : m_sUser.c_str());
            SetGroup(m_sGroup.empty() ? APP_DEFAULT_GROUP : m_sGroup.c_str());

            SetListen(m_sListen.empty() ? APP_DEFAULT_LISTEN : m_sListen.c_str());

            SetPrefix(m_sPrefix.empty() ? APP_PREFIX : m_sPrefix.c_str());
            SetConfPrefix(m_sConfPrefix.empty() ? APP_CONF_PREFIX : m_sConfPrefix.c_str());
            SetCachePrefix(m_sCachePrefix.empty() ? APP_CACHE_PREFIX : m_sCachePrefix.c_str());
            SetConfFile(m_sConfFile.empty() ? APP_CONF_FILE : m_sConfFile.c_str());
            SetDocRoot(m_sDocRoot.empty() ? APP_DOC_ROOT : m_sDocRoot.c_str());

            SetLocale(m_sLocale.empty() ? APP_DEFAULT_LOCALE : m_sLocale.c_str());

            SetPidFile(m_sPidFile.empty() ? APP_PID_FILE : m_sPidFile.c_str());
            SetLockFile(m_sLockFile.empty() ? APP_LOCK_FILE : m_sLockFile.c_str());

            SetErrorLog(m_sErrorLog.empty() ? APP_ERROR_LOG_FILE : m_sErrorLog.c_str());
            SetAccessLog(m_sAccessLog.empty() ? APP_ACCESS_LOG_FILE : m_sAccessLog.c_str());
            SetPostgresLog(m_sPostgresLog.empty() ? APP_POSTGRES_LOG_FILE : m_sPostgresLog.c_str());
            SetStreamLog(m_sStreamLog.empty() ? APP_STREAM_LOG_FILE : m_sStreamLog.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::SetPostgresEnvironment(const CString &ConfName, CStringList &List) {
            const auto pg_database = getenv("PGDATABASE");
            char *pg_host = getenv("PGHOST");
            char *pg_hostaddr = getenv("PGHOSTADDR");
            char *pg_port = getenv("PGPORT");
            char *pg_user = getenv("PGUSER");
            char *pg_password = getenv("PGPASSWORD");
            char *pg_temp = nullptr;

            if (ConfName == "helper") {
                pg_temp = getenv("PGHOSTAPI");
                if (pg_temp != nullptr)
                    pg_host = pg_temp;

                pg_temp = getenv("PGHOSTADDR");
                if (pg_temp != nullptr)
                    pg_hostaddr = pg_temp;

                pg_temp = getenv("PGPORTAPI");
                if (pg_temp != nullptr)
                    pg_port = pg_temp;

                pg_temp = getenv("PGUSERAPI");
                if (pg_temp != nullptr)
                    pg_user = pg_temp;

                pg_temp = getenv("PGPASSWORDAPI");
                if (pg_temp != nullptr)
                    pg_password = pg_temp;
            } else if (ConfName == "kernel") {
                pg_temp = getenv("PGUSERKERNEL");
                if (pg_temp != nullptr)
                    pg_user = pg_temp;

                pg_temp = getenv("PGPASSWORDKERNEL");
                if (pg_temp != nullptr)
                    pg_password = pg_temp;
            }

            if (pg_host != nullptr) {
                List.Values("host", pg_host);
            }

            if (pg_hostaddr != nullptr) {
                List.Values("hostaddr", pg_hostaddr);
            }

            if (pg_port != nullptr) {
                List.Values("port", pg_port);
            }

            if (pg_database != nullptr) {
                List.Values("dbname", pg_database);
            }

            if (pg_user != nullptr) {
                List.Values("user", pg_user);
            }

            if (pg_password != nullptr) {
                List.Values("password", pg_password);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::DefaultCommands() {

            Clear();
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            Add(new CConfigCommand(_T("main"), _T("user"), m_sUser.c_str(), [this](auto && AValue) { SetUser(AValue); }));
            Add(new CConfigCommand(_T("main"), _T("group"), m_sGroup.c_str(), [this](auto && AValue) { SetGroup(AValue); }));

            Add(new CConfigCommand(_T("main"), _T("limitnofile"), &m_nLimitNoFile));

            if (m_nWorkers == 0) {
                Add(new CConfigCommand(_T("main"), _T("workers"), &m_nWorkers));
            }

            Add(new CConfigCommand(_T("main"), _T("master"), &m_fMaster));
            Add(new CConfigCommand(_T("main"), _T("helper"), &m_fHelper));
            Add(new CConfigCommand(_T("main"), _T("locale"), m_sLocale.c_str(), [this](auto && AValue) { SetLocale(AValue); }));

            Add(new CConfigCommand(_T("daemon"), _T("daemon"), &m_fDaemon));
            Add(new CConfigCommand(_T("daemon"), _T("pid"), m_sPidFile.c_str(), [this](auto && AValue) { SetPidFile(AValue); }));

            Add(new CConfigCommand(_T("server"), _T("listen"), m_sListen.c_str(), [this](auto && AValue) { SetListen(AValue); }));
            Add(new CConfigCommand(_T("server"), _T("port"), &m_nPort));
            Add(new CConfigCommand(_T("server"), _T("timeout"), &m_nTimeOut));
            Add(new CConfigCommand(_T("server"), _T("root"), m_sDocRoot.c_str(), [this](auto && AValue) { SetDocRoot(AValue); }));

            Add(new CConfigCommand(_T("cache"), _T("prefix"), m_sCachePrefix.c_str(), [this](auto && AValue) { SetCachePrefix(AValue); }));

            Add(new CConfigCommand(_T("log"), _T("error"), m_sErrorLog.c_str(), [this](auto && AValue) { SetErrorLog(AValue); }));
            Add(new CConfigCommand(_T("stream"), _T("log"), m_sStreamLog.c_str(), [this](auto && AValue) { SetStreamLog(AValue); }));
            Add(new CConfigCommand(_T("server"), _T("log"), m_sAccessLog.c_str(), [this](auto && AValue) { SetAccessLog(AValue); }));
            Add(new CConfigCommand(_T("postgres"), _T("log"), m_sPostgresLog.c_str(), [this](auto && AValue) { SetPostgresLog(AValue); }));

            Add(new CConfigCommand(_T("postgres"), _T("connect"), &m_fPostgresConnect));
            Add(new CConfigCommand(_T("postgres"), _T("notice"), &m_fPostgresNotice));
            Add(new CConfigCommand(_T("postgres"), _T("timeout"), &m_nConnectTimeOut));

            Add(new CConfigCommand(_T("postgres/poll"), _T("min"), &m_nPostgresPollMin));
            Add(new CConfigCommand(_T("postgres/poll"), _T("max"), &m_nPostgresPollMax));
#else
            Add(new CConfigCommand(_T("main"), _T("user"), m_sUser.c_str(), std::bind(&CConfig::SetUser, this, _1)));
            Add(new CConfigCommand(_T("main"), _T("group"), m_sGroup.c_str(), std::bind(&CConfig::SetGroup, this, _1)));

            Add(new CConfigCommand(_T("main"), _T("limitnofile"), &m_nLimitNoFile));

            if (m_nWorkers == 0) {
              Add(new CConfigCommand(_T("main"), _T("workers"), &m_nWorkers));
            }

            Add(new CConfigCommand(_T("main"), _T("master"), &m_fMaster));
            Add(new CConfigCommand(_T("main"), _T("helper"), &m_fHelper));
            Add(new CConfigCommand(_T("main"), _T("locale"), m_sLocale.c_str(), std::bind(&CConfig::SetLocale, this, _1)));

            Add(new CConfigCommand(_T("daemon"), _T("daemon"), &m_fDaemon));
            Add(new CConfigCommand(_T("daemon"), _T("pid"), m_sPidFile.c_str(), std::bind(&CConfig::SetPidFile, this, _1)));

            Add(new CConfigCommand(_T("server"), _T("listen"), m_sListen.c_str(), std::bind(&CConfig::SetListen, this, _1)));
            Add(new CConfigCommand(_T("server"), _T("port"), &m_nPort));
            Add(new CConfigCommand(_T("server"), _T("timeout"), &m_nTimeOut));
            Add(new CConfigCommand(_T("server"), _T("root"), m_sDocRoot.c_str(), std::bind(&CConfig::SetDocRoot, this, _1)));

            Add(new CConfigCommand(_T("cache"), _T("prefix"), m_sCachePrefix.c_str(), std::bind(&CConfig::SetCachePrefix, this, _1)));

            Add(new CConfigCommand(_T("log"), _T("error"), m_sErrorLog.c_str(), std::bind(&CConfig::SetErrorLog, this, _1)));
            Add(new CConfigCommand(_T("stream"), _T("log"), m_sStreamLog.c_str(), std::bind(&CConfig::SetStreamLog, this, _1)));
            Add(new CConfigCommand(_T("server"), _T("log"), m_sAccessLog.c_str(), std::bind(&CConfig::SetAccessLog, this, _1)));
            Add(new CConfigCommand(_T("postgres"), _T("log"), m_sPostgresLog.c_str(), std::bind(&CConfig::SetPostgresLog, this, _1)));

            Add(new CConfigCommand(_T("postgres"), _T("connect"), &m_fPostgresConnect));
            Add(new CConfigCommand(_T("postgres"), _T("notice"), &m_fPostgresNotice));
            Add(new CConfigCommand(_T("postgres"), _T("timeout"), &m_nConnectTimeOut));

            Add(new CConfigCommand(_T("postgres/poll"), _T("min"), &m_nPostgresPollMin));
            Add(new CConfigCommand(_T("postgres/poll"), _T("max"), &m_nPostgresPollMax));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        CIniFile *CConfig::ptrIniFile() const {
            if (m_pIniFile == nullptr)
                throw Delphi::Exception::Exception(_T("Not initialized"));
            return m_pIniFile;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CIniFile &CConfig::IniFile() const {
            return *ptrIniFile();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::LoadLogFilesDefault() {
            m_LogFiles.AddPair(_T("error"), m_sErrorLog.c_str());
            m_LogFiles.AddPair(_T("stream"), m_sStreamLog.c_str());
            m_LogFiles.AddPair(_T("postgres"), m_sPostgresLog.c_str());
        };
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::Parse() {

            if (GetCount() == 0)
                throw Delphi::Exception::Exception(_T("Command list is empty"));

            CVariant var;

            if (!FileExists(m_sConfFile.c_str())) {
                Log()->Error(APP_LOG_ERR, 0, APP_FILE_NOT_FOUND, m_sConfFile.c_str());
                return;
            }

            if (m_pIniFile == nullptr) {
                m_pIniFile = new CIniFile(m_sConfFile.c_str());
            } else {
                m_pIniFile->Rename(m_sConfFile.c_str(), true);
            }

#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            m_pIniFile->OnIniFileParseError([this](auto && Sender, auto && lpszSectionName, auto && lpszKeyName, auto && lpszValue, auto && lpszDefault, auto && Line) {
                OnIniFileParseError(Sender, lpszSectionName, lpszKeyName, lpszValue, lpszDefault, Line);
            });
#else
            m_pIniFile->OnIniFileParseError(std::bind(&CConfig::OnIniFileParseError, this, _1, _2, _3, _4, _5, _6));
#endif
            for (int i = 0; i < Count(); ++i) {
                const auto command = Commands(i);

                switch (command->Type()) {
                    case ctInteger:
                        var = m_pIniFile->ReadInteger(command->Section(), command->Ident(), command->Default().vasInteger);
                        break;

                    case ctUInteger:
                        var = ((uint32_t) m_pIniFile->ReadInteger(command->Section(), command->Ident(), (INT) command->Default().vasUnsigned));
                        break;

                    case ctDouble:
                        var = m_pIniFile->ReadFloat(command->Section(), command->Ident(), command->Default().vasDouble);
                        break;

                    case ctBoolean:
                        var = m_pIniFile->ReadBool(command->Section(), command->Ident(), command->Default().vasBoolean);
                        break;

                    case ctDateTime:
                        var = m_pIniFile->ReadDateTime(command->Section(), command->Ident(), command->Default().vasDouble);
                        break;

                    default:
                        var = new CString();
                        m_pIniFile->ReadString(command->Section(), command->Ident(), command->Default().vasStr, *var.vasCString);
                        break;
                }

                command->Value(var);
            }

            m_LogFiles.Clear();
            m_pIniFile->ReadSectionValues(_T("log"), &m_LogFiles);
            if ((m_LogFiles.Count() == 0) || !CheckLogFiles())
                LoadLogFilesDefault();

            m_PostgresConnInfo.Clear();

            m_PostgresConnInfo.AddPair("worker", CStringList());
            m_PostgresConnInfo.AddPair("helper", CStringList());
            m_PostgresConnInfo.AddPair("kernel", CStringList());

            auto &worker = m_PostgresConnInfo["worker"];
            auto &helper = m_PostgresConnInfo["helper"];
            auto &kernel = m_PostgresConnInfo["kernel"];

            m_pIniFile->ReadSectionValues(_T("postgres/worker"), &worker);
            m_pIniFile->ReadSectionValues(_T("postgres/helper"), &helper);
            m_pIniFile->ReadSectionValues(_T("postgres/kernel"), &kernel);

            SetPostgresEnvironment("worker", worker);
            SetPostgresEnvironment("helper", helper);
            SetPostgresEnvironment("kernel", kernel);

            if (worker.Count() == 0) {
                m_pIniFile->ReadSectionValues(_T("postgres/conninfo"), &worker);
                helper = worker;
            }

            if (worker.Count() == 0) {
                m_fPostgresConnect = false;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::Reload() {
            SetDefault();
            DefaultCommands();
            Parse();
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CConfig::CheckLogFiles() {
            u_int Level;

            for (int i = 0; i < m_LogFiles.Count(); ++i) {

                const CString &Key = m_LogFiles.Names(i);

                Level = GetLogLevelByName(Key.c_str());
                if (Level == 0) {
                    Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidKey _T(" - ignored and set by default"), _T("log"),
                                 Key.c_str(), m_sConfFile.c_str());
                    return false;
                }

                const CString &Value = m_LogFiles.Values(Key);
                if (!path_separator(m_LogFiles.Values(Key).front())) {
                    m_LogFiles[i] = Key + _T("=") + m_sPrefix + Value;
                }
            }

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CConfig::OnIniFileParseError(CCustomIniFile *Sender, LPCTSTR lpszSectionName, LPCTSTR lpszKeyName,
                LPCTSTR lpszValue, LPCTSTR lpszDefault, int Line) {

            const auto& LConfFile = Sender->FileName();

            m_uErrorCount++;
            if ((lpszValue == nullptr) || (lpszValue[0] == '\0')) {
                if (Flags().test_config || (lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                    Log()->Error(APP_LOG_EMERG, 0, ConfMsgEmpty, lpszSectionName, lpszKeyName, LConfFile.c_str(), Line);
                else
                    Log()->Error(APP_LOG_EMERG, 0, ConfMsgEmpty _T(" - ignored and set by default: \"%s\""), lpszSectionName,
                                lpszKeyName, LConfFile.c_str(), Line, lpszDefault);
            } else {
                if (Flags().test_config || (lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                    Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue, lpszSectionName, lpszKeyName, lpszValue,
                                LConfFile.c_str(), Line);
                else
                    Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue _T(" - ignored and set by default: \"%s\""), lpszSectionName, lpszKeyName, lpszValue,
                                LConfFile.c_str(), Line, lpszDefault);
            }
        }

    }
}
};
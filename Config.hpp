/*++

Library name:

  apostol-core

Module Name:

  Config.hpp

Notices:

  Config

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_CONFIG_HPP
#define APOSTOL_CONFIG_HPP

#define ConfMsgInvalidKey   _T("section \"%s\" invalid key \"%s\" in %s")
#define ConfMsgInvalidValue _T("section \"%s\" key \"%s\" invalid value \"%s\" in %s:%d")
#define ConfMsgEmpty        _T("section \"%s\" key \"%s\" value is empty in %s:%d")

extern "C++" {

namespace Apostol {

    namespace Config {

        class CConfig;
        class CConfigCommand;
        //--------------------------------------------------------------------------------------------------------------

        extern CConfig *GConfig;
        //--------------------------------------------------------------------------------------------------------------

        enum CCommandType { ctInteger, ctUInteger, ctDouble, ctBoolean, ctString, ctDateTime };
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (LPCTSTR lpValue)> COnConfigSetEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CConfigCommand: public CObject {
        private:

            CCommandType    m_Type;

            LPCTSTR         m_lpszSection;
            LPCTSTR         m_lpszIdent;

            CVariant        m_Default;
            CVariant        m_Value;

            Pointer         m_pPtr;
            COnConfigSetEvent m_OnSetString;

        protected:

            CCommandType GetType() { return m_Type; };

            LPCTSTR GetSection() { return m_lpszSection; };
            LPCTSTR GetIdent() { return m_lpszIdent; };

            const CVariant &GetDefault() { return m_Default; };

            void SetDefault(const CVariant &Default) {
                m_Default = Default;
            }

            const CVariant &GetValue() { return m_Value; };

            void SetValue(const CVariant &Value) {
                m_Value = Value;

                switch (m_Value.VType) {
                    case vtInteger:
                        *((int *) m_pPtr) = m_Value.vasInteger;
                        break;

                    case vtUnsigned:
                        *((uint32_t *) m_pPtr) = m_Value.vasUnsigned;
                        break;

                    case vtDouble:
                        *((double *) m_pPtr) = m_Value.vasDouble;
                        break;

                    case vtBoolean:
                        *((bool *) m_pPtr) = m_Value.vasBoolean;
                        break;

                    case vtString:
                        m_OnSetString(m_Value.vasCString->c_str());
                        delete m_Value.vasCString;
                        break;

                    default:
                        break;
                }

            };

            Pointer GetPtr() { return m_pPtr; };
            void SetPtr(Pointer Value) { m_pPtr = Value; };

        public:

            CConfigCommand(CCommandType Type, LPCTSTR Section, LPCTSTR Ident, const CVariant &Default, Pointer Ptr): CObject() {
                m_Type = Type;
                m_lpszSection = Section;
                m_lpszIdent = Ident;
                m_Default = Default;
                m_Value = Default;
                m_pPtr = Ptr;
            };

            CConfigCommand(LPCTSTR Section, LPCTSTR Ident, int *Value): CObject() {
                m_Type = ctInteger;
                m_lpszSection = Section;
                m_lpszIdent = Ident;
                m_Default = *Value;
                m_Value = *Value;
                m_pPtr = (Pointer) Value;
            };

            CConfigCommand(LPCTSTR Section, LPCTSTR Ident, uint32_t *Value): CObject() {
                m_Type = ctUInteger;
                m_lpszSection = Section;
                m_lpszIdent = Ident;
                m_Default = *Value;
                m_Value = *Value;
                m_pPtr = (Pointer) Value;
            };

            CConfigCommand(LPCTSTR Section, LPCTSTR Ident, const double *Value): CObject() {
                m_Type = ctDouble;
                m_lpszSection = Section;
                m_lpszIdent = Ident;
                m_Default = *Value;
                m_Value = *Value;
                m_pPtr = (Pointer) Value;
            };

            CConfigCommand(LPCTSTR Section, LPCTSTR Ident, bool *Value): CObject() {
                m_Type = ctBoolean;
                m_lpszSection = Section;
                m_lpszIdent = Ident;
                m_Default = *Value;
                m_Value = *Value;
                m_pPtr = (Pointer) Value;
            };

            CConfigCommand(LPCTSTR Section, LPCTSTR Ident, CDateTime *Value): CObject() {
                m_Type = ctDateTime;
                m_lpszSection = Section;
                m_lpszIdent = Ident;
                m_Default = *Value;
                m_Value = *Value;
                m_pPtr = (Pointer) Value;
            };

            CConfigCommand(LPCTSTR Section, LPCTSTR Ident, LPCTSTR Default, COnConfigSetEvent && OnSetEvent): CObject() {
                m_Type = ctString;
                m_lpszSection = Section;
                m_lpszIdent = Ident;
                m_Default = Default;
                m_Value = Default;
                m_pPtr = nullptr;
                m_OnSetString = OnSetEvent;
            };

            ~CConfigCommand() override = default;

            CCommandType Type() { return GetType(); };

            LPCTSTR Section() { return GetSection(); };
            LPCTSTR Ident() { return GetIdent(); };

            const CVariant &Default() { return GetDefault(); };
            const CVariant &Value() { return GetValue(); };

            void Default(const CVariant &Value) { SetDefault(Value); };
            void Value(const CVariant &Value) { SetValue(Value); };

            void OnSetEvent(COnConfigSetEvent && Value) { m_OnSetString = Value; };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomConfig ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CCustomConfig: public CObject {
        private:

            CStringList *m_pCommands;

        protected:

            virtual void SetDefault() abstract;

            virtual int GetCount();

            virtual CConfigCommand *Get(int Index);
            virtual void Put(int Index, CConfigCommand *ACommand);

            virtual void Parse() abstract;

        public:

            CCustomConfig();

            ~CCustomConfig() override;

            virtual void Clear();

            virtual int Add(CConfigCommand *ACommand);

            virtual void Delete(int Index);

            virtual int Count() { return GetCount(); };

            CStringList *CommandList() { return m_pCommands; };

            CConfigCommand *Commands(int Index) { return Get(Index); };
            void Commands(int Index, CConfigCommand *Value) { return Put(Index, Value); };

            CConfigCommand *operator[] (int Index) { return Commands(Index); }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CConfigComponent ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CConfigComponent {
        public:
            CConfigComponent();
            virtual ~CConfigComponent();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CConfig ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        struct CConfigFlags {
            bool show_help;
            bool show_version;
            bool show_configure;
            bool test_config;
        };
        //--------------------------------------------------------------------------------------------------------------

        class CConfig: public CCustomConfig {
        private:

            CIniFile *m_pIniFile;

            CLog *m_pLog;

            uint32_t m_uErrorCount;

            uint32_t m_nWorkers;

            uint32_t m_nPort;

            long int m_nProcessors;

            int m_nTimeOut;
            int m_nConnectTimeOut;

            uint32_t m_nLimitNoFile;

            bool m_fMaster;
            bool m_fHelper;
            bool m_fDaemon;

            bool m_fPostgresConnect;
            bool m_fPostgresNotice;

            uint32_t m_nPostgresPollMin;
            uint32_t m_nPostgresPollMax;

            CString m_sUser;
            CString m_sGroup;
            CString m_sListen;
            CString m_sPrefix;
            CString m_sConfPrefix;
            CString m_sCachePrefix;
            CString m_sConfFile;
            CString m_sConfParam;
            CString m_sSignal;
            CString m_sLocale;
            CString m_sPidFile;
            CString m_sLockFile;
            CString m_sDocRoot;

            CString m_sErrorLog;
            CString m_sAccessLog;
            CString m_sPostgresLog;
            CString m_sStreamLog;

            CStringList m_LogFiles;

            CStringListPairs m_PostgresConnInfo;

            CConfigFlags m_Flags;

            static void SetPostgresEnvironment(const CString &ConfName, CStringList &List);

        protected:

            void SetDefault() override;
            void DefaultCommands();

            void LoadLogFilesDefault();

            void SetUser(LPCTSTR AValue);
            void SetGroup(LPCTSTR AValue);
            void SetListen(LPCTSTR AValue);
            void SetPrefix(LPCTSTR AValue);
            void SetConfPrefix(LPCTSTR AValue);
            void SetConfFile(LPCTSTR AValue);
            void SetConfParam(LPCTSTR AValue);
            void SetSignal(LPCTSTR AValue);
            void SetLocale(LPCTSTR AValue);
            void SetPidFile(LPCTSTR AValue);
            void SetLockFile(LPCTSTR AValue);
            void SetDocRoot(LPCTSTR AValue);
            void SetCachePrefix(LPCTSTR AValue);

            void SetErrorLog(LPCTSTR AValue);
            void SetAccessLog(LPCTSTR AValue);
            void SetPostgresLog(LPCTSTR AValue);
            void SetStreamLog(LPCTSTR AValue);

            uint32_t GetWorkers() const;
            void SetWorkers(uint32_t AValue);

            bool CheckLogFiles();

            void OnIniFileParseError(CCustomIniFile *Sender, LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszValue,
                    LPCTSTR lpszDefault, int Line);

        public:

            explicit CConfig(CLog *ALog);

            ~CConfig() override;

            static class CConfig *Create() { return GConfig = new CConfig(GLog); };

            void Destroy() override { delete GConfig; };

            void Parse() override;

            void Reload();

            CLog *Log() { return m_pLog; };

            CConfigFlags &Flags() { return m_Flags; };
            const CConfigFlags &Flags() const { return m_Flags; };

            uint32_t ErrorCount() const { return m_uErrorCount; };

            uint32_t Workers() const { return GetWorkers(); };
            void Workers(uint32_t Value) { SetWorkers(Value); };

            bool Master() const { return m_fMaster; };
            bool Helper() const { return m_fHelper; };
            bool Daemon() const { return m_fDaemon; };

            uint32_t Port() const { return m_nPort; };

            int TimeOut() const { return m_nTimeOut; };
            int ConnectTimeOut() const { return m_nConnectTimeOut; };

            uint32_t LimitNoFile() const { return m_nLimitNoFile; };

            bool PostgresConnect() const { return m_fPostgresConnect; };
            bool PostgresNotice() const { return m_fPostgresNotice; };

            size_t PostgresPollMin() const { return (size_t) m_nPostgresPollMin; };

            size_t PostgresPollMax() const { return (size_t) m_nPostgresPollMax; };

            const CString& User() const { return m_sUser; };
            void User(const CString& AValue) { SetUser(AValue.c_str()); };
            void User(LPCTSTR AValue) { SetUser(AValue); };

            const CString& Group() const { return m_sGroup; };
            void Group(const CString& AValue) { SetGroup(AValue.c_str()); };
            void Group(LPCTSTR AValue) { SetGroup(AValue); };

            const CString& Listen() { return m_sListen; };
            void Listen(const CString& AValue) { SetListen(AValue.c_str()); };
            void Listen(LPCTSTR AValue) { SetListen(AValue); };

            const CString& Prefix() const { return m_sPrefix; };
            void Prefix(const CString& AValue) { SetPrefix(AValue.c_str()); };
            void Prefix(LPCTSTR AValue) { SetPrefix(AValue); };

            const CString& ConfPrefix() const { return m_sConfPrefix; };
            void ConfPrefix(const CString& AValue) { SetConfPrefix(AValue.c_str()); };
            void ConfPrefix(LPCTSTR AValue) { SetConfPrefix(AValue); };

            const CString& CachePrefix() const { return m_sCachePrefix; };
            void CachePrefix(const CString& AValue) { SetCachePrefix(AValue.c_str()); };
            void CachePrefix(LPCTSTR AValue) { SetCachePrefix(AValue); };

            const CString& ConfFile() const { return m_sConfFile; };
            void ConfFile(const CString& AValue) { SetConfFile(AValue.c_str()); };
            void ConfFile(LPCTSTR AValue) { SetConfFile(AValue); };

            const CString& ConfParam() const { return m_sConfParam; };
            void ConfParam(const CString& AValue) { SetConfParam(AValue.c_str()); };
            void ConfParam(LPCTSTR AValue) { SetConfParam(AValue); };

            const CString& Signal() const { return m_sSignal; };
            void Signal(const CString& AValue) { SetSignal(AValue.c_str()); };
            void Signal(LPCTSTR AValue) { SetSignal(AValue); };

            const CString& Locale() const { return m_sLocale; };
            void Locale(const CString& AValue) { SetLocale(AValue.c_str()); };
            void Locale(LPCTSTR AValue) { SetLocale(AValue); };

            const CString& PidFile() const { return m_sPidFile; };
            void PidFile(const CString& AValue) { SetPidFile(AValue.c_str()); };
            void PidFile(LPCTSTR AValue) { SetPidFile(AValue); };

            const CString& LockFile() const { return m_sLockFile; };
            void LockFile(const CString& AValue) { SetLockFile(AValue.c_str()); };
            void LockFile(LPCTSTR AValue) { SetLockFile(AValue); };

            const CString& ErrorLog() const { return m_sErrorLog; };
            void ErrorLog(const CString& AValue) { SetErrorLog(AValue.c_str()); };
            void ErrorLog(LPCTSTR AValue) { SetErrorLog(AValue); };

            const CString& AccessLog() const { return m_sAccessLog; };
            void AccessLog(const CString& AValue) { SetAccessLog(AValue.c_str()); };
            void AccessLog(LPCTSTR AValue) { SetAccessLog(AValue); };

            const CString& PostgresLog() const { return m_sPostgresLog; };
            void PostgresLog(const CString& AValue) { SetPostgresLog(AValue.c_str()); };
            void PostgresLog(LPCTSTR AValue) { SetPostgresLog(AValue); };

            const CString& StreamLog() const { return m_sStreamLog; };
            void StreamLog(const CString& AValue) { SetStreamLog(AValue.c_str()); };
            void StreamLog(LPCTSTR AValue) { SetStreamLog(AValue); };

            const CString& DocRoot() const { return m_sDocRoot; };
            void DocRoot(const CString& AValue) { SetDocRoot(AValue.c_str()); };
            void DocRoot(LPCTSTR AValue) { SetDocRoot(AValue); };

            CStringList& LogFiles() { return m_LogFiles; };
            const CStringList& LogFiles() const { return m_LogFiles; };

            CStringListPairs& PostgresConnInfo() { return m_PostgresConnInfo; };
            const CStringListPairs& PostgresConnInfo() const { return m_PostgresConnInfo; };

            CIniFile *ptrIniFile() const;
            const CIniFile& IniFile() const;

        };

    }
}

using namespace Apostol::Config;
}
#endif
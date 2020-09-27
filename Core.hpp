/*++

Library name:

  apostol-core

Module Name:

  Core.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_CORE_HPP
#define APOSTOL_CORE_HPP
//----------------------------------------------------------------------------------------------------------------------

#include <wait.h>
#include <execinfo.h>
//----------------------------------------------------------------------------------------------------------------------

#include "delphi.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include "Log.hpp"
#include "Config.hpp"
//----------------------------------------------------------------------------------------------------------------------

#define APP_FILE_NOT_FOUND _T("File not found: %s")
//----------------------------------------------------------------------------------------------------------------------

typedef std::function<void (const CIniFile &IniFile, const CString &Section, CStringList &Config)> COnInitConfigEvent;
//----------------------------------------------------------------------------------------------------------------------

class CGlobalComponent: public CLogComponent, public CConfigComponent {
public:

    CGlobalComponent(): CLogComponent(), CConfigComponent() {

    };

    static void OnIniFileParseError (CCustomIniFile *Sender, LPCTSTR lpszSectionName, LPCTSTR lpszKeyName,
                                     LPCTSTR lpszValue, LPCTSTR lpszDefault, int Line) {

        const auto& LConfFile = Sender->FileName();

        if ((lpszValue == nullptr) || (lpszValue[0] == '\0')) {
            if ((lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                Log()->Error(APP_LOG_EMERG, 0, ConfMsgEmpty, lpszSectionName, lpszKeyName, LConfFile.c_str(), Line);
        } else {
            if ((lpszDefault == nullptr) || (lpszDefault[0] == '\0'))
                Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue, lpszSectionName, lpszKeyName, lpszValue,
                             LConfFile.c_str(), Line);
            else
                Log()->Error(APP_LOG_EMERG, 0, ConfMsgInvalidValue _T(" - ignored and set by default: \"%s\""), lpszSectionName, lpszKeyName, lpszValue,
                             LConfFile.c_str(), Line, lpszDefault);
        }
    };

    static void LoadConfig(const CString &FileName, TPairs<CStringList> &Profiles, COnInitConfigEvent && OnInitConfig) {

        const CString Prefix(Config()->Prefix());
        CString ConfigFile(FileName);

        if (!path_separator(ConfigFile.front())) {
            ConfigFile = Prefix + ConfigFile;
        }

        Profiles.Clear();

        if (FileExists(ConfigFile.c_str())) {
            CIniFile IniFile(ConfigFile.c_str());
            IniFile.OnIniFileParseError(OnIniFileParseError);

            CStringList Sections;
            IniFile.ReadSections(&Sections);

            for (int i = 0; i < Sections.Count(); i++) {
                const auto& Section = Sections[i];
                int Index = Profiles.AddPair(Section, CStringList());
                auto& Config = Profiles[Index].Value();
                OnInitConfig(IniFile, Section, Config);
            }

            auto& Default = Profiles.Default();
            if (Default.Name().IsEmpty()) {
                Default.Name() = _T("default");
                OnInitConfig(IniFile, Default.Name(), Default.Value());
            }
        } else {
            Log()->Error(APP_LOG_EMERG, 0, APP_FILE_NOT_FOUND, ConfigFile.c_str());
        }
    }
    //--------------------------------------------------------------------------------------------------------------

    static CConfig *Config() { return GConfig; };

    static CLog *Log(){ return GLog; };

};
//----------------------------------------------------------------------------------------------------------------------

#include "Process.hpp"
#include "Server.hpp"
#include "Module.hpp"
#include "Modules.hpp"
#include "Application.hpp"
#include "Processes.hpp"
//----------------------------------------------------------------------------------------------------------------------

#endif //APOSTOL_CORE_HPP

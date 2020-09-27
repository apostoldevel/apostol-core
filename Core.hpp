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

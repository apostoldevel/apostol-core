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
    //------------------------------------------------------------------------------------------------------------------

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
    //------------------------------------------------------------------------------------------------------------------

    static void LoadConfig(const CString &FileName, CStringListPairs &Profiles, COnInitConfigEvent && OnInitConfig) {

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
            Log()->Error(APP_LOG_WARN, 0, APP_FILE_NOT_FOUND, ConfigFile.c_str());
        }
    }
    //------------------------------------------------------------------------------------------------------------------

    static void WSDebug(CWebSocket *AData) {
#ifdef _DEBUG
        CString Hex;
        CMemoryStream Stream;

        TCHAR szZero[1] = { 0x00 };

        size_t delta = 0;
        size_t size = AData->Payload().Size();

        Stream.SetSize((ssize_t) size);
        Stream.Write(AData->Payload().Memory(), size);

        Hex.SetLength(size * 3 + 1);
        ByteToHexStr((LPSTR) Hex.Data(), Hex.Size(), (LPCBYTE) Stream.Memory(), size, 32);

        if (size > MaxFormatStringLength) {
            delta = size - MaxFormatStringLength;
            size = MaxFormatStringLength;
        }

        DebugMessage("[FIN: %#x; OP: %#x; MASK: %#x LEN: %d] [%d-%d=%d] [%d] [%d]\n",
                     AData->Frame().FIN, AData->Frame().Opcode, AData->Frame().Mask, AData->Frame().Length,
                     AData->Payload().Size(), AData->Payload().Position(), AData->Payload().Size() - AData->Payload().Position(), delta, size
        );

        if (Stream.Size() != 0) {
            Stream.Write(&szZero, sizeof(szZero));
            DebugMessage("HEX: %s", Hex.c_str());
            DebugMessage("\nSTR: %s\n", (LPCTSTR) Stream.Memory() + delta);
        }
#endif
    }
    //------------------------------------------------------------------------------------------------------------------

    static void WSDebugConnection(CWebSocketConnection *AConnection) {
#ifdef _DEBUG
        if (AConnection != nullptr) {

            auto pSocket = AConnection->Socket();
            if (pSocket != nullptr) {
                auto pHandle = pSocket->Binding();
                if (pHandle != nullptr) {
                    DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnRequest] "), AConnection,
                                 pHandle->PeerIP(), pHandle->PeerPort(), pHandle->Handle());
                }
            }

            WSDebug(AConnection->WSRequest());

            static auto OnRequest = [](CObject *Sender) {
                auto pConnection = dynamic_cast<CWebSocketConnection *> (Sender);
                if (pConnection != nullptr) {
                    auto pSocket = pConnection->Socket();
                    if (pSocket != nullptr) {
                        auto pHandle = pSocket->Binding();
                        if (pHandle != nullptr) {
                            DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnRequest] "), pConnection,
                                         pHandle->PeerIP(), pHandle->PeerPort(), pHandle->Handle());
                        }
                    }

                    WSDebug(pConnection->WSRequest());
                }
            };

            static auto OnWaitRequest = [](CObject *Sender) {
                auto pConnection = dynamic_cast<CWebSocketConnection *> (Sender);
                if (pConnection != nullptr) {
                    auto pSocket = pConnection->Socket();
                    if (pSocket != nullptr) {
                        auto pHandle = pSocket->Binding();
                        if (pHandle != nullptr) {
                            DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnWaitRequest] "), pConnection,
                                         pHandle->PeerIP(), pHandle->PeerPort(), pHandle->Handle());
                        }
                    }

                    WSDebug(pConnection->WSRequest());
                }
            };

            static auto OnReply = [](CObject *Sender) {
                auto pConnection = dynamic_cast<CWebSocketConnection *> (Sender);
                if (pConnection != nullptr) {
                    auto pSocket = pConnection->Socket();
                    if (pSocket != nullptr) {
                        auto pHandle = pSocket->Binding();
                        if (pHandle != nullptr) {
                            DebugMessage(_T("\n[%p] [%s:%d] [%d] [WebSocket] [OnReply] "), pConnection,
                                         pHandle->PeerIP(), pHandle->PeerPort(), pHandle->Handle());
                        }
                    }

                    WSDebug(pConnection->WSReply());
                }
            };

            AConnection->OnWaitRequest(OnWaitRequest);
            AConnection->OnRequest(OnRequest);
            AConnection->OnReply(OnReply);
        }
#endif
    }
    //------------------------------------------------------------------------------------------------------------------

    static void DebugRequest(CHTTPRequest *ARequest) {
#ifdef _DEBUG
        DebugMessage(_T("[%p] Request:\n%s %s HTTP/%d.%d\n"), ARequest, ARequest->Method.c_str(), ARequest->URI.c_str(), ARequest->VMajor, ARequest->VMinor);

        for (int i = 0; i < ARequest->Headers.Count(); i++)
            DebugMessage(_T("%s: %s\n"), ARequest->Headers[i].Name().c_str(), ARequest->Headers[i].Value().c_str());

        if (!ARequest->Content.IsEmpty())
            DebugMessage(_T("\n%s\n"), ARequest->Content.c_str());
#endif
    }
    //------------------------------------------------------------------------------------------------------------------

    static void DebugReply(CHTTPReply *AReply) {
#ifdef _DEBUG
        if (!AReply->StatusText.IsEmpty()) {
            DebugMessage(_T("[%p] Reply:\nHTTP/%d.%d %d %s\n"), AReply, AReply->VMajor, AReply->VMinor, AReply->Status, AReply->StatusText.c_str());

            for (int i = 0; i < AReply->Headers.Count(); i++)
                DebugMessage(_T("%s: %s\n"), AReply->Headers[i].Name().c_str(), AReply->Headers[i].Value().c_str());

            if (!AReply->Content.IsEmpty())
                DebugMessage(_T("\n%s\n"), AReply->Content.c_str());
        }
#endif
    }
    //------------------------------------------------------------------------------------------------------------------

    static void DebugConnection(CHTTPServerConnection *AConnection) {
#ifdef _DEBUG
        if (AConnection != nullptr) {
            auto pSocket = AConnection->Socket();
            if (pSocket != nullptr) {
                auto pHandle = pSocket->Binding();
                if (pHandle != nullptr) {
                    if (pHandle->HandleAllocated()) {
                        DebugMessage(_T("\n[%p] [%s:%d] [%d] "), AConnection,
                                     pHandle->PeerIP(), pHandle->PeerPort(), pHandle->Handle());
                    }
                }
            }

            DebugRequest(AConnection->Request());

            static auto OnReply = [](CObject *Sender) {

                auto pConnection = dynamic_cast<CHTTPServerConnection *> (Sender);

                if (pConnection != nullptr) {
                    auto pSocket = pConnection->Socket();
                    if (pSocket != nullptr) {
                        auto pHandle = pSocket->Binding();
                        if (pHandle != nullptr) {
                            if (pHandle->HandleAllocated()) {
                                DebugMessage(_T("\n[%p] [%s:%d] [%d] "), pConnection, pHandle->PeerIP(),
                                             pHandle->PeerPort(), pHandle->Handle());
                            }
                        }
                    }

                    DebugReply(pConnection->Reply());
                }
            };

            AConnection->OnReply(OnReply);
        }
#endif
    }
    //------------------------------------------------------------------------------------------------------------------
#ifdef WITH_POSTGRESQL
    static void DebugNotify(CPQConnection *AConnection, PGnotify *ANotify) {
#ifdef _DEBUG
        const auto& conInfo = AConnection->ConnInfo();

        TCHAR szTimeStamp[25] = {0};
        DateTimeToStr(Now(), szTimeStamp, sizeof(szTimeStamp));

        DebugMessage("[%s] [%d#0] [NOTIFY] [%d] [%d] [postgresql://%s@%s:%s/%s] [PID: %d] [%s] %s\n",
                     szTimeStamp, MainThreadID, AConnection->PID(), AConnection->Socket(),
                     conInfo["user"].c_str(), conInfo["host"].c_str(), conInfo["port"].c_str(), conInfo["dbname"].c_str(),
                     ANotify->be_pid, ANotify->relname, ANotify->extra);
#endif
    }
    //------------------------------------------------------------------------------------------------------------------
#endif
    static CConfig *Config() { return GConfig; };

    static CLog *Log(){ return GLog; };

};
//----------------------------------------------------------------------------------------------------------------------

#include "Process.hpp"
#include "Server.hpp"
#include "Token.hpp"
#include "Module.hpp"
#include "Modules.hpp"
#include "Application.hpp"
#include "Processes.hpp"
//----------------------------------------------------------------------------------------------------------------------

#endif //APOSTOL_CORE_HPP

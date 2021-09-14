/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it and/or modify it under version 2 of the License, or (at your option), any later version.
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "ScriptMgr.h"
#include "AccountMgr.h"
#include "Chat.h"
#include "Player.h"
#include "Anticheat/Anticheat.hpp"
#include "Anticheat/module/Antispam/antispammgr.hpp"
#include "Anticheat/module/Antispam/antispam.hpp"
#include "Anticheat/module/libanticheat.hpp"
#include "Anticheat/module/config.hpp"

class anticheat_commandscript : public CommandScript
{
public:
    anticheat_commandscript() : CommandScript("anticheat_commandscript") {}

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> anticheatFingerprintCommandTable =
        {
            {"list",        SEC_ADMINISTRATOR,  false, &HandleAnticheatFingerprintListCommand,      ""},
            {"history",     SEC_ADMINISTRATOR,  false, &HandleAnticheatFingerprintHistoryCommand,   ""},
            {"ahistory",    SEC_ADMINISTRATOR,  false, &HandleAnticheatFingerprintAHistoryCommand,  ""},
            {nullptr,       0,                  false, nullptr,                                     ""},
        };

        static std::vector<ChatCommand> anticheatCommandTable =
        {
            {"info",        SEC_GAMEMASTER,     true,   &HandleAnticheatInfoCommand,        ""},
            {"enable",      SEC_ADMINISTRATOR,  true,   &HandleAnticheatEnableCommand,      ""},
            {"silence",     SEC_GAMEMASTER,     false,  &HandleAnticheatSilenceCommand,     ""},
            {"unsilence",   SEC_GAMEMASTER,     false,  &HandleAnticheatUnsilenceCommand,   ""},
            {"spaminfo",    SEC_ADMINISTRATOR,  false,  &HandleAnticheatSpaminfoCommand,    ""},
            {"fingerprint", SEC_ADMINISTRATOR,  false,  nullptr,                            "", anticheatFingerprintCommandTable},
            {"cheatinform", SEC_GAMEMASTER,     false,  &HandleAnticheatCheatinformCommand, ""},
            {"spaminform",  SEC_GAMEMASTER,     false,  &HandleAnticheatSpaminformCommand,  ""},
            {"blacklist",   SEC_GAMEMASTER,     false,  &HandleAnticheatBlacklistCommand,   ""},
            {"debugextrap", SEC_ADMINISTRATOR,  true,   &HandleAnticheatDebugExtrapCommand, ""},
            {nullptr,       0,                  false,  nullptr,                            ""},
        };

        static std::vector<ChatCommand> commandTable = {{"account", SEC_PLAYER, true, nullptr, "", anticheatCommandTable}};

        return commandTable;
    }

    static bool HandleAnticheatInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* target = nullptr;

        if (!handler->extractPlayerTarget((char*) args, &target))
        {
            return false;
        }

        if (auto const anticheat = dynamic_cast<const NamreebAnticheat::SessionAnticheat*>(target->GetSession()->GetAnticheat()))
        {
            handler->PSendSysMessage("Anticheat info for %s", target->GetGUID().ToString().c_str());
            anticheat->SendCheatInfo(handler);
        }
        else
        {
            handler->PSendSysMessage("No anticheat session for %s, they may be a bot or anticheat is disabled", target->GetGUID().ToString().c_str());
        }
        return true;
    }

    static bool HandleAnticheatEnableCommand(ChatHandler* handler, char const* args)
    {
        handler->PSendSysMessage("Anticheat Late March version");
        return true;
    }

    static bool HandleAnticheatSilenceCommand(ChatHandler* handler, char const* args)
    {
        char* accId = strtok((char*) args, " ");
        if (!accId)
            return false;

        int32 AccountId = atoi(accId);
        if (!AccountId)
            return false;

        sAntispamMgr->Silence(AccountId);

        handler->PSendSysMessage("Silenced account %u", AccountId);
        return true;
    }

    static bool HandleAnticheatSpaminfoCommand(ChatHandler* handler, char const* args)
    {
        Player*    target = nullptr;
        ObjectGuid playerGuid;

        if (!handler->extractPlayerTarget((char*) args, &target, &playerGuid))
            return false;

        std::shared_ptr<NamreebAnticheat::Antispam> antispam;

        if (target)
            if (auto const anticheat = dynamic_cast<const NamreebAnticheat::SessionAnticheat*>(target->GetSession()->GetAnticheat()))
                antispam = anticheat->GetAntispam();

        if (!antispam)
        {
            // if we reach here, lookup cached information instead
            auto const accountId = sObjectMgr->GetPlayerAccountIdByGUID(playerGuid.GetCounter());

            antispam = sAntispamMgr->CheckCache(accountId);
        }

        if (!antispam)
            handler->SendSysMessage("No antispam info available");
        else
            handler->SendSysMessage(antispam->GetInfo().c_str());

        return true;
    }

    static bool HandleAnticheatFingerprintListCommand(ChatHandler* handler, char const* args)
    {
        //uint32 fingerprintNum = 0;

        //if (!handler->extract((char*) args, fingerprintNum, 16))
        //{
        //    return false;
        //}

        //// search all session with specified fingerprint
        //sWorld.GetMessager().AddMessage([session = GetSession(), fingerprintNum](World* world) {
        //    int32 count = 0;

        //    std::stringstream str;

        //    world->ExecuteForAllSessions([&](auto& data) {
        //        const WorldSession&                       sess      = data;
        //        const NamreebAnticheat::SessionAnticheat* anticheat = dynamic_cast<const NamreebAnticheat::SessionAnticheat*>(sess.GetAnticheat());

        //        if (!anticheat)
        //            return;

        //        if (anticheat->GetFingerprint() == fingerprintNum)
        //        {
        //            str << "Account: " << sess.GetAccountName() << " ID: " << sess.GetAccountId() << " IP: " << sess.GetRemoteAddress();

        //            if (auto const player = sess.GetPlayer())
        //                str << " Player name: " << player->GetName();

        //            str << "\n";

        //            ++count;
        //        }
        //    });

        //    ChatHandler(session).PSendSysMessage("%s\nEnd of listing for fingerprint 0x%x.  Found %d matches.", str.str().data(), fingerprintNum, count);
        //});
        return true;
    }

    static bool HandleAnticheatFingerprintHistoryCommand(ChatHandler* handler, char const* args)
    {
        char* fprNum = strtok((char*) args, " ");
        if (!fprNum)
            return false;

        int32 fingerprintNum = atoi(fprNum);
        if (!fingerprintNum)
            return false;

        handler->PSendSysMessage("Listing history for fingerprint 0x%x.  Maximum history length from config: %u", fingerprintNum, sAnticheatConfig->GetFingerprintHistory());

        QueryResult result(LoginDatabase.PQuery("SELECT account, ip, realm, time FROM system_fingerprint_usage WHERE fingerprint = %u ORDER BY `time` DESC", fingerprintNum));

        int count = 0;
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();

                uint32      accountId = fields[0].GetUInt32();
                std::string ip        = fields[1].GetString();
                uint32      realm     = fields[2].GetUInt32();
                std::string time      = fields[3].GetString();

                handler->PSendSysMessage("Account ID: %u IP: %s Realm: %u Time: %s", accountId, ip.c_str(), realm, time.c_str());

                ++count;
            } while (result->NextRow());
        }

        handler->PSendSysMessage("End of history for fingerprint 0x%x.  Found %d matches", fingerprintNum, count);
        return true;
    }

    static bool HandleAnticheatFingerprintAHistoryCommand(ChatHandler* handler, char const* args)
    {
        char* accId = strtok((char*) args, " ");
        if (!accId)
            return false;

        int32 AccountId = atoi(accId);
        if (!AccountId)
            return false;

        handler->PSendSysMessage("Listing history for account %u.  Maximum length: %u", AccountId, sAnticheatConfig->GetFingerprintHistory());

        QueryResult result(LoginDatabase.PQuery("SELECT fingerprint, ip, realm, time FROM system_fingerprint_usage WHERE account = %u ORDER BY `time` DESC LIMIT %u", AccountId, sAnticheatConfig->GetFingerprintHistory()));

        int count = 0;
        if (result)
        {
            do
            {
                const Field* fields = result->Fetch();

                uint32      fingerprint = fields[0].GetUInt32();
                std::string ip          = fields[1].GetString();
                uint32      realm       = fields[2].GetUInt32();
                std::string time        = fields[3].GetString();

                handler->PSendSysMessage("Fingerprint: 0x%x IP: %s Realm: %u Time: %s", fingerprint, ip.c_str(), realm, time.c_str());

                ++count;
            } while (result->NextRow());
        }

        handler->PSendSysMessage("End of history for account %u.  Found %d matches", AccountId, count);
        return true;
    }

    static bool HandleAnticheatCheatinformCommand(ChatHandler* handler, char const* args)
    {
        // XXX - ACCOUNT FLAGS
        //WorldSession* session = handler->GetSession();

        //if (!session->HasAccountFlag(ACCOUNT_FLAG_SHOW_ANTICHEAT))
        //{
        //    session->AddAccountFlag(ACCOUNT_FLAG_SHOW_ANTICHEAT);
        //    LoginDatabase.PExecute("UPDATE account SET flags = flags | 0x%x WHERE id = %u", session->GetAccountId(), ACCOUNT_FLAG_SHOW_ANTICHEAT);

        //    handler->SendSysMessage("Anticheat messages will be shown");
        //}
        //else
        //{
        //    session->RemoveAccountFlag(ACCOUNT_FLAG_SHOW_ANTICHEAT);
        //    LoginDatabase.PExecute("UPDATE account SET flags = flags & ~0x%x WHERE id = %u", session->GetAccountId(), ACCOUNT_FLAG_SHOW_ANTICHEAT);

        //    handler->SendSysMessage("Anticheat messages will be hidden");
        //}
        return true;
    }

    static bool HandleAnticheatSpaminformCommand(ChatHandler* handler, char const* args)
    {
        // ACCOUNT FLAGS
        //WorldSession* session = handler->GetSession();

        //if (!session->HasAccountFlag(ACCOUNT_FLAG_SHOW_ANTISPAM))
        //{
        //    session->AddAccountFlag(ACCOUNT_FLAG_SHOW_ANTISPAM);
        //    LoginDatabase.PExecute("UPDATE account SET flags = flags | 0x%x WHERE id = %u", session->GetAccountId(), ACCOUNT_FLAG_SHOW_ANTISPAM);

        //    handler->SendSysMessage("Antispam messages will be shown");
        //}
        //else
        //{
        //    session->RemoveAccountFlag(ACCOUNT_FLAG_SHOW_ANTISPAM);
        //    LoginDatabase.PExecute("UPDATE account SET flags = flags & ~0x%x WHERE id = %u", session->GetAccountId(), ACCOUNT_FLAG_SHOW_ANTISPAM);

        //    handler->SendSysMessage("Antispam messages will be hidden");
        //}
        return true;
    }

    static bool HandleAnticheatBlacklistCommand(ChatHandler* handler, char const* args)
    {
        sAntispamMgr->BlacklistAdd(args);

        handler->SendSysMessage("Blacklist add submitted");
        return true;
    }

    static bool HandleAnticheatUnsilenceCommand(ChatHandler* handler, char const* args)
    {
        char* accId = strtok((char*) args, " ");
        if (!accId)
            return false;

        int32 AccountId = atoi(accId);
        if (!AccountId)
            return false;

        sAntispamMgr->Unsilence(AccountId);

        handler->PSendSysMessage("Unsilenced account %u", AccountId);
        return true;
    }

    static bool HandleAnticheatDebugExtrapCommand(ChatHandler* handler, char const* args)
    {
        char* accId = strtok((char*) args, " ");
        if (!accId)
            return false;

        int32 seconds = atoi(accId);
        if (!seconds)
            return false;

        if (seconds > HOUR)
        {
            handler->SendSysMessage("Do not enable this for more than an hour");
            return false;
        }

        auto const anticheat = dynamic_cast<NamreebAnticheat::AnticheatLib*>(GetAnticheatLib());
        if (!anticheat)
        {
            handler->SendSysMessage("No anticheat lib present");
            return false;
        }

        anticheat->EnableExtrapolationDebug(seconds);

        handler->PSendSysMessage("Extrapolation debug enabled for %u seconds", seconds);

        return true;
    };
};

void AddSC_anticheat_commandscript()
{
    new anticheat_commandscript();
}

/*
 * CoA Universal Content Scaling
 * CoAContentScalingCommands: Diagnostic and administration commands (.coascale).
 */

#include "AdaptiveEncounterAPI.h"
#include "Chat.h"
#include "CoAContentScaling.h"
#include "CombatBudgetProfile.h"
#include "CommandScript.h"
#include "ContentPackRegistry.h"
#include "Creature.h"
#include "InstanceScaleContext.h"
#include "ItemBudgetScaler.h"
#include "ItemTemplate.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ProgressionLayout.h"
#include "QuestDef.h"
#include <string>

using namespace Acore::ChatCommands;

class coa_content_scaling_commandscript : public CommandScript
{
public:
    coa_content_scaling_commandscript() : CommandScript("coa_content_scaling_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable const coaScaleCommandTable =
        {
            { "status",   HandleStatus,   SEC_ADMINISTRATOR, Console::Yes },
            { "layout",   HandleLayout,   SEC_ADMINISTRATOR, Console::Yes },
            { "creature", HandleCreature, SEC_ADMINISTRATOR, Console::No },
            { "instance", HandleInstance, SEC_ADMINISTRATOR, Console::No },
            { "quest",    HandleQuest,    SEC_ADMINISTRATOR, Console::Yes },
            { "item",     HandleItem,     SEC_ADMINISTRATOR, Console::Yes },
            { "validate", HandleValidate, SEC_ADMINISTRATOR, Console::Yes },
        };

        static ChatCommandTable const commandTable =
        {
            { "coascale", coaScaleCommandTable },
        };
        return commandTable;
    }

private:
    static bool HandleStatus(ChatHandler* handler)
    {
        ProgressionLayout const& layout = sCoAContentScaling->GetLayout();
        handler->PSendSysMessage("=== CoA Universal Content Scaling Status ===");
        handler->PSendSysMessage("Enabled: %s", sCoAContentScaling->IsEnabled() ? "Yes" : "No");
        handler->PSendSysMessage("MaxPlayerLevel: %u", uint32(layout.maxLevel));
        handler->PSendSysMessage("Active Packs: Classic%s%s",
            layout.tbcEnabled ? ", TBC" : "", layout.wotlkEnabled ? ", WotLK" : "");
        handler->PSendSysMessage("Group Scaling: %s", sCoAContentScaling->IsGroupScalingEnabled() ? "Enabled" : "Disabled");
        handler->PSendSysMessage("Adaptive Mechanics: %s", sCoAContentScaling->IsAdaptiveMechanicsEnabled() ? "Enabled" : "Disabled");
        return true;
    }

    static bool HandleLayout(ChatHandler* handler)
    {
        ProgressionLayout const& layout = sCoAContentScaling->GetLayout();
        handler->PSendSysMessage("=== CoA Progression Layout ===");
        handler->PSendSysMessage("MaxPlayerLevel: %u", uint32(layout.maxLevel));
        handler->PSendSysMessage("Classic: %u -> %u", uint32(layout.classic.minLevel), uint32(layout.classic.maxLevel));
        if (layout.tbc.has_value())
            handler->PSendSysMessage("TBC:     %u -> %u", uint32(layout.tbc->minLevel), uint32(layout.tbc->maxLevel));
        else
            handler->PSendSysMessage("TBC:     [Disabled / Locked]");

        if (layout.wotlk.has_value())
            handler->PSendSysMessage("WotLK:   %u -> %u", uint32(layout.wotlk->minLevel), uint32(layout.wotlk->maxLevel));
        else
            handler->PSendSysMessage("WotLK:   [Disabled / Locked]");

        return true;
    }

    static bool HandleCreature(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        Creature* target = handler->getSelectedCreature();
        if (!target)
        {
            handler->SendSysMessage("No creature selected.");
            return true;
        }

        CreatureTemplate const* cinfo = target->GetCreatureTemplate();
        if (!cinfo)
            return true;

        CreatureScaleContext const ctx = sCombatBudgetProfile->BuildContext(cinfo, target, target->GetLevel());
        CalculatedCombatBudget const budget = sCombatBudgetProfile->CalculateBudget(cinfo, ctx);

        handler->PSendSysMessage("=== Creature Scaling: %s (Entry: %u) ===", cinfo->Name.c_str(), cinfo->Entry);
        handler->PSendSysMessage("Authored Level: %u | Effective Level: %u", uint32(ctx.authoredLevel), uint32(ctx.effectiveLevel));
        handler->PSendSysMessage("Era: %s | Tier: %s", ContentEraToString(ctx.era).data(), ContentTierToString(ctx.tier).data());
        handler->PSendSysMessage("Authored HealthMod: %.2f | DamageMod: %.2f | Expansion: %u",
            cinfo->ModHealth, cinfo->DamageModifier, uint32(cinfo->expansion));
        handler->PSendSysMessage("Scaled Health: %u | Mana: %u | Armor: %.0f", budget.health, budget.mana, budget.armor);
        handler->PSendSysMessage("Damage Range: %.1f - %.1f | Current HP: %u / %u",
            budget.minDamage, budget.maxDamage, target->GetHealth(), target->GetMaxHealth());

        return true;
    }

    static bool HandleInstance(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        Map* map = player->GetMap();
        if (!map || !map->IsDungeon())
        {
            handler->SendSysMessage("You are not inside an instance/dungeon.");
            return true;
        }

        InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(map);
        handler->PSendSysMessage("=== Instance Group Scaling (Map: %u, Inst: %u) ===", ctx.mapId, ctx.instanceId);
        handler->PSendSysMessage("Era: %s | Tier: %s | Difficulty: %u",
            ContentEraToString(ctx.era).data(), ContentTierToString(ctx.tier).data(), uint32(ctx.difficulty));
        handler->PSendSysMessage("Intended Players: %u | Effective Players: %.1f", ctx.intendedPlayers, ctx.effectivePlayers);
        handler->PSendSysMessage("Status: %s", ctx.encounterLocked ? "[FROZEN IN ENCOUNTER]" : "[IDLE / DYNAMIC]");
        handler->PSendSysMessage("Multipliers: HP x%.2f | Damage x%.2f | Heal x%.2f | Absorb x%.2f",
            ctx.healthScale, ctx.damageScale, ctx.healingScale, ctx.absorbScale);

        return true;
    }

    static bool HandleQuest(ChatHandler* handler, uint32 questId)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
        {
            handler->PSendSysMessage("Quest %u not found.", questId);
            return true;
        }

        ProgressionLayout const& layout = sCoAContentScaling->GetLayout();
        ContentEra const era = sContentPackRegistry->ResolveEraForQuest(questId, quest->GetZoneOrSort());
        int32 const effectiveLevel = sCoAContentScaling->GetEffectiveQuestLevel(quest);
        uint32 const effectiveMin = sCoAContentScaling->GetEffectiveQuestMinLevel(quest);

        handler->PSendSysMessage("=== Quest Scaling: %s (ID: %u) ===", quest->GetTitle().c_str(), questId);
        handler->PSendSysMessage("Era: %s", ContentEraToString(era).data());
        handler->PSendSysMessage("Authored Level: %d | Effective Level: %d", quest->GetQuestLevel(), effectiveLevel);
        handler->PSendSysMessage("Authored MinLevel: %u | Effective MinLevel: %u", quest->GetMinLevel(), effectiveMin);

        return true;
    }

    static bool HandleItem(ChatHandler* handler, uint32 itemId)
    {
        ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);
        if (!item)
        {
            handler->PSendSysMessage("Item %u not found.", itemId);
            return true;
        }

        ProgressionLayout const& layout = sCoAContentScaling->GetLayout();
        ScaledItemBudget const budget = sItemBudgetScaler->CalculateItemBudget(item, layout);

        handler->PSendSysMessage("=== Item Scaling: %s (ID: %u) ===", item->Name1.c_str(), itemId);
        handler->PSendSysMessage("Authored ReqLevel: %u | Effective ReqLevel: %u", item->RequiredLevel, budget.effectiveRequiredLevel);
        handler->PSendSysMessage("Authored ItemLevel: %u | Effective ItemLevel: %u", item->ItemLevel, budget.effectiveItemLevel);
        handler->PSendSysMessage("Multipliers: Stats x%.2f | Ratings x%.2f | Armor x%.2f | DPS x%.2f",
            budget.statMultiplier, budget.ratingMultiplier, budget.armorMultiplier, budget.weaponDpsMultiplier);

        return true;
    }

    static bool HandleValidate(ChatHandler* handler)
    {
        handler->SendSysMessage("=== Validating CoA Universal Content Scaling ===");
        ProgressionLayout const& layout = sCoAContentScaling->GetLayout();
        std::string err;
        if (!layout.Validate(err))
        {
            handler->PSendSysMessage("[FAIL] ProgressionLayout invalid: %s", err.c_str());
            return true;
        }

        handler->PSendSysMessage("[PASS] ProgressionLayout valid: Cap %u", uint32(layout.maxLevel));
        handler->PSendSysMessage("[PASS] Enabled eras continuous and terminating at Cap");
        handler->PSendSysMessage("All validation checks passed.");
        return true;
    }
};

void AddCoAContentScalingCommands()
{
    new coa_content_scaling_commandscript();
}

/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * By牛皮德来
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Unit.h"


bool PlayerClassDamageEnabled = false;
bool PlayerClassDamageDebugEnabled = false;
float PlayerClassDamageRate[12] = {};


class Mod_PlayerClassDamage_Unitscript : public UnitScript
{
public:
    Mod_PlayerClassDamage_Unitscript() : UnitScript("Mod_PlayerClassDamage_Unitscript") { }
	
    void ModifyHealReceived(Unit* /*target*/, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
    {
		//直接治疗效果,不开启模块或治疗不是玩家直接退出
        if (!PlayerClassDamageEnabled || !healer || !healer->ToPlayer() || healer->GetLevel() != 80 || !PlayerClassDamageRate[healer->getClass()])
            return;

        if (spellInfo)
        {
            if (spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES) || spellInfo->Mechanic == MECHANIC_BANDAGE)
                return;

            int32 originHeal = heal;
            heal *= PlayerClassDamageRate[healer->getClass()]; 
            if (PlayerClassDamageDebugEnabled && healer->ToPlayer()->GetSession())
            {
                ChatHandler(healer->ToPlayer()->GetSession()).PSendSysMessage("玩家治疗: %s (%u) :治疗效果从 %i 调整为 %i", spellInfo->SpellName[healer->ToPlayer()->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originHeal, heal);
            }
        }
    }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
		//吸收类治疗效果,不开启模块或治疗不是玩家直接退出
        if (!PlayerClassDamageEnabled || !target || !aura)
            return;

		Unit* caster = aura->GetCaster();

		if (!caster || !caster->ToPlayer() || caster->GetLevel() != 80 || !PlayerClassDamageRate[caster->getClass()])
            return;

        if (SpellInfo const* spellInfo = aura->GetSpellInfo())
        {
            if (spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES))
                return;

            if (spellInfo->HasAura(SPELL_AURA_SCHOOL_ABSORB))
            {
                std::list<AuraEffect*> AuraEffectList  = target->GetAuraEffectsByType(SPELL_AURA_SCHOOL_ABSORB);
                for (AuraEffect* eff : AuraEffectList)
                {
                    if ((eff->GetAuraType() == SPELL_AURA_SCHOOL_ABSORB) && (eff->GetSpellInfo()->Id == spellInfo->Id))
                    {
                        int32 absorb = eff->GetAmount();
                        eff->SetAmount(absorb * PlayerClassDamageRate[caster->getClass()]);
                        if (PlayerClassDamageDebugEnabled && caster->ToPlayer()->GetSession())
                            ChatHandler(caster->ToPlayer()->GetSession()).PSendSysMessage("玩家光环: %s (%u) :吸收从 %i 调整为 %i", spellInfo->SpellName[caster->ToPlayer()->GetSession()->GetSessionDbcLocale()], spellInfo->Id, absorb, eff->GetAmount());
                    }
                }
            }
        }
    }

    void ModifyMeleeDamage(Unit* /*target*/, Unit* attacker, uint32& damage) override
    {
		//平砍类伤害,不开启模块或攻击者不是玩家不判定
        if (!PlayerClassDamageEnabled || !attacker || !attacker->ToPlayer() || attacker->GetLevel() != 80 || !PlayerClassDamageRate[attacker->getClass()])
            return;

		int32 originMelee = damage;
        damage *= PlayerClassDamageRate[attacker->getClass()];
        if (PlayerClassDamageDebugEnabled && attacker->ToPlayer()->GetSession())
            ChatHandler(attacker->ToPlayer()->GetSession()).PSendSysMessage("玩家肉搏: 伤害从 %i 调整为 %i", originMelee, damage); //这个调试信息显示不准,但是在游戏内是准确的,感觉可能还被其他机制在瞬间修改伤害值导致
    }

    void ModifySpellDamageTaken(Unit* /*target*/, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
		//技能类伤害,不开启模块或攻击者不是玩家不判定
        if (!PlayerClassDamageEnabled || !attacker || !attacker->ToPlayer() || attacker->GetLevel() != 80 || !PlayerClassDamageRate[attacker->getClass()])
            return;

        if (spellInfo)
        {
            int32 originDamage = damage;
            damage *= PlayerClassDamageRate[attacker->getClass()]; 
            if (PlayerClassDamageDebugEnabled && attacker->ToPlayer()->GetSession())
                ChatHandler(attacker->ToPlayer()->GetSession()).PSendSysMessage("玩家技能: %s (%u) :伤害从 %i 调整为 %i", spellInfo->SpellName[attacker->ToPlayer()->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originDamage, damage);
        }
    }

    void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
		//持续技能类伤害,不开启模块或攻击者不是玩家不判定
        if (!PlayerClassDamageEnabled || !attacker || !attacker->ToPlayer() || attacker->GetLevel() != 80 || !PlayerClassDamageRate[attacker->getClass()])
            return;

        if (spellInfo)
        {
            int32 originDamage = damage;
            damage *= PlayerClassDamageRate[attacker->getClass()]; 
            if (PlayerClassDamageDebugEnabled && attacker->ToPlayer()->GetSession())
                ChatHandler(attacker->ToPlayer()->GetSession()).PSendSysMessage("玩家Hot: %s (%u) :伤害从 %i 调整为 %i", spellInfo->SpellName[attacker->ToPlayer()->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originDamage, damage);
        }
	}
};


class Mod_PlayerClassDamageConfig_WorldScript : public WorldScript
{
public:
    Mod_PlayerClassDamageConfig_WorldScript() : WorldScript("Mod_PlayerClassDamageConfig_WorldScript") {}

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        //载入配置文件设定的配置
        PlayerClassDamageEnabled = sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Enable", false);
        PlayerClassDamageDebugEnabled = sConfigMgr->GetOption<bool>("ModPlayerClassDamage.DebugInfo", false);

		auto ReSetToNULL = NULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Warior", false))
			PlayerClassDamageRate[1] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.WariorRate", 1.0);
		else
			PlayerClassDamageRate[1] = ReSetToNULL;		//不要直接设为NULL,否则编译报警

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Paladin", false))
			PlayerClassDamageRate[2] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.PaladinRate", 1.0);
		else
			PlayerClassDamageRate[2] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Hunter", false))
			PlayerClassDamageRate[3] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.HunterRate", 1.0);
		else
			PlayerClassDamageRate[3] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Rogue", false))
			PlayerClassDamageRate[4] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.RogueRate", 1.0);
		else
			PlayerClassDamageRate[4] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.priest", false))
			PlayerClassDamageRate[5] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.priestRate", 1.0);
		else
			PlayerClassDamageRate[5] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Deadknight", false))
			PlayerClassDamageRate[6] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.DeadknightRate", 1.0);
		else
			PlayerClassDamageRate[6] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Shaman", false))
			PlayerClassDamageRate[7] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.ShamanRate", 1.0);
		else
			PlayerClassDamageRate[7] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Mage", false))
			PlayerClassDamageRate[8] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.MageRate", 1.0);
		else
			PlayerClassDamageRate[8] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Warlock", false))
			PlayerClassDamageRate[9] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.WarlockRate", 1.0);
		else
			PlayerClassDamageRate[9] = ReSetToNULL;

		if (sConfigMgr->GetOption<bool>("ModPlayerClassDamage.Druid", false))
			PlayerClassDamageRate[11] = sConfigMgr->GetOption<float>("ModPlayerClassDamage.DruidRate", 1.0);
		else
			PlayerClassDamageRate[11] = ReSetToNULL;
    }
};


// 加入所有脚本
void AddModPlayerClassDamageScripts()
{
    new Mod_PlayerClassDamage_Unitscript();
    new Mod_PlayerClassDamageConfig_WorldScript();
}

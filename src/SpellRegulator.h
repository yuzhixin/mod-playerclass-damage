// mod_class_damage_regulator.cpp

#include "ScriptMgr.h"
#include "Config.h"
#include "Player.h"
#include "SpellScript.h"
#include <unordered_map>

class ClassDamageRegulator
{
public:
	static ClassDamageRegulator *instance()
	{
		static ClassDamageRegulator instance;
		return &instance;
	}

	void LoadFromConfig()
	{
		std::unordered_map<uint8, float> modifiers;

		for (auto const &[name, id] : ClassNameToId)
		{
			std::string key = "ClassDamage." + name;
			float val = sConfigMgr->GetOption<float>(key.c_str(), 100.0f);
			modifiers[id] = val / 100.0f; // 转为倍率，例如 95.0 → 0.95
		}

		_classModifiers = modifiers;
		LOG_INFO("mod_class_damage_regulator", "Loaded class damage modifiers from config.");
	}

	float GetMultiplier(uint8 classId) const
	{
		auto it = _classModifiers.find(classId);
		if (it != _classModifiers.end())
			return it->second;
		return 1.0f; // 默认 1 倍
	}

private:
	std::unordered_map<uint8, float> _classModifiers;

	static std::unordered_map<std::string, uint8> ClassNameToId;
};

std::unordered_map<std::string, uint8> ClassDamageRegulator::ClassNameToId = {
	{"WARRIOR", 1}, {"PALADIN", 2}, {"HUNTER", 3}, {"ROGUE", 4}, {"PRIEST", 5}, {"DEATH_KNIGHT", 6}, {"SHAMAN", 7}, {"MAGE", 8}, {"WARLOCK", 9}, {"MONK", 10}, {"DRUID", 11}, {"DEMON_HUNTER", 12}};

#define sClassDamageRegulator ClassDamageRegulator::instance()

class ModClassDamageScript : public UnitScript
{
public:
	ModClassDamageScript() : UnitScript("ModClassDamageScript") {}

	virtual void ModifyMeleeDamage(Unit *attacker, Unit * /*victim*/, uint32 &damage) override
	{
		if (Player *player = attacker->ToPlayer())
		{
			if (player->GetLevel() == 80) // 仅80级角色应用倍率
			{
				float multiplier = sClassDamageRegulator->GetMultiplier(player->getClass());
				if (multiplier != 1.0f)
				{
					damage = static_cast<uint32>(damage * multiplier);
				}
			}
		}
	}
};

class ModClassDamageLoader : public WorldScript
{
public:
	ModClassDamageLoader() : WorldScript("ModClassDamageLoader") {}

	void OnStartup() override
	{
		sClassDamageRegulator->LoadFromConfig();
	}
};

class ModClassDamageLoginScript : public PlayerScript
{
public:
	ModClassDamageLoginScript() : PlayerScript("ModClassDamageLoginScript") {}

	void OnLogin(Player* player) override
	{
		if (player->GetLevel() == 80)
		{
			float multiplier = sClassDamageRegulator->GetMultiplier(player->getClass());
			ChatHandler(player->GetSession()).PSendSysMessage("|cff00ff00[伤害调节]|r 你的职业伤害倍率: %.1f%%", multiplier * 100);
		}
	}
};

class ModClassDamageSpellScript : public SpellScriptLoader
{
public:
	ModClassDamageSpellScript() : SpellScriptLoader("ModClassDamageSpellScript") {}

	class ModClassDamageSpellScriptImpl : public SpellScript
	{
		PrepareSpellScript(ModClassDamageSpellScriptImpl);

		void HandleHit()
		{
			if (Player *player = GetCaster()->ToPlayer())
			{
				if (player->GetLevel() == 80) // 仅80级角色应用倍率
				{
					float multiplier = sClassDamageRegulator->GetMultiplier(player->getClass());
					if (multiplier != 1.0f)
						SetHitDamage(GetHitDamage() * multiplier);
				}
			}
		}

		void Register() override
		{
			OnHit += SpellHitFn(ModClassDamageSpellScriptImpl::HandleHit);
		}
	};

	SpellScript *GetSpellScript() const override
	{
		return new ModClassDamageSpellScriptImpl();
	}
};

void Addmod_class_damage_regulator()
{
	new ModClassDamageScript();		 // 白字近战
	new ModClassDamageSpellScript(); // 技能/法术
	new ModClassDamageLoader();		 // 配置加载
	new ModClassDamageLoginScript(); // 登录提示
}

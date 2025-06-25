/*
 * Copyright (C) 2025 模块作者
 * 本文件是魔兽世界AzerothCore模块的一部分
 */

#ifndef SPELL_REGULATOR_H
#define SPELL_REGULATOR_H

#include "ScriptMgr.h"
#include "Config.h"
#include "Player.h"
#include "SpellScript.h"
#include <unordered_map>

class ClassDamageRegulator
{
public:
    static ClassDamageRegulator* instance();

    void LoadFromConfig();
    float GetMultiplier(uint8 classId) const;

private:
    std::unordered_map<uint8, float> _classModifiers;
    static std::unordered_map<std::string, uint8> ClassNameToId;
};

#define sClassDamageRegulator ClassDamageRegulator::instance()

#endif

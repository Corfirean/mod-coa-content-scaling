"""Compile the production damage hooks with a minimal server context and exercise their behaviour."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def method(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end].replace(' override', '')


def main():
    compiler = shutil.which(os.environ.get('CXX', 'cl.exe' if os.name == 'nt' else 'c++'))
    assert compiler, 'Enable a C++20 compiler'
    source = (ROOT / 'src/CoAContentScaling.cpp').read_text(encoding='utf-8')
    hooks = '\n'.join(method(source, name) for name in (
        'void ModifyMeleeDamage(', 'void ModifySpellDamageTaken(', 'void ModifyPeriodicDamageAurasTick('))
    harness = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include "SoloAssistPolicy.h"
using int32 = std::int32_t;
constexpr uint8 MAX_SPELL_EFFECTS = 3;
constexpr int SPELL_ATTR0_CU_AURA_CC = 1, SPELL_EFFECT_INSTAKILL = 2, SPELL_AURA_PERIODIC_DAMAGE_PERCENT = 3;
struct Map { bool dungeon = true; bool IsDungeon() const { return dungeon; } };
struct Unit { Map* map; bool creature = true; bool controlled = false; bool IsCreature() const { return creature; } bool IsControlledByPlayer() const { return !creature || controlled; } Map* GetMap() { return map; } };
struct SpellInfo {
  bool cc = false;
  struct EffectInfo { int Effect = 0; int ApplyAuraName = 0; } Effects[MAX_SPELL_EFFECTS];
  bool HasAttribute(int) const { return cc; }
};
struct InstanceScaleContext { float effectivePlayers = 1; float damageScale = 0.5f; };
struct Manager {
  InstanceScaleContext ctx;
  InstanceScaleContext GetOrCreateContext(Map*) const { return ctx; }
} manager;
auto sInstanceScalingMgr = &manager;
struct Controller {
  bool enabled = true, group = true;
  float difficulty = 1.0f;
  bool IsEnabled() const { return enabled; }
  bool IsGroupScalingEnabled() const { return group; }
  float GetDamageMultiplier() const { return difficulty; }
} controller;
auto sCoAContentScaling = &controller;
struct Hooks { ACTUAL_HOOKS };
int main() {
  Hooks hooks;
  Map dungeon, world; world.dungeon = false;
  Unit mob{&dungeon}, player{&dungeon, false}, outdoor{&world};
  for (float difficulty : {0.25f, 0.5f, 1.0f, 2.0f}) {
    controller.difficulty = difficulty;
    for (auto mode : {SoloAssistMode::NONE, SoloAssistMode::LIGHT, SoloAssistMode::FULL}) {
      sSoloAssistPolicy->SetMode(mode);
      float solo = sSoloAssistPolicy->GetDamageMitigationMultiplier(true, true);
      uint32 melee = 1000, tick = 1000; int32 spell = 1000;
      hooks.ModifyMeleeDamage(&player, &mob, melee);
      hooks.ModifySpellDamageTaken(&player, &mob, spell, nullptr);
      hooks.ModifyPeriodicDamageAurasTick(&player, &mob, tick, nullptr);
      assert(melee == uint32(std::ceil(1000.f * 0.5f * solo * difficulty)));
      assert(spell == int32(melee));
      // Preserve existing periodic-damage policy: solo mitigation was never applied here.
      assert(tick == uint32(std::ceil(1000.f * 0.5f * difficulty)));
    }
  }
  controller.difficulty = 0.25f;
  sSoloAssistPolicy->SetMode(SoloAssistMode::FULL);
  manager.ctx.effectivePlayers = 5;
  uint32 groupHit = 1000;
  hooks.ModifyMeleeDamage(&player, &mob, groupHit); assert(groupHit == 125);
  Unit pet{&dungeon, true, true};
  uint32 petIncoming = 1000, petOutgoing = 1000, npcOutgoing = 1000;
  hooks.ModifyMeleeDamage(&pet, &mob, petIncoming); assert(petIncoming == 125);
  hooks.ModifyMeleeDamage(&mob, &pet, petOutgoing); assert(petOutgoing == 500);
  hooks.ModifyMeleeDamage(&mob, &mob, npcOutgoing); assert(npcOutgoing == 500);
  for (Unit* attacker : {&player, &outdoor, static_cast<Unit*>(nullptr)}) {
    uint32 melee = 1000, tick = 1000; int32 spell = 1000;
    hooks.ModifyMeleeDamage(&player, attacker, melee);
    hooks.ModifySpellDamageTaken(&player, attacker, spell, nullptr);
    hooks.ModifyPeriodicDamageAurasTick(&player, attacker, tick, nullptr);
    assert(melee == 1000 && spell == 1000 && tick == 1000);
  }
  for (bool disableMaster : {false, true}) {
    controller.enabled = !disableMaster; controller.group = disableMaster;
    uint32 melee = 1000, tick = 1000; int32 spell = 1000;
    hooks.ModifyMeleeDamage(&player, &mob, melee);
    hooks.ModifySpellDamageTaken(&player, &mob, spell, nullptr);
    hooks.ModifyPeriodicDamageAurasTick(&player, &mob, tick, nullptr);
    assert(melee == 1000 && spell == 1000 && tick == 1000);
  }
  controller.enabled = controller.group = true;
  for (int effect : {SPELL_EFFECT_INSTAKILL, SPELL_AURA_PERIODIC_DAMAGE_PERCENT}) {
    SpellInfo info;
    if (effect == SPELL_EFFECT_INSTAKILL) info.Effects[0].Effect = effect;
    else info.Effects[0].ApplyAuraName = effect;
    int32 spell = 1000; uint32 tick = 1000;
    hooks.ModifySpellDamageTaken(&player, &mob, spell, &info);
    hooks.ModifyPeriodicDamageAurasTick(&player, &mob, tick, &info);
    assert(spell == 1000 && tick == 500); // Existing group multiplier remains on periodic damage.
  }
  int32 lethal = 10000000; uint32 lethalTick = 10000000;
  hooks.ModifySpellDamageTaken(&player, &mob, lethal, nullptr);
  hooks.ModifyPeriodicDamageAurasTick(&player, &mob, lethalTick, nullptr);
  assert(lethal == 10000000 && lethalTick == 5000000);
}
'''.replace('ACTUAL_HOOKS', hooks)
    with tempfile.TemporaryDirectory(prefix='coa-damage-difficulty-') as folder:
        out = Path(folder)
        (out / 'Define.h').write_text('#pragma once\n#include <cstdint>\nusing uint8=std::uint8_t; using uint32=std::uint32_t;\n')
        shutil.copyfile(ROOT / 'include/SoloAssistPolicy.h', out / 'SoloAssistPolicy.h')
        (out / 'test.cpp').write_text('#include <initializer_list>\n' + harness, encoding='utf-8')
        exe = out / ('test.exe' if os.name == 'nt' else 'test')
        flags = (['/nologo', '/std:c++20', '/EHsc', '/utf-8', 'test.cpp', '/Fe' + str(exe)]
                 if Path(compiler).stem.lower() == 'cl' else ['-std=c++20', 'test.cpp', '-o', str(exe)])
        subprocess.run([compiler, *flags], cwd=out, check=True)
        subprocess.run([str(exe)], cwd=out, check=True)
    print('Production damage hooks: difficulty, solo, group, disabled and protected mechanics passed')


if __name__ == '__main__':
    main()

import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    compiler = shutil.which(os.environ.get('CXX', 'cl.exe' if os.name == 'nt' else 'c++'))
    assert compiler, 'Enable a C++20 compiler'
    with tempfile.TemporaryDirectory(prefix='coa-health-controls-') as folder:
        out = Path(folder)
        for name in ('CombatBudgetProfile', 'CreatureScaleContext', 'ContentEra', 'ContentTier'):
            shutil.copyfile(ROOT / f'include/{name}.h', out / f'{name}.h')
        shutil.copyfile(ROOT / 'src/CombatBudgetProfile.cpp', out / 'CombatBudgetProfile.cpp')
        (out / 'Define.h').write_text('#pragma once\n#include <cstdint>\nusing uint8=std::uint8_t; using uint32=std::uint32_t;\n')
        (out / 'Map.h').write_text('''#pragma once
#include "Define.h"
struct Map {
  bool dungeon=false, raid=false, heroic=false;
  uint32 GetId() const { return 0; }
  bool IsDungeon() const { return dungeon; }
  bool IsRaid() const { return raid; }
  bool IsHeroic() const { return heroic; }
};
''')
        (out / 'Creature.h').write_text('''#pragma once
#include "Map.h"
struct Creature {
  Map map;
  float multiplier=1.f;
  Map const* GetMap() const { return &map; }
  uint32 GetAreaId() const { return 0; }
};
''')
        (out / 'CreatureData.h').write_text('''#pragma once
#include "Define.h"
enum { CREATURE_ELITE_NORMAL, CREATURE_ELITE_ELITE, CREATURE_ELITE_RAREELITE, CREATURE_ELITE_WORLDBOSS, CREATURE_ELITE_RARE };
enum : uint32 { CREATURE_FLAG_EXTRA_DUNGEON_BOSS = 0x10000000 };
struct CreatureTemplate {
  uint32 Entry=705; uint8 maxlevel=1, expansion=0, unit_class=1, rank=CREATURE_ELITE_NORMAL;
  uint32 flags_extra=0;
  float ModHealth=.93f, ModMana=1.f, ModArmor=1.f, DamageModifier=1.f;
};
struct CreatureBaseStats {
  uint32 BaseHealth[1]={42}, BaseMana=0, AttackPower=0;
  float BaseDamage[1]={2.f}, BaseArmor=1.f;
};
''')
        (out / 'ObjectMgr.h').write_text('''#pragma once
#include "CreatureData.h"
struct ObjectMgr {
  CreatureBaseStats stats;
  CreatureBaseStats const* GetCreatureBaseStats(uint8, uint8) const { return &stats; }
};
inline ObjectMgr objectMgr;
inline auto sObjectMgr=&objectMgr;
''')
        (out / 'ContentPackRegistry.h').write_text('''#pragma once
#include "ContentEra.h"
struct Registry {
  ContentEra ResolveEraForCreature(uint32, uint32, uint32, uint8, uint8) const { return ContentEra::Classic; }
};
inline Registry registry;
inline auto sContentPackRegistry=&registry;
''')
        (out / 'CoAContentScaling.h').write_text('''#pragma once
#include "Define.h"
struct ContentScalingStub {
  float difficulty=1.f;  // CoAContentScaling.Difficulty.Health.* for any place and role
  float GetHealthMultiplier(bool, bool, bool) const { return difficulty; }
};
inline ContentScalingStub contentScaling;
inline auto sCoAContentScaling=&contentScaling;
''')
        (out / 'CoACreatureScaling.h').write_text('''#pragma once
#include "Creature.h"
namespace CreatureScaling {
inline float HealthMultiplier(Creature const* creature) { return creature ? creature->multiplier : 1.f; }
}
''')
        (out / 'CoACreatureScalingPolicy.h').write_text('''#pragma once
#include "Define.h"
namespace CreatureScaling {
inline uint32 Scaled(uint32 value, float multiplier) { return uint32(double(value)*multiplier+.5); }
}
''')
        (out / 'World.h').write_text('''#pragma once
enum ServerConfigs { RATE_CREATURE_NORMAL_HP, RATE_CREATURE_ELITE_ELITE_HP, RATE_CREATURE_ELITE_RAREELITE_HP, RATE_CREATURE_ELITE_WORLDBOSS_HP, RATE_CREATURE_ELITE_RARE_HP };
struct World {
  float rates[5]={1.f,1.f,1.f,1.f,1.f};
  float getRate(ServerConfigs rate) const { return rates[rate]; }
};
inline World world;
inline auto sWorld=&world;
''')
        (out / 'test.cpp').write_text('''#include <cassert>
#include <cmath>
#include "CoAContentScaling.h"
#include "CombatBudgetProfile.h"
#include "Creature.h"
#include "CreatureData.h"
#include "World.h"
int main() {
  CreatureTemplate wolf;
  Creature creature;
  auto baseline=sCombatBudgetProfile->BuildContext(&wolf,&creature,1);
  assert(sCombatBudgetProfile->CalculateBudget(&wolf,baseline).health==40);
  creature.multiplier=2.5f;
  auto scaled=sCombatBudgetProfile->BuildContext(&wolf,&creature,1);
  assert(sCombatBudgetProfile->CalculateBudget(&wolf,scaled).health==100);
  for (int i=0;i<5;++i) {
    scaled=sCombatBudgetProfile->BuildContext(&wolf,&creature,57);
    assert(sCombatBudgetProfile->CalculateBudget(&wolf,scaled).health==100);
  }
  world.rates[RATE_CREATURE_NORMAL_HP]=2.f;
  assert(sCombatBudgetProfile->CalculateBudget(&wolf,scaled).health==200);
  scaled.groupHealthScale=.5f;
  assert(sCombatBudgetProfile->CalculateBudget(&wolf,scaled).health==100);
  world.rates[RATE_CREATURE_NORMAL_HP]=1.f;
  for (auto tier : {ContentTier::DUNGEON_NORMAL,ContentTier::RAID_ENTRY}) {
    baseline.tier=tier; scaled=baseline; scaled.healthModifier=2.2f;
    auto hp=sCombatBudgetProfile->CalculateBudget(&wolf,baseline).health;
    assert(sCombatBudgetProfile->CalculateBudget(&wolf,scaled).health==uint32(hp*2.2+.5));
  }
  creature.multiplier=1.f;
  for (uint8 rank=0;rank<5;++rank) {
    wolf.rank=rank;
    auto ctx=sCombatBudgetProfile->BuildContext(&wolf,&creature,1);
    for (auto& rate : world.rates) rate=1.f;
    auto hp=sCombatBudgetProfile->CalculateBudget(&wolf,ctx).health;
    world.rates[rank]=2.f;
    assert(sCombatBudgetProfile->CalculateBudget(&wolf,ctx).health==hp*2);
  }
  for (auto& rate : world.rates) rate=1.f;

  // Authored budget only: ModHealth already carries the rank, so there is no elite x2, world boss x4 or normal dungeon
  // x1.35 on top. Base HP 42: elite trash ModHealth 3 -> 126 at a full party. (Old stack at a full party, with the core
  // dungeon multiplier 2.5: 42 * 3 * 1.35 * 2 * 2.5 = 850.)
  CreatureTemplate trash; trash.rank=CREATURE_ELITE_ELITE; trash.ModHealth=3.f;
  CreatureTemplate boss=trash; boss.ModHealth=10.f; boss.flags_extra=CREATURE_FLAG_EXTRA_DUNGEON_BOSS;
  CreatureTemplate worldBoss; worldBoss.rank=CREATURE_ELITE_WORLDBOSS; worldBoss.ModHealth=100.f;
  Creature inDungeon; inDungeon.map.dungeon=true;
  Creature inRaid; inRaid.map.dungeon=true; inRaid.map.raid=true;
  Creature inOpenWorld;
  assert(CombatBudgetProfile::GetTierHealthMultiplier(ContentTier::DUNGEON_NORMAL)==1.f);
  assert(CombatBudgetProfile::GetTierHealthMultiplier(ContentTier::DUNGEON_HEROIC)==2.2f);

  auto trashCtx=sCombatBudgetProfile->BuildContext(&trash,&inDungeon,1);
  assert(!trashCtx.boss && trashCtx.tier==ContentTier::DUNGEON_NORMAL);
  assert(sCombatBudgetProfile->CalculateBudget(&trash,trashCtx).health==126);
  for (auto scale : {.217f,.419f,.616f,.809f,1.f}) {
    trashCtx.groupHealthScale=scale;
    assert(sCombatBudgetProfile->CalculateBudget(&trash,trashCtx).health==uint32(std::ceil(126.f*scale)));
  }
  auto bossCtx=sCombatBudgetProfile->BuildContext(&boss,&inDungeon,1);
  assert(bossCtx.boss && !bossCtx.worldBoss);
  assert(sCombatBudgetProfile->CalculateBudget(&boss,bossCtx).health==420);  // above trash by ModHealth alone

  // Boss identity: only the encounter flag or the WORLDBOSS rank. Elite trash is trash in dungeons and raids.
  assert(!CombatBudgetProfile::IsBossTemplate(&trash));
  assert(CombatBudgetProfile::IsBossTemplate(&boss));
  assert(CombatBudgetProfile::IsBossTemplate(&worldBoss));
  assert(!sCombatBudgetProfile->BuildContext(&trash,&inRaid,1).boss);
  auto worldBossCtx=sCombatBudgetProfile->BuildContext(&worldBoss,&inOpenWorld,1);
  assert(worldBossCtx.boss && worldBossCtx.worldBoss);
  assert(sCombatBudgetProfile->CalculateBudget(&worldBoss,worldBossCtx).health==4200);  // no x4
  auto raidBossCtx=sCombatBudgetProfile->BuildContext(&worldBoss,&inRaid,1);
  assert(raidBossCtx.boss && !raidBossCtx.worldBoss && raidBossCtx.tier==ContentTier::RAID_MID);

  // CoAContentScaling.Difficulty.Health.* is the one explicit place and role knob and stays on top of the budget.
  contentScaling.difficulty=1.5f;
  assert(sCombatBudgetProfile->CalculateBudget(&trash,sCombatBudgetProfile->BuildContext(&trash,&inDungeon,1)).health==189);
  contentScaling.difficulty=1.f;

  // Breakdown used by CoAContentScaling.Debug.
  trashCtx=sCombatBudgetProfile->BuildContext(&trash,&inDungeon,1);
  trashCtx.groupHealthScale=.5f;
  auto parts=sCombatBudgetProfile->CalculateBudget(&trash,trashCtx);
  assert(parts.authoredHealth==126.f && parts.tierHealthMultiplier==1.f && parts.difficultyHealthMultiplier==1.f);
  assert(parts.externalHealthMultiplier==1.f && parts.health==63);

  // The core multiplier only enters through the context; the module's ownership makes it 1.0 for dungeons and raids,
  // so a module-disabled server (healthModifier = core Dungeon value) is simply the old core behaviour on top.
  inDungeon.multiplier=2.5f;
  assert(sCombatBudgetProfile->CalculateBudget(&trash,sCombatBudgetProfile->BuildContext(&trash,&inDungeon,1)).health==315);
}
''')
        exe = out / ('test.exe' if os.name == 'nt' else 'test')
        files = ['CombatBudgetProfile.cpp', 'test.cpp']
        flags = (['/nologo', '/std:c++20', '/EHsc', '/utf-8', *files, '/Fe' + str(exe)]
                 if Path(compiler).stem.lower() == 'cl' else ['-std=c++20', *files, '-o', str(exe)])
        subprocess.run([compiler, *flags], cwd=out, check=True)
        subprocess.run([str(exe)], cwd=out, check=True)
    print('Production health budget: owner multipliers, rank rates, group scaling and repeat calculation passed')


if __name__ == '__main__':
    main()

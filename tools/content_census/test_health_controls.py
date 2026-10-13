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
struct CreatureTemplate {
  uint32 Entry=705; uint8 maxlevel=1, expansion=0, unit_class=1, rank=CREATURE_ELITE_NORMAL;
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

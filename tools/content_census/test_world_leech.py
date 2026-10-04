"""Compile the production open-world leech hook and exercise damage and scope guards."""
from pathlib import Path
import os, shutil, subprocess, tempfile
from test_scaling_controls import method
ROOT = Path(__file__).resolve().parents[2]
def main():
    hook = method((ROOT/'src/CoAContentScaling.cpp').read_text(), 'void OnDamage(').replace(' override', '')
    harness = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
using uint32 = std::uint32_t;
struct Map { bool world = true; bool IsWorldMap() const { return world; } };
struct Unit {
    Map* map; bool player = false, alive = true, controlled = false, evade = false, flight = false;
    uint32 hp = 1000, max = 2000;
    bool IsPlayer() const { return player; } bool IsCreature() const { return !player; }
    bool IsAlive() const { return alive; } bool IsControlledByPlayer() const { return controlled; }
    Unit* ToCreature() { return this; } bool IsEvadingAttacks() const { return evade; }
    bool IsInFlight() const { return flight; } Map* GetMap() { return map; }
    uint32 GetHealth() const { return hp; } uint32 GetMaxHealth() const { return max; }
    static void DealHeal(Unit*, Unit* victim, uint32 heal) { victim->hp += heal; }
};
struct Controller {
    bool enabled = true, leech = true; float percent = 25;
    bool IsEnabled() const { return enabled; } bool IsWorldLeechEnabled() const { return leech; }
    float GetWorldLeechPercent() const { return percent; }
} controller;
auto sCoAContentScaling = &controller;
struct Hooks { ACTUAL_HOOK };
int main() {
    Hooks hook; Map world, instance; instance.world = false;
    Unit player{&world}, mob{&world}; player.player = true;
    uint32 damage = 400;
    hook.OnDamage(&player, &mob, damage); assert(player.hp == 1100 && damage == 400);
    mob.hp = 40; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1110);
    mob.hp = 1000; player.hp = 1999; hook.OnDamage(&player, &mob, damage); assert(player.hp == 2000);
    player.hp = 1000;
    for (bool* flag : {&mob.controlled, &mob.evade, &mob.flight, &mob.player}) {
        *flag = true; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1000); *flag = false;
    }
    for (bool* flag : {&controller.enabled, &controller.leech, &player.alive, &mob.alive}) {
        *flag = false; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1000); *flag = true;
    }
    player.map = mob.map = &instance; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1000);
    player.map = &world; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1000); mob.map = &world;
    hook.OnDamage(nullptr, &mob, damage); hook.OnDamage(&player, nullptr, damage);
    hook.OnDamage(&player, &player, damage); assert(player.hp == 1000);
    damage = 0; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1000);
    damage = 400; controller.percent = 0; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1000);
    controller.percent = 100; hook.OnDamage(&player, &mob, damage); assert(player.hp == 1400);
}
'''.replace('ACTUAL_HOOK', hook)
    compiler = shutil.which('cl.exe' if os.name == 'nt' else 'c++'); assert compiler
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp); (p/'test.cpp').write_text(harness); exe=p/('test.exe' if os.name=='nt' else 'test')
        flags = ['/nologo','/std:c++20','/EHsc','test.cpp','/Fe'+str(exe)] if os.name=='nt' else ['-std=c++20','test.cpp','-o',str(exe)]
        subprocess.run([compiler,*flags],cwd=p,check=True); subprocess.run([str(exe)],check=True)
    print('Open-world leech regression passed')
if __name__ == '__main__': main()

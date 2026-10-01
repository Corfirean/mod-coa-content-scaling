/*
 * CoA Universal Content Scaling
 * CoAContentScaling: Master controller and lifecycle manager.
 */

#ifndef COA_CONTENT_SCALING_H
#define COA_CONTENT_SCALING_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "Define.h"
#include "InstanceScaleContext.h"
#include "ProgressionLayout.h"
#include <string>

class Creature;
class Player;
class Unit;
class Quest;
class Map;
struct CreatureTemplate;

class CoAContentScaling
{
public:
    static CoAContentScaling* Instance();

    void LoadConfig();
    void InitializeLayout();

    [[nodiscard]] bool IsEnabled() const { return _enabled; }
    [[nodiscard]] bool IsGroupScalingEnabled() const { return _groupScalingEnabled; }
    [[nodiscard]] bool IsAdaptiveMechanicsEnabled() const { return _adaptiveMechanicsEnabled; }
    [[nodiscard]] bool IsDebugEnabled() const { return _debug; }

    [[nodiscard]] ProgressionLayout const& GetLayout() const { return _layout; }

    // Effective Level Mapping
    [[nodiscard]] uint8 GetEffectiveCreatureLevel(CreatureTemplate const* cinfo, Creature const* creature, uint8 authoredLevel) const;
    [[nodiscard]] int32 GetEffectiveQuestLevel(Quest const* quest) const;
    [[nodiscard]] uint32 GetEffectiveQuestMinLevel(Quest const* quest) const;

    // Combat scaling calculations
    void ApplyCreatureScaling(CreatureTemplate const* cinfo, Creature* creature);

    // Expansion enablement
    void SetTbcEnabled(bool enabled) { _tbcEnabled = enabled; }
    void SetWotlkEnabled(bool enabled) { _wotlkEnabled = enabled; }
    [[nodiscard]] bool IsTbcEnabled() const { return _tbcEnabled; }
    [[nodiscard]] bool IsWotlkEnabled() const { return _wotlkEnabled; }

    // Map access validation
    [[nodiscard]] bool CanPlayerEnterMap(Player const* player, uint32 mapId) const;

private:
    CoAContentScaling() = default;

    bool _enabled{true};
    bool _tbcEnabled{false};
    bool _wotlkEnabled{false};

    bool _groupScalingEnabled{true};
    bool _lockOnEncounterStart{true};
    bool _adaptiveMechanicsEnabled{true};
    bool _scaleLootCount{true};
    bool _allowSoloRaids{true};
    bool _debug{false};

    std::string _progressionMode{"Auto"};
    uint8 _customClassicEnd{0};
    uint8 _customTbcEnd{0};

    ProgressionLayout _layout;
};

#define sCoAContentScaling CoAContentScaling::Instance()

#endif // COA_CONTENT_SCALING_H

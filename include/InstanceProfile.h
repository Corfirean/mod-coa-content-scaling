/*
 * CoA Universal Content Scaling
 * InstanceProfile: Explicit metadata and sizing rules for dungeons and raids across eras.
 */

#ifndef COA_INSTANCE_PROFILE_H
#define COA_INSTANCE_PROFILE_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "Define.h"
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct InstanceProfile
{
    uint32 mapId{0};
    std::string_view name;
    ContentEra era{ContentEra::Classic};
    uint32 defaultGroupSize{5};
    uint32 raid10Size{10};
    uint32 raid25Size{25};
    bool isRaid{false};
};

class InstanceProfileRegistry
{
public:
    static InstanceProfileRegistry* Instance();

    void Initialize();
    [[nodiscard]] InstanceProfile const* GetProfile(uint32 mapId) const;
    [[nodiscard]] std::optional<ContentEra> GetEraForMap(uint32 mapId, uint8 difficulty = 0) const;
    [[nodiscard]] uint32 GetIntendedPlayers(uint32 mapId, uint8 difficulty) const;

    [[nodiscard]] bool ValidateAll(std::vector<std::string>& issues) const;

private:
    InstanceProfileRegistry();

    std::unordered_map<uint32, InstanceProfile> _profiles;
};

#define sInstanceProfileRegistry InstanceProfileRegistry::Instance()

#endif // COA_INSTANCE_PROFILE_H

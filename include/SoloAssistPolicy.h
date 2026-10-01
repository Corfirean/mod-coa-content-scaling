/*
 * CoA Universal Content Scaling
 * SoloAssistPolicy: Optional assist layer addressing role deficit for solo/small group gameplay.
 */

#ifndef COA_SOLO_ASSIST_POLICY_H
#define COA_SOLO_ASSIST_POLICY_H

#include "Define.h"

enum class SoloAssistMode : uint8
{
    NONE  = 0,
    LIGHT = 1,
    FULL  = 2
};

class SoloAssistPolicy
{
public:
    static SoloAssistPolicy* Instance()
    {
        static SoloAssistPolicy instance;
        return &instance;
    }

    void SetMode(SoloAssistMode mode) { _mode = mode; }
    [[nodiscard]] SoloAssistMode GetMode() const { return _mode; }

    [[nodiscard]] float GetDamageMitigationMultiplier(bool isSolo, bool isDungeonOrRaid) const
    {
        if (!isSolo || !isDungeonOrRaid)
            return 1.0f;

        switch (_mode)
        {
            case SoloAssistMode::LIGHT: return 0.85f;
            case SoloAssistMode::FULL:  return 0.70f;
            case SoloAssistMode::NONE:
            default:                    return 1.0f;
        }
    }

private:
    SoloAssistPolicy() = default;
    SoloAssistMode _mode{SoloAssistMode::NONE};
};

#define sSoloAssistPolicy SoloAssistPolicy::Instance()

#endif // COA_SOLO_ASSIST_POLICY_H

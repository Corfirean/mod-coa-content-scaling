/*
 * CoA Universal Content Scaling
 * EncounterAdapterRegistry: Registers built-in curated encounter adapters.
 */

#include "AdaptiveEncounterAPI.h"
#include <memory>

// Forward declarations of adapter factory functions
std::shared_ptr<IEncounterAdapter> CreateRazorgoreAdapter();
std::shared_ptr<IEncounterAdapter> CreateTwinEmperorsAdapter();
std::shared_ptr<IEncounterAdapter> CreateChessEventAdapter();
std::shared_ptr<IEncounterAdapter> CreateFourHorsemenAdapter();
std::shared_ptr<IEncounterAdapter> CreateFlameLeviathanAdapter();
std::shared_ptr<IEncounterAdapter> CreateValithriaAdapter();
std::shared_ptr<IEncounterAdapter> CreateLichKingAdapter();

void RegisterCuratedEncounterAdapters()
{
    sAdaptiveEncounterMgr->RegisterAdapter(CreateRazorgoreAdapter());
    sAdaptiveEncounterMgr->RegisterAdapter(CreateTwinEmperorsAdapter());
    sAdaptiveEncounterMgr->RegisterAdapter(CreateChessEventAdapter());
    sAdaptiveEncounterMgr->RegisterAdapter(CreateFourHorsemenAdapter());
    sAdaptiveEncounterMgr->RegisterAdapter(CreateFlameLeviathanAdapter());
    sAdaptiveEncounterMgr->RegisterAdapter(CreateValithriaAdapter());
    sAdaptiveEncounterMgr->RegisterAdapter(CreateLichKingAdapter());
}

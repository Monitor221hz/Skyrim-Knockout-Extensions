#pragma once 
#include <shared_mutex>

#include "settings.h"
#include "util.h"

using namespace RE; 

namespace KnockoutExtensions
{
    constexpr uint32_t SerializeCode = 'KNEX';
    constexpr uint32_t SerializeVersion = 1;
    
    constexpr uint32_t SerializeActor = 'KNAC';

    class KnockoutHandler
    {

        public:
            static bool CanPassOut(Actor *a_actor);
            static void ApplyUnconscious(Actor* a_actor);
            static void ApplyUnconscious(Actor* a_actor, Actor* a_causer);

            static void RecoverUnconscious(Actor* a_actor);

            static void UpdateTrackedActors();


            static void TrackActor(Actor* a_actor);
            static void UntrackActor(Actor* a_actor);
            
            static void GameSaveCallback(SKSE::SerializationInterface* serde);
            static void GameLoadCallback(SKSE::SerializationInterface* serde);
            static void GameRevertCallback(SKSE::SerializationInterface* serde);
        private:
            using Lock = std::shared_mutex;
            using ReadLocker = std::shared_lock<Lock>;
            using WriteLocker = std::unique_lock<Lock>;

            static inline Lock actorLock; 

            static inline std::unordered_map<FormID, float> actorIDMap; 

            

            static bool GameSave(SKSE::SerializationInterface* serde);
            static bool GameLoad(SKSE::SerializationInterface* serde); 
            static void GameRevert(SKSE::SerializationInterface* serde);

            static bool RecordSave(SKSE::SerializationInterface* serde, uint32_t a_type, uint32_t a_version);
            
            static void InterruptAll(Actor* a_actor);
            static void SetUnconsciousFlags(Actor *a_actor);
    };
}
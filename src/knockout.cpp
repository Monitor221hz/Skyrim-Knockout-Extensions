#include "knockout.h"
#include <limits>

namespace KnockoutExtensions
{
    void KnockoutHandler::InterruptAll(Actor *a_actor)
    {
        if (a_actor->IsInCombat())
        {
            a_actor->StopCombat();
        }
        a_actor->InterruptCast(false); 
        a_actor->PauseCurrentDialogue(); 
        a_actor->StopInteractingQuick(true);  
    }

    bool KnockoutHandler::CanPassOut(Actor* a_actor)
    {
        auto* scene = a_actor->GetCurrentScene(); 
        if (Settings::GetKnockoutHumanoidOnly())
        {
            auto bodyPartData = a_actor->GetRace() ? a_actor->GetRace()->bodyPartData : nullptr;
            if (!bodyPartData || bodyPartData->GetFormID() != 0x1d)
            {
                return false; 
            }
        }
        if (scene != nullptr && scene->isPlaying) //scene->isRunning
        {
            int i = 0; 
            for(auto formId : scene->actors)
            {
                if (formId == a_actor->GetFormID())
                {
                    if (!scene->actorFlags[i].any(SCENE_ACTOR_FLAG::kOptional))
                    {
                        RE::DebugNotification("Non-optional actors in active Scenes cannot become unconscious.");
                        return false; 
                    }
                }
                i++; 
            }
        } 

        auto& extraList = a_actor->extraList; 
        if (extraList.HasType(ExtraDataType::kAliasInstanceArray))
        {
            auto* extra_ref_alias_array = extraList.GetByType<ExtraAliasInstanceArray>(); 
            BSReadLockGuard extra_lock(extra_ref_alias_array->lock); 
            for(auto* extra_alias_instance : extra_ref_alias_array->aliases)
            {
                auto* quest = extra_alias_instance->quest;
                if (!quest->IsRunning() || quest->GetType() != QUEST_DATA::Type::kMainQuest) { continue; }
                BSReadLockGuard lock(extra_alias_instance->quest->aliasAccessLock); 
                if (auto* alias = extra_alias_instance->alias)
                {
                    if (alias->flags.any(BGSBaseAlias::FLAGS::kReserves) && !alias->flags.any(BGSBaseAlias::FLAGS::kAllowDead))
                    {
                        RE::DebugNotification("Reserved actors cannot become unconscious.");
                        return false; 
                    }
                }
            }

        }
        return true; 
    }
    void KnockoutHandler::SetUnconsciousFlags(Actor* a_actor)
    {
        auto* actor_state = a_actor->AsActorState(); 
        auto& rtd = a_actor->GetActorRuntimeData();
        if (actor_state)
        {
            auto& state1 = actor_state->actorState1; 
            state1.knockState = KNOCK_STATE_ENUM::kDown;
            state1.sitSleepState = SIT_SLEEP_STATE::kIsSleeping; 
        }
        rtd.boolFlags.set(Actor::BOOL_FLAGS::kDoNotShowOnStealthMeter);
        auto& extraList = a_actor->extraList; 
        // extraList.SetExtraFlags(ExtraFlags::Flag::kBlockPlayerActivate, false); 
        // extraList.SetExtraFlags(ExtraFlags::Flag::kNone, true);
        // a_actor->SetActivationBlocked(false);
    }
    void KnockoutHandler::ApplyUnconscious(Actor *a_actor)
    {
        ReadLocker locker(actorLock);
        if (actorIDMap.count(a_actor->GetFormID()) > 0) { return; }
        locker.unlock();
        InterruptAll(a_actor); 
        NiPoint3 actorPos = a_actor->GetPosition();
        auto& rtd = a_actor->GetActorRuntimeData(); 
        a_actor->SetLifeState(ACTOR_LIFE_STATE::kAlive);
        ActorUtil::Physics::PushActorAway(a_actor->GetActorRuntimeData().currentProcess, a_actor, &actorPos, std::numeric_limits<float>::min());
        a_actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious);
        SetUnconsciousFlags(a_actor); 
    }
    void KnockoutHandler::ApplyUnconscious(Actor *a_actor, Actor *a_causer)
    {
        ReadLocker locker(actorLock);
        if (actorIDMap.count(a_actor->GetFormID()) > 0) { return; }
        locker.unlock();
        InterruptAll(a_actor); 
        uint64_t witnessCount; 
        auto& rtd = a_actor->GetActorRuntimeData();
        if (ActorUtil::Detection::GetHighestDetectionValue(a_causer, &witnessCount) > 0 && Settings::GetKnockoutIsCrime())
        {
            ActorUtil::Detection::SendAssaultAlarm(a_actor, a_causer, false);
            rtd.boolBits.set(Actor::BOOL_BITS::kMurderAlarm);
        }
        NiPoint3 actorPos = a_causer->GetPosition();
        a_actor->SetLifeState(ACTOR_LIFE_STATE::kAlive);
        ActorUtil::Physics::PushActorAway(a_actor->GetActorRuntimeData().currentProcess, a_actor, &actorPos, std::numeric_limits<float>::min());
        a_actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious);
        SetUnconsciousFlags(a_actor); 
    }
    void KnockoutHandler::RecoverUnconscious(Actor *a_actor)
    {
        auto& rtd = a_actor->GetActorRuntimeData();
        rtd.boolFlags.reset(Actor::BOOL_FLAGS::kDoNotShowOnStealthMeter);
        rtd.boolBits.reset(Actor::BOOL_BITS::kMurderAlarm);
        auto* actor_state = a_actor->AsActorState(); 
        if (actor_state)
        {
            auto& state1 = actor_state->actorState1; 
            state1.knockState = KNOCK_STATE_ENUM::kGetUp; 
            state1.sitSleepState = SIT_SLEEP_STATE::kNormal; 
        }
        if (a_actor->Is3DLoaded()) 
        { 
            a_actor->Update3DModel(); 
            // a_actor->DoReset3D(false); 
            a_actor->UpdateActor3DPosition(); 
            if (auto* model = a_actor->Get3D())
            {
                model->SetMaterialNeedsUpdate(true); 
            }
            NiPoint3 actorPos = a_actor->GetPosition();
            ActorUtil::Physics::PushActorAway(rtd.currentProcess, a_actor, &actorPos, std::numeric_limits<float>::min());
        }
    }
    void KnockoutHandler::TrackActor(Actor *a_actor)
    {
        auto *calendar = Calendar::GetSingleton();
        if (!calendar)
        {
            a_actor->SetLifeState(ACTOR_LIFE_STATE::kAlive);
            RecoverUnconscious(a_actor);
            return;
        }
        float exactHoursPassed = calendar->GetHoursPassed();
        
        WriteLocker locker(actorLock); 
        actorIDMap.emplace(a_actor->GetFormID(), exactHoursPassed);
    }
    void KnockoutHandler::UpdateTrackedActors()
    {
        WriteLocker locker(actorLock);

        auto* calendar = Calendar::GetSingleton(); 
        if (!calendar) { return; }
        float exactHoursPassed = calendar->GetHoursPassed();
        float duration = Settings::GetUnconsciousDuration();
        std::vector<FormID> markedFormIDs;
        for(auto& it : actorIDMap)
        {
            auto formID = it.first; 
            if (exactHoursPassed - it.second > duration)
            {
                markedFormIDs.emplace_back(formID);
                auto* actor = TESForm::LookupByID(formID)->As<Actor>();
                if (actor == nullptr) { continue; }
                actor->SetLifeState(ACTOR_LIFE_STATE::kAlive);       
                RecoverUnconscious(actor); 
            }
            // else 
            // {
            //     auto* actor = TESForm::LookupByID(formID)->As<Actor>();
            //     actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious); 
            // }
        }
        for (auto formID : markedFormIDs)
        {
            actorIDMap.erase(formID);
        }

    }

    void KnockoutHandler::UntrackActor(Actor* a_actor)
    {
        WriteLocker locker(actorLock);
        actorIDMap.erase(a_actor->GetFormID());
    }

    bool KnockoutHandler::GameSave(SKSE::SerializationInterface *serde)
    {
        assert(serde); 

        ReadLocker locker(actorLock);

        const std::size_t numActors = actorIDMap.size();

        if (!serde->WriteRecordData(numActors))
        {
            SKSE::log::error("Failed to save {} unconscious actors!", numActors); 
            return false; 
        }
        uint16_t actorCount = 0;
        for (auto& it : actorIDMap)
        {
            auto formID = it.first; 
            float time = it.second;
            if (!serde->WriteRecordData(formID))
            {
                SKSE::log::error("Failed to save actor FormID {}", formID); 
                return false; 
            }
            if (!serde->WriteRecordData(time))
            {
                SKSE::log::error("Failed to save timestamp {}", time); 
                return false; 
            }
            actorCount++; 
        }
        SKSE::log::info("{} unconscious actors saved", actorCount);
        return true; 
    }
    bool KnockoutHandler::GameLoad(SKSE::SerializationInterface *serde)
    {
        assert(serde);

        std::size_t size; 
        serde->ReadRecordData(size);
        WriteLocker locker(actorLock);

        actorIDMap.clear(); 

        RE::FormID formID; 
        float hoursPassed; 
        uint16_t actorCount = 0;
        for (std::size_t i = 0; i < size; i++) {
            serde->ReadRecordData(formID); 
            if (!serde->ResolveFormID(formID, formID))
            {
                SKSE::log::error("Failed to resolve formID {}", formID);
                continue; 
            }
            serde->ReadRecordData(hoursPassed); 
            actorIDMap.emplace(formID, hoursPassed);
            actorCount++;
        }
        SKSE::log::info("{} unconscious actors loaded", actorCount);

        return true; 
    }
    void KnockoutHandler::GameRevert(SKSE::SerializationInterface *serde)
    {
        WriteLocker locker(actorLock); 

        actorIDMap.clear();
    }

    bool KnockoutHandler::RecordSave(SKSE::SerializationInterface *serde, uint32_t a_type, uint32_t a_version)
    {
        if (!serde->OpenRecord(a_type, a_version))
        {
            SKSE::log::error("Failed to open cosave record!");
            return false; 
        }
        return GameSave(serde);
    }

    void KnockoutHandler::GameSaveCallback(SKSE::SerializationInterface *serde)
    {
        if (!RecordSave(serde, SerializeActor, SerializeVersion))
        {
            SKSE::log::critical("Failed to save unconscious actors!"); 
        }
        
        SKSE::log::info("Finished game cosave."); 
    }
    void KnockoutHandler::GameLoadCallback(SKSE::SerializationInterface *serde)
    {
        uint32_t type; 
        uint32_t version; 
        uint32_t length; 

        while (serde->GetNextRecordInfo(type, version, length))
        {
            if (version != SerializeVersion)
            {
                SKSE::log::critical("Loaded data is out of date! Read ({}), expected ({}) for type code ({})", version, SerializeCode, type);
            }
            if (type == SerializeActor)
            {
                if (!GameLoad(serde)) 
                {
                    SKSE::log::info("Failed to load unconscious actors!");
                }
            }
        }

    }
    void KnockoutHandler::GameRevertCallback(SKSE::SerializationInterface *serde)
    {
        GameRevert(serde);
    }
}
#include "hook.h"
#pragma once

namespace KnockoutExtensions
{



    void HitEventHook::ProcessHitEvent(TESObjectREFR *a_target, HitData *a_hitData)
    {
        if (!a_target || !a_hitData || a_target->GetFormType() != FormType::ActorCharacter || !a_hitData->flags.any(HitData::Flag::kBash)) { return _ProcessHitEvent(a_target, a_hitData); }
        
        auto* target_actor = a_target->As<Actor>();
        if (target_actor->IsPlayerRef() || !KnockoutHandler::CanPassOut(target_actor)) { return _ProcessHitEvent(a_target, a_hitData); }

        auto* targetState = target_actor->AsActorState();
        bool inBleedout = targetState ? targetState->GetLifeState() == ACTOR_LIFE_STATE::kBleedout : false;

        if (target_actor->IsInCombat() && !inBleedout) { return _ProcessHitEvent(a_target, a_hitData); }

        auto* attacker = a_hitData->aggressor.get().get();
        if (!attacker) { return _ProcessHitEvent(a_target, a_hitData); }
        
        bool arg2;
        bool hasLOS = target_actor->HasLineOfSight(attacker,arg2);


        if ((hasLOS || !Settings::GetTriggerBackBash()) && (!inBleedout || !Settings::GetTriggerBleedoutBash())) { return _ProcessHitEvent(a_target, a_hitData); }

        target_actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious);
        KnockoutHandler::ApplyUnconscious(target_actor, attacker);
        KnockoutHandler::TrackActor(target_actor);
        a_hitData->totalDamage = 0.0f;

        a_hitData->flags.set(HitData::Flag::kSneakAttack);


        _ProcessHitEvent(a_target, a_hitData);
    }


    void MainUpdateHook::Update(RE::Actor *a_this, float a_delta)
    {
        if (framesElapsed < 60)
        {
            framesElapsed += 1; 
            _Update(a_this, a_delta);
        }

        framesElapsed = 0;
        KnockoutHandler::UpdateTrackedActors(); 
        _Update(a_this, a_delta);
    }


    bool BleedoutStateHook::SetLifeState(Actor *a_actor, ACTOR_LIFE_STATE a_lifeState)
    {
        if (!a_actor || a_lifeState != ACTOR_LIFE_STATE::kEssentialDown || !Settings::GetTriggerEssentialDown() || a_actor->IsPlayerRef())
        {
            return _SetLifeState(a_actor, a_lifeState);
        }

        KnockoutHandler::ApplyUnconscious(a_actor);

        bool ret = _SetLifeState(a_actor, ACTOR_LIFE_STATE::kUnconcious);
        KnockoutHandler::TrackActor(a_actor);
        return ret;
    }


    bool UnconsciousFuncHook::DisableUnconscious(Actor *a_actor, bool a_enable)
    {
        bool ret = _DisableUnconscious(a_actor, a_enable);
        if (!a_actor) { return ret; }

        KnockoutHandler::RecoverUnconscious(a_actor); 
        KnockoutHandler::UntrackActor(a_actor);

        return ret;
    }
    bool UnconsciousFuncHook::EnableUnconscious(Actor *a_actor, bool a_enable)
    {

        bool ret = _EnableUnconscious(a_actor, a_enable);
        if (!a_actor) { return ret; }

        KnockoutHandler::ApplyUnconscious(a_actor);
        KnockoutHandler::TrackActor(a_actor);

        return ret;
    }
    void GetUpHook::InitiateGetUpPackage(Actor *actor)
    {
        auto* state = actor->AsActorState(); 
        if (true) { return; }

        _InitiateGetUpPackage(actor); 
    }
    bool PlayerActivateHook::ActivateRef(TESObjectREFR *a_ref, TESObjectREFR *a_activate_trigger, uint8_t a_arg2, TESBoundObject *a_object, int32_t a_count, bool a_defaultProcessingOnly)
    {
        if (auto* actor = a_ref->As<Actor>())
        {
            if (auto* actor_state = actor->AsActorState())
            {
                if (actor_state->GetLifeState() == ACTOR_LIFE_STATE::kUnconcious)
                {
                    SKSE::log::info("Interacting with {}", actor->GetName()); 
                }
            }
        }
        return _ActivateRef(a_ref, a_activate_trigger, a_arg2, a_object, a_count, a_defaultProcessingOnly); 
    }
}

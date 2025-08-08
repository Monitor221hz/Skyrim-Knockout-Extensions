#include "hook.h"
#pragma once

namespace KnockoutExtensions
{



    void HitEventHook::ProcessHitEvent(TESObjectREFR *a_target, HitData *a_hitData)
    {
        if (!a_target || !a_hitData || a_target->GetFormType() != FormType::ActorCharacter) { return _ProcessHitEvent(a_target, a_hitData); }

        auto* attacker = a_hitData->aggressor.get().get();
        auto* target_actor = a_target->As<Actor>();

        if (!attacker || target_actor->IsPlayerRef() || !KnockoutHandler::CanPassOut(target_actor)) { return _ProcessHitEvent(a_target, a_hitData); }
        
        auto* target_state = target_actor->AsActorState(); 
        if (target_state && target_state->GetLifeState() == ACTOR_LIFE_STATE::kUnconcious) 
        { 
            a_hitData->totalDamage = 0.f; 
            a_hitData->flags.set(HitData::Flag::kBlocked);
            a_hitData->skill = RE::ActorValue::kNone; 
            return; 
        } // don't react to hits while down

        if (Settings::GetTriggerBrawl() && (a_hitData->weapon != nullptr && a_hitData->weapon->GetWeaponType() == WEAPON_TYPE::kHandToHandMelee) && !a_hitData->flags.any(HitData::Flag::kPowerAttack) && a_hitData->flags.any(HitData::Flag::kFatal))
        {
            a_hitData->totalDamage = 0.0f;
            target_actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious); 
            KnockoutHandler::ApplyUnconscious(target_actor, attacker);
            KnockoutHandler::TrackActor(target_actor);
            return _ProcessHitEvent(a_target, a_hitData);
        }
        if (!a_hitData->flags.any(HitData::Flag::kBash)) { return _ProcessHitEvent(a_target, a_hitData); }
        auto* targetState = target_actor->AsActorState();
        bool inBleedout = targetState ? targetState->GetLifeState() == ACTOR_LIFE_STATE::kBleedout : false;

        if (target_actor->IsInCombat() && !inBleedout) { return _ProcessHitEvent(a_target, a_hitData); }
        
        bool arg2;
        bool hasLOS = target_actor->HasLineOfSight(attacker,arg2);


        if ((hasLOS || !Settings::GetTriggerBackBash()) && (!inBleedout || !Settings::GetTriggerBleedoutBash())) { return _ProcessHitEvent(a_target, a_hitData); }
        a_hitData->totalDamage = 0.0f;
        // a_hitData->flags.set(HitData::Flag::kSneakAttack);
        target_actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious); 

        KnockoutHandler::ApplyUnconscious(target_actor, attacker);
        KnockoutHandler::TrackActor(target_actor);
        
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
        a_actor->SetLifeState(ACTOR_LIFE_STATE::kAlive);
        KnockoutHandler::RecoverUnconscious(a_actor); 
        KnockoutHandler::UntrackActor(a_actor);

        return ret;
    }
    bool UnconsciousFuncHook::EnableUnconscious(Actor *a_actor, bool a_enable)
    {

        bool ret = _EnableUnconscious(a_actor, a_enable);
        if (!a_actor) { return ret; }
        a_actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious); 
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
                    if (actor->GetCrimeFaction())
                    {
                        OpenInventory(actor, 0); 
                        return true;
                    }
                    OpenInventory(actor, 1);
                    return true;
                }
            }
        }
        return _ActivateRef(a_ref, a_activate_trigger, a_arg2, a_object, a_count, a_defaultProcessingOnly); 
    }

    bool IsDeadHook::IsDead(Actor* actor, bool a_notEssential)
    {
        // using LS = ACTOR_LIFE_STATE; 
        // auto* actor_state = actor->AsActorState(); 
        // auto* actor_name = actor->GetName(); 
        // auto life_state = actor_state->GetLifeState(); 
        // if (a_notEssential)
        // {
        //     switch(life_state)
        //     {
        //         case LS::kRestrained:
        //             return false; 

        //         case LS::kDead:
        //         case LS::kUnconcious:
        //         case LS::kEssentialDown:
        //             return true; 

        //         default:
        //             break;
        //     }
        // }
        // else 
        // {
        //     switch(life_state)
        //     {
        //         case LS::kDying:
        //         case LS::kDead:
        //         case LS::kEssentialDown:
        //         // case LS::kUnconcious:
        //             return true; 
        //         default:
        //             break;
        //     }
        // }
        // return false;
        uint32_t v2; // er8
        long v4;          // eax
        v2 = (uint32_t)actor->AsActorState()->GetLifeState();  
        if (a_notEssential)
        {
            if (v2 == 3)
                return 1; 
            if (((v2 - 1) & 0xFFFFFFFA) == 0 && v2 != 6)
                return 1;
        }
        else
        if (v2 <= 7)
        {
            v4 = 166;
            if (_bittest(&v4, v2))
                return 1;
        }
        return 0;
    }
    bool RagdollStateHook::IsInRagdollState(Actor *a_actor)
    {
        // if (auto* controller = a_actor->GetCharController())
        // {
        //     return controller->flags.any(CHARACTER_FLAGS::kFollowRagdoll);  
        // }
        
        auto* actor_state = a_actor->AsActorState(); 
        return actor_state->GetLifeState() == ACTOR_LIFE_STATE::kUnconcious && actor_state->actorState2.reanimating == 0 && (actor_state->GetKnockState() == KNOCK_STATE_ENUM::kDown || actor_state->GetKnockState() == KNOCK_STATE_ENUM::kOut); 
    }
    void ConcussionStateHook::Start(ConcussionEffect *a_effect)
    {
        auto* refr = a_effect->target->GetTargetStatsObject();
        if (!refr)
        {
            return;
        }
        auto* target_actor = refr->As<Actor>(); 

        auto* caster = a_effect->GetCasterActor().get(); 
        if (!target_actor) { return; }
        target_actor->SetLifeState(ACTOR_LIFE_STATE::kUnconcious); 
        SKSE::GetTaskInterface()->AddTask(
            [target_actor, caster](
            )
            {
                KnockoutHandler::ApplyUnconscious(target_actor, caster);
            }
        ); 

        _Start(a_effect); 
    }
    void ConcussionStateHook::Finish(ConcussionEffect *a_effect)
    {
        auto* refr = a_effect->target->GetTargetStatsObject();
        if (!refr)
        {
            return;
        }
        auto* target_actor = refr->As<Actor>(); 
        if (!target_actor) { return; }
        target_actor->SetLifeState(ACTOR_LIFE_STATE::kAlive);
        SKSE::GetTaskInterface()->AddTask(
            [target_actor](
            )
            {
                KnockoutHandler::RecoverUnconscious(target_actor);
            }
        );
        _Finish(a_effect); 
    }
    /*
            enum class ACTOR_LIFE_STATE : std::uint32_t
            {
                kAlive = 0, 0000 -> 0000
                kDying = 1, 0001 -> 0001
                kDead = 2, 0010 -> 0010
                kUnconcious = 3, 0011 -> 0010
                kReanimate = 4, 0100 -> 0000
                kRecycle = 5, 0101 -> 0000
                kRestrained = 6, 0110 -> 0010
                kEssentialDown = 7, 0111 -> 0010
                kBleedout = 8 1000 -> 1000
            };

            10101110
            enum class ACTOR_LIFE_STATE : std::uint32_t
            {
                kAlive = 0,
                kDying = 1, x
                kDead = 2,x
                kUnconcious = 3, x
                kReanimate = 4,
                kRecycle = 5, x
                kRestrained = 6,
                kEssentialDown = 7, x
                kBleedout = 8
            };


        */
    bool ConcussionStateHook::CanFinish(ConcussionEffect *a_effect)
    {
        auto* calendar = Calendar::GetSingleton(); 
        float timescale = calendar ? calendar->GetTimescale() : 20.f; 
        return a_effect->elapsedSeconds * timescale > a_effect->duration; 
    }
}

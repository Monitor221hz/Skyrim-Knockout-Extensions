#pragma once 

#include "settings.h"
#include "util.h"
#include "knockout.h"
using namespace RE; 
namespace KnockoutExtensions
{
class UnconsciousFuncHook
{
    public: 

    static void Install()
    {
        auto& trampoline = SKSE::GetTrampoline(); 

        SKSE::AllocTrampoline(32);

        //SE GameFunc__handler__SetUnconscious_1402FFAF0+C8	call    SetUnconscious_1405E32A0
        //AE sub_140314500+C8	call    sub_140608F50
        REL::Relocation<std::uintptr_t> enableTarget{ REL::RelocationID(21874, 22356) , REL::Relocate(0xC8, 0xC8) };

        //SE GameFunc__handler__SetUnconscious_1402FFAF0+120	call    SetUnconscious_1405E32A0
        //AE sub_140314500+120	call    sub_140608F50
        REL::Relocation<std::uintptr_t> disableTarget{ REL::RelocationID(21874,22356), REL::Relocate(0x120, 0x120) }; 

        _EnableUnconscious = trampoline.write_call<5>(enableTarget.address(), EnableUnconscious);
        _DisableUnconscious = trampoline.write_call<5>(disableTarget.address(), DisableUnconscious); 

        SKSE::log::info("Unconscious GameFunc Hooks Installed");
    }
    private: 

    static bool DisableUnconscious(Actor* a_actor, bool a_enable); 
    static bool EnableUnconscious(Actor* a_actor, bool a_enable);

    static inline REL::Relocation<decltype(EnableUnconscious)> _EnableUnconscious; 
    static inline REL::Relocation<decltype(DisableUnconscious)> _DisableUnconscious; 
    //	p	Actor__KillImpl_140603B30+6C4	call    Actor__SetLifeState_1405EDEF0
    //>(REL::RelocationID(37673, 38627))
};
class BleedoutStateHook
{
    public: 

    static void Install()
    {
            //SE Actor__KillImpl_140603B30+6C4	call    Actor__SetLifeState_1405EDEF0
            //AE sub_14062B1E0+F78	call    sub_140614650
        auto& trampoline = SKSE::GetTrampoline(); 

        SKSE::AllocTrampoline(16);

        REL::Relocation<std::uintptr_t> target{ REL::RelocationID(36872, 37896), REL::Relocate(0x6C4, 0xF78)} ; 
        _SetLifeState = trampoline.write_call<5>(target.address(), SetLifeState);
    }
    private: 
    static bool SetLifeState(Actor* a_actor, ACTOR_LIFE_STATE a_lifeState); 
    static inline REL::Relocation<decltype(SetLifeState)> _SetLifeState;


};
class HitEventHook
{
    public: 
    static void Install()
    {
        auto& trampoline = SKSE::GetTrampoline(); 

        SKSE::AllocTrampoline(16);
        REL::Relocation<std::uintptr_t> target { REL::RelocationID(37673, 38627), REL::Relocate(0x3C0, 0x4A8) };

        _ProcessHitEvent = trampoline.write_call<5>(target.address(), ProcessHitEvent);

        SKSE::log::info("Hit Event Hook Installed");
    }
    private: 
    static void ProcessHitEvent(TESObjectREFR *a_target, HitData *a_hitData);
    static inline REL::Relocation<decltype(ProcessHitEvent)> _ProcessHitEvent;
};

class MainUpdateHook
{
    public: 

    static void Install()
    {

        REL::Relocation<std::uintptr_t> PlayerVtbl{RE::VTABLE_PlayerCharacter[0]};
        _Update = PlayerVtbl.write_vfunc(0xAD, Update);

        SKSE::log::info("Hook - Main Update Installed");
    }

     private:
        static void Update(RE::Actor *a_this, float a_delta);
        static inline REL::Relocation<decltype(Update)> _Update;
        
        static inline uint32_t framesElapsed = 0; 
        static inline uint32_t secondsElapsed = 0; 
};
class GetUpHook
{
    public:

    static void Install()
    {
        REL::Relocation<std::uintptr_t> ActorVtbl { RE::VTABLE_Character[0] }; 
        _InitiateGetUpPackage = ActorVtbl.write_vfunc(0xDE, InitiateGetUpPackage); 

        SKSE::log::info("Hook - Get up Installed"); 
    }   

    private:
        static void InitiateGetUpPackage(Actor* actor); 
        static inline REL::Relocation<decltype(InitiateGetUpPackage)> _InitiateGetUpPackage; 
}; 

class PlayerActivateHook
{
    public:

        static void Install()
        {
            auto& trampoline = SKSE::GetTrampoline(); 
            SKSE::AllocTrampoline(14);
            REL::Relocation<std::uintptr_t> target{ REL::RelocationID( 39471, 40548 ), REL::Relocate(0x135, 0x10D)}; 
            _ActivateRef = trampoline.write_call<5>(target.address(), ActivateRef);

            SKSE::log::info("Activate Hook installed");
        }
    private:
        static bool ActivateRef(TESObjectREFR* a_ref, TESObjectREFR* a_activate_trigger, uint8_t a_arg2, TESBoundObject* a_object, int32_t a_count, bool a_defaultProcessingOnly);
        static inline REL::Relocation<decltype(ActivateRef)> _ActivateRef; 
        static void OpenInventory(TESObjectREFR* a_ref, uint32_t openType)
        {
            using func_t = decltype(OpenInventory);
            REL::Relocation<func_t> func{RELOCATION_ID(50211, 50849)};
            return func(a_ref, openType);
        }

};
class IsDeadHook
{
    public: 
    static void Install()
    {
        // REL::Relocation<std::uintptr_t> ActorVtbl { VTABLE_Actor[0] }; 
        // _IsDead = ActorVtbl.write_vfunc(0x99, IsDead); 
        auto& trampoline = SKSE::GetTrampoline(); 
        SKSE::AllocTrampoline(14);
        REL::Relocation<std::uintptr_t> target{ REL::RelocationID( 36484, 35638 ) }; 
        trampoline.write_branch<5>(target.address(), IsDead); 

        SKSE::log::info("GetDead installed");
    }
    private:
    static bool IsDead(Actor* actor, bool a_notEssential); 
    // static inline REL::Relocation<decltype(IsDead)> _IsDead; 
};

class RagdollStateHook
{
    public:
    static void Install()
    {
        auto& trampoline = SKSE::GetTrampoline(); 
        SKSE::AllocTrampoline(14); 
        REL::Relocation<std::uintptr_t> target { REL::RelocationID(36492, 0) }; 
        trampoline.write_branch<5>(target.address(), IsInRagdollState);
    }
    private:
    static bool IsInRagdollState(Actor* a_actor); 
};

class ConcussionStateHook
{
    public:
    static void Install()
    {
        REL::Relocation<std::uintptr_t> ConcEffectVtbl { RE::VTABLE_ConcussionEffect[0] }; 
        // _InitiateGetUpPackage = ActorVtbl.write_vfunc(0xDE, InitiateGetUpPackage); 
        _Start = ConcEffectVtbl.write_vfunc(0x14, Start); 
        _Finish = ConcEffectVtbl.write_vfunc(0x15, Finish); 
        _CanFinish = ConcEffectVtbl.write_vfunc(0x16, CanFinish); 
        SKSE::log::info("Hook - Concussion State Start/Finish Installed"); 
    }
    private:
    static void Start(ConcussionEffect* a_effect);  // 14
    static void Finish(ConcussionEffect* a_effect); // 15
    static bool CanFinish(ConcussionEffect* a_effect); //16

    static inline REL::Relocation<decltype(Start)> _Start;
    static inline REL::Relocation<decltype(Finish)> _Finish; 
    static inline REL::Relocation<decltype(CanFinish)> _CanFinish;





}; 
}



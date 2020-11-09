#include "AGameRules.h"

AGameRules* g_pGameRules = NULL;
AGameRules::AGameRules() : ABaseEntity() {
    g_pGameRules                  = this;
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(true);
}

void AGameRules::Tick(float deltaTime) {
    Super::Tick(deltaTime);

    // check for round restart
    // also check for initialization of all entities (efficient place to do it)
    if (g_pGlobals->curtime > m_tNextRoundRestart) {
        ADebug::Log("Executing round restart statement");
        ADebug::Log("ReadyEntityCount = %i/%i\n", s_iReadyEntityCount,
                    s_iEntityCount);
        if (!m_bHasInitializedAllEntities && AllEntitiesReady()) {
            ADebug::Log("Initializing all entities");
            InitializeAllEntities();
        } else if (!AllEntitiesReady()) {
            //?! This should never happen!
            ADebug::LogError(
                "OUT OF SYNC - s_iReadyEntityCount != s_iEntityCount !!!");
        } else {
            ADebug::Log("Restarting round");
            RestartRound();
        }
        m_tNextRoundRestart = FLT_MAX;
    }

    // execute all default thinks
    for (eindex i = 0; i < g_entList.Num(); i++) {
        // ADebug::Log(L"Executing think for actor %s",
        // WCStr(g_entList[i]->GetActor()->GetName()));
        g_entList[i]->DefaultThink();
    }

    // execute all the pointer-based thinks
    for (eindex i = 0; i < g_entList.Num(); i++) {
        if (g_entList[i]->GetNextThink() > g_pGlobals->curtime) {
            g_entList[i]->Think();
        }
    }

    // update globals
    g_pGlobals->update();
}

UWorld* g_pWorld = NULL;
void    AGameRules::BeginPlay() {
    Super::BeginPlay();
    ADebug::Log("AGameRules::BeginPlay\n");
    ADebug::Assert((bool)(s_iEntityCount = g_entList.Num()),
                   "\nIBaseEntity::s_iEntityCount = g_entList.Num()");

    m_bHasRestartedRound         = false;
    m_bHasInitializedAllEntities = false;

    m_tNextRoundRestart = -FLT_MAX;

    g_pWorld = GetWorld();
}

void AGameRules::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    ADebug::Log(__FUNCTION__);
    m_bHasInitializedAllEntities = false;
    g_pGlobals->markReset();
}

void AGameRules::RestartRound() {
    ADebug::Log(__FUNCTION__);

    // we need to do this right, otherwise when we iterate through
    // the entity list, the size of the list might change
    TArray<EHANDLE> m_aDestroyedEntities;
    // this array remembers which entities we'll destroy later

    for (int i = 0; i < g_entList.Num(); i++) {
        IBaseEntity* pEnt = g_entList[i];

        // ignore entities marked with preserve
        if (!pEnt->HasFlags(FL_ROUND_PRESERVE)) {

            // remember entities which need destruction
            if (pEnt->HasFlags(FL_ROUND_DESTROY)) {
                m_aDestroyedEntities.Add(pEnt->GetEHandle());
                continue;
            }
        }
    }

    // now destroy everything marked in the list
    while (m_aDestroyedEntities.Num() > 0) {
        EHANDLE ent = m_aDestroyedEntities[0];
        ent->DestroyEntity();
        m_aDestroyedEntities.RemoveAt(0);
    }
}

void AGameRules::InitializeAllEntities() {
    ADebug::Log(__FUNCTION__);
    // run pre inits of all entities
    ADebug::Log("Running all pre-inits");
    for (eindex i = 0; i < g_entList.Num(); i++) {
        g_entList[i]->SetNextThink(FLT_MAX);
        g_entList[i]->PreInit();
    }

    // run post inits of all entities
    ADebug::Log("Running all post-inits");
    for (eindex i = 0; i < g_entList.Num(); i++)
        g_entList[i]->PostInit();

    // mark as ready
    m_bHasInitializedAllEntities = true;
}
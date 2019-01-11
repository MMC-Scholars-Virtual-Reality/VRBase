#include "AGameRules.h"
#include "IGameRulesSystem/IGameRulesSystem.h"
#include "FMovingVector/FMovingVector.h"

AGameRules* g_pGameRules = NULL;
AGameRules::AGameRules() : ABaseEntity() {
	g_pGameRules = this;
	//g_pGlobals->reset();
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);
}

void AGameRules::Tick(float deltaTime) {
	Super::Tick(deltaTime);

	//check for round restart
	//here we also check for initialization of all entities, efficient place to do it
	if (g_pGlobals->curtime > m_tNextRoundRestart) {
		Msg("Executing round restart statement");
		Msg("ReadyEntityCount = %i/%i\n", s_iReadyEntityCount, s_iEntityCount);
		if (!m_bHasInitializedAllEntities && AllEntitiesReady()) {
			Msg("Initializing all entities");
			InitializeAllEntities();
		}
		else if (!AllEntitiesReady()) {
			//?! This should never happen!
			NLogger::Fatal("OUT OF SYNC - s_iReadyEntityCount != s_iEntityCount !!!");
		}
		else {
			NLogger::Message(FColor::Cyan, 3.0f, "Restarting round");
			RestartRound();
		}
		m_tNextRoundRestart = FLT_MAX;
	}

	//Execute all default thinks
	for (eindex i = 0; i < g_entList.Num(); i++) {
		//Msg(L"Executing think for actor %s", WCStr(g_entList[i]->GetActor()->GetName()));
		g_entList[i]->DefaultThink();
	}

	//Execute all the pointer-based thinks
	for (eindex i = 0; i < g_entList.Num(); i++) {
		g_entList[i]->Think();
	}

	//Call the IGameRulesSystem thinks
	for (int32 i = 0; i < g_aGameRulesSystems.Num(); i++) {
		if (g_aGameRulesSystems[i]->GetNextThinkTime() < g_pGlobals->curtime) {
			g_aGameRulesSystems[i]->Think();
		}
	}

	//Update globals
	g_pGlobals->update();
}

void AGameRules::BeginPlay() {
	BaseClass::BeginPlay();
	Msg("AGameRules::BeginPlay\n");
	if (s_iEntityCount != g_entList.Num()) {
		NLogger::Fatal("\nIBaseEntity::s_iEntityCount != g_entList.Num()");
	}
	
	m_bHasRestartedRound = false;
	m_bHasInitializedAllEntities = false;


	m_tNextRoundRestart = -FLT_MAX;
}

void AGameRules::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Msg(__FUNCTION__);
	m_bHasInitializedAllEntities = false;
	g_pGlobals->markReset();
}

void AGameRules::RestartRound() {
	Msg(__FUNCTION__);

	//we need to do this right, otherwise when we iterate through
	//the entity list, the size of the list might change
	TArray<EHANDLE> m_aDestroyedEntities;
	//this array remembers which entities we'll destroy later

	for (int i = 0; i < g_entList.Num(); i++) {
		IBaseEntity* pEnt = g_entList[i];

		//ignore entities marked with preserve
		if (!pEnt->HasFlags(FL_ROUND_PRESERVE)) {

			//remember entities which need destruction
			if (pEnt->HasFlags(FL_ROUND_DESTROY)) {
				m_aDestroyedEntities.Add(pEnt->GetEHandle());
				continue;
			}
			else
				pEnt->Respawn();
		}
	}

	//now destroy everything marked in the list
	while (m_aDestroyedEntities.Num() > 0) {
		EHANDLE ent = m_aDestroyedEntities[0];
		ent->DestroyEntity();
		m_aDestroyedEntities.RemoveAt(0);
	}
}

void AGameRules::InitializeAllEntities() {
	Msg(__FUNCTION__);
	//run pre inits of all entities
	Msg("Running all pre-inits");
	for (eindex i = 0; i < g_entList.Num(); i++) {
		g_entList[i]->SetNextThink(FLT_MAX);
		g_entList[i]->PreInit();
	}

	//call independent game system PreInit
	for (int32 i = 0; i < g_aGameRulesSystems.Num(); i++) {
		g_aGameRulesSystems[i]->PreInit();
	}

	//run global initializers in order
	Msg("Running all static initializers");
	CStaticInitializer::InvokeAllInOrder();

	//run post inits of all entities
	Msg("Running all post-inits");
	for (eindex i = 0; i < g_entList.Num(); i++)
		g_entList[i]->PostInit();

	//call independent game system PostInit
	for (int32 i = 0; i < g_aGameRulesSystems.Num(); i++) {
		g_aGameRulesSystems[i]->PostInit();
	}

	//mark us as ready
	m_bHasInitializedAllEntities = true;
}
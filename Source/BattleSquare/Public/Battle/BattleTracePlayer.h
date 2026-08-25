// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleEvent.h"
#include "BattleTracePlayer.generated.h"

// T6–T7 (tasks.md, PRES-09/PRES-10, BTL-22): consome o trace linear do
// núcleo, agrupa por fase para simultaneidade, e orquestra a animação.
// Nenhum cálculo aqui — só leitura de campos de FBattleEvent.

// Agrupa eventos consecutivos com o MESMO (SlotIndex, Phase) — eventos
// da mesma fase tocam juntos (PRES-09, critério 2); fases diferentes,
// mesmo do mesmo slot, ficam em grupos separados e tocam em sequência.
// Função pura: não modifica Trace, não depende de nada além dele.
TArray<TArray<FBattleEvent>> GroupBattleEventsByPhase(const TArray<FBattleEvent>& Trace);

// Multicast simples (não dinâmico): permite binding por lambda, usado nos
// testes headless e por APetView (T8) via AddRaw/AddLambda. Não precisa
// ser BlueprintAssignable — nada aqui é exposto a Blueprint/UMG (T7 é
// lógica pura, camada visual fica para T11/T12).
DECLARE_MULTICAST_DELEGATE_OneParam(FBattleEventAppliedSignature, const FBattleEvent&);

// Orquestra a reprodução de um trace: despacha cada evento, em ordem de
// grupo, para quem estiver ouvindo (APetView, T8) via delegate — nunca
// chama diretamente uma classe de apresentação concreta, para não criar
// dependência circular antes de APetView existir (T7 vem antes de T8 no
// plano de execução).
UCLASS()
class BATTLESQUARE_API UBattleTracePlayer : public UObject
{
	GENERATED_BODY()

public:
	FBattleEventAppliedSignature OnEventApplied;

	// Não exposto ao Blueprint: TArray<FBattleEvent> usa um USTRUCT sem
	// BlueprintType (FBattleEvent não precisa ser editável/visível em
	// Blueprint — mesmo racional de FTurnCommit).
	// Despacha os grupos em ordem, evento a evento, dentro de cada grupo.
	void PlayTrace(const TArray<FBattleEvent>& Trace);

	// Aplica o trace inteiro imediatamente — usado quando o jogador pula
	// a animação. Não exige que grupos anteriores já tenham "tocado".
	void SkipToEnd(const TArray<FBattleEvent>& Trace);
};

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BattleEvent.generated.h"

// Todo o vocabulário de eventos que a resolução de um turno pode emitir.
// A apresentação (camada BattleSquare) anima este trace e NUNCA recalcula
// nada — ver design.md, BTL-22: se a animação precisa de um número, esse
// número veio daqui.
UENUM()
enum class EBattleEventType : uint8
{
	TurnoIniciado = 0,
	SlotIniciado,
	PosturaAssumida,
	Moveu,
	MovimentoBloqueado,
	AtaqueAcertou,
	AtaqueErrou,
	Esquivou,
	Defendeu,
	DanoAplicado,
	PetMorreu,
	SlotEncerrado,
	TurnoEncerrado,
	BatalhaEncerrada,

	// DP-02: dois pets chegaram ao mesmo ponto do campo. Acrescentado ao FIM
	// do enum de propósito — os valores existentes entram no hash do traço, e
	// inserir no meio invalidaria todo snapshot de determinismo já gravado.
	EncontroNoMesmoPonto,

	// Skill tentada onde o terreno não permite (submergir fora d'água).
	// Também no fim, pelo mesmo motivo do anterior.
	PosturaFalhou,

	// Um golpe mudou a casa que acertou (fatia 4). No FIM do enum, pelo mesmo
	// motivo dos anteriores: os valores existentes entram no hash do traço.
	TerrenoMudou,

	// Esquiva que o pet NÃO declarou: saiu do reflexo dele (fatia 4). Evento
	// próprio, e não `Esquivou` reaproveitado, porque a diferença é o que o
	// jogador precisa entender — uma foi decisão dele, a outra foi atributo.
	// No FIM do enum, pelo mesmo motivo dos anteriores.
	EsquivouPorReflexo,

	// Magia de atributo (escola psíquica). `Detail` diz qual atributo,
	// `Value` diz quanto — positivo em si, negativo no oponente. No FIM do
	// enum, pelo mesmo motivo dos anteriores: os valores existentes entram no
	// hash do traço.
	AtributoAlterado,
	AtributoVoltouAoNormal,

	// A casa bloqueada tem corpo: quem tem força derruba o obstáculo, quem
	// tem agilidade sobe nele. Dois eventos, e não um `TerrenoMudou` para o
	// primeiro e um `Moveu` para o segundo, porque é a diferença entre os
	// dois que o jogador precisa entender — e porque `Moveu` sozinho não
	// contaria que aquele pet agora luta de cima.
	//
	// No FIM do enum, pelo mesmo motivo dos anteriores: os valores existentes
	// entram no hash do traço.
	ObstaculoDerrubado,
	SubiuNoObstaculo,

	// Terreno temporário chegou ao fim e a casa voltou ao que era — o gelo
	// derretendo. Evento PRÓPRIO, e não um segundo `TerrenoMudou`, porque a
	// diferença é o que o jogador precisa entender: uma casa mudou porque
	// alguém a atingiu, a outra mudou porque o tempo passou, e ninguém tem
	// o que comemorar na segunda.
	//
	// `Value` carrega o terreno que a casa voltou a ser; `ActorId` é vazio,
	// porque derreter não tem autor.
	//
	// No FIM do enum, pelo mesmo motivo dos anteriores: os valores existentes
	// entram no hash do traço.
	TerrenoDerreteu
};

// Sentinela para TargetId/ActorId quando não há alvo aplicável
// (ex.: TurnoIniciado, MovimentoBloqueado sem interação).
inline constexpr uint8 BattleEventNoActor = 0xFF;

// Struct POD achatado — sem ponteiro, sem virtual, sem FString, sem
// TArray interno. Ver design.md: replicação nativa lida bem com struct
// plano; polimorfismo abriria espaço para divergência entre cliente e
// servidor.
USTRUCT()
struct FBattleEvent
{
	GENERATED_BODY()

	UPROPERTY()
	EBattleEventType Type = EBattleEventType::TurnoIniciado;

	UPROPERTY()
	uint8 SlotIndex = 0;

	UPROPERTY()
	uint8 Phase = 0;

	UPROPERTY()
	uint8 ActorId = BattleEventNoActor;

	UPROPERTY()
	uint8 TargetId = BattleEventNoActor;

	UPROPERTY()
	uint8 FromCell = 0; // empacotado via PackCell (BattleTypes.h)

	UPROPERTY()
	uint8 ToCell = 0;

	// Genérico de propósito: dano, cura, ou qualquer número que a
	// animação precise ler. O custo de um campo genérico é baixo perto
	// do que se evita ao não ter hierarquia de eventos.
	UPROPERTY()
	int32 Value = 0;

	/**
	 * Segundo genérico, pelo mesmo motivo do primeiro.
	 *
	 * Existe porque a magia de atributo precisa dizer DUAS coisas — qual
	 * atributo e quanto — e empacotar as duas dentro de `Value` seria o tipo
	 * de aritmética que ninguém lembra de desfazer seis meses depois.
	 */
	UPROPERTY()
	uint8 Detail = 0;
};

static_assert(sizeof(EBattleEventType) == 1, "EBattleEventType deve ocupar 1 byte — struct de evento precisa ficar compacto para o trace inteiro replicar barato.");

// Teto de tamanho do struct — T4 (tasks.md). Não é um valor exato porque
// padding de alinhamento pode variar por compilador/plataforma; o que
// importa é que o evento continue compacto o bastante para replicar um
// trace inteiro sem pesar na rede.
static_assert(sizeof(FBattleEvent) <= 16, "FBattleEvent cresceu além do esperado — reveja se algum campo pode ser mais estreito.");

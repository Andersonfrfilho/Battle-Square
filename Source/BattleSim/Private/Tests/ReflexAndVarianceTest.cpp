// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeReflexDuel(int32 EsquivaDoAlvoPercent, int32 VariacaoDoAtacantePercent, uint64 Semente)
	{
		FBattleState Estado;

		FPetState Atacante;
		Atacante.PetId = 1;
		Atacante.Side = 0;
		Atacante.Column = 0;
		Atacante.Row = 1;
		Atacante.Health = 200;
		Atacante.MaxHealth = 200;
		Atacante.Attack = 100;
		Atacante.Defense = 10;
		Atacante.DamageVariancePercent = VariacaoDoAtacantePercent;

		FPetState Alvo;
		Alvo.PetId = 2;
		Alvo.Side = 1;
		Alvo.Column = 1;
		Alvo.Row = 1;
		Alvo.Health = 200;
		Alvo.MaxHealth = 200;
		Alvo.Attack = 10;
		Alvo.Defense = 10;
		Alvo.ReflexDodgePercent = EsquivaDoAlvoPercent;

		Estado.Pets.Add(Atacante);
		Estado.Pets.Add(Alvo);
		Estado.Random.State = Semente;
		return Estado;
	}

	FTurnCommit AtaqueSimples()
	{
		FTurnCommit Commit;
		Commit.Actions[0].Type = EActionType::Atacar;
		return Commit;
	}

	FTurnCommit AguardarTresVezes()
	{
		FTurnCommit Commit;
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			Commit.Actions[Slot].Type = EActionType::Aguardar;
		}
		return Commit;
	}

	int32 ContarEventos(const TArray<FBattleEvent>& Trace, EBattleEventType Tipo)
	{
		int32 Total = 0;
		for (const FBattleEvent& Evento : Trace)
		{
			if (Evento.Type == Tipo)
			{
				++Total;
			}
		}
		return Total;
	}
}

// Reflexo ZERO nunca esquiva sozinho — é o comportamento de antes desta
// fatia, e nenhuma batalha existente pode mudar por ela ter chegado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZeroReflexNeverDodgesByItselfTest,
	"BattleSim.Attributes.ZeroReflexNeverDodgesByItself",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZeroReflexNeverDodgesByItselfTest::RunTest(const FString& Parameters)
{
	int32 Reflexos = 0;
	for (uint64 Semente = 1; Semente <= 200; ++Semente)
	{
		FBattleState Estado = MakeReflexDuel(/*Esquiva=*/0, /*Variacao=*/0, Semente);
		const FBattleResolveResult Resultado =
			FBattleResolver::ResolveTurn(Estado,
		FBattleResolver::DuelCommits(Estado, AtaqueSimples(), AguardarTresVezes()));

		Reflexos += ContarEventos(Resultado.Trace, EBattleEventType::EsquivouPorReflexo);
	}

	TestEqual(TEXT("Nenhuma esquiva por reflexo em 200 sementes"), Reflexos, 0);
	return true;
}

// O TETO é a amarra do DP-atr-07: sem ele, atributo alto viraria imunidade, e
// o número passaria a decidir mais que a decisão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReflexDodgeIsCappedTest,
	"BattleSim.Attributes.ReflexDodgeIsCapped",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FReflexDodgeIsCappedTest::RunTest(const FString& Parameters)
{
	constexpr int32 Amostras = 400;
	int32 Reflexos = 0;

	// Chance absurda: se não houvesse recorte, seriam 250%.
	for (uint64 Semente = 1; Semente <= Amostras; ++Semente)
	{
		FBattleState Estado = MakeReflexDuel(/*Esquiva=*/250, /*Variacao=*/0, Semente);
		const FBattleResolveResult Resultado =
			FBattleResolver::ResolveTurn(Estado,
		FBattleResolver::DuelCommits(Estado, AtaqueSimples(), AguardarTresVezes()));

		Reflexos += ContarEventos(Resultado.Trace, EBattleEventType::EsquivouPorReflexo);
	}

	// Um ataque por turno, então a proporção de esquivas é a chance efetiva.
	// Faixa larga de propósito: o teste protege o TETO, não uma estatística —
	// apertar a faixa faria dele um teste de gerador, que é outro assunto e já
	// tem o seu.
	TestTrue(TEXT("Esquiva por reflexo acontece"), Reflexos > 0);
	TestTrue(
		*FString::Printf(TEXT("Fica abaixo do dobro do teto (%d de %d)"), Reflexos, Amostras),
		Reflexos < (Amostras * BattleArenaConstants::ReflexDodgeMaxPercent * 2) / 100);

	return true;
}

// A esquiva por reflexo é contra FÍSICO. Magia já ignora a esquiva declarada
// (BTL-10), e ignorar as duas tornaria magia obrigatória.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReflexDodgeDoesNotStopMagicTest,
	"BattleSim.Attributes.ReflexDodgeDoesNotStopMagic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FReflexDodgeDoesNotStopMagicTest::RunTest(const FString& Parameters)
{
	int32 Reflexos = 0;
	for (uint64 Semente = 1; Semente <= 200; ++Semente)
	{
		FBattleState Estado = MakeReflexDuel(/*Esquiva=*/250, /*Variacao=*/0, Semente);

		FTurnCommit Magia;
		Magia.Actions[0].Type = EActionType::Magia;

		const FBattleResolveResult Resultado =
			FBattleResolver::ResolveTurn(Estado,
		FBattleResolver::DuelCommits(Estado, Magia, AguardarTresVezes()));

		Reflexos += ContarEventos(Resultado.Trace, EBattleEventType::EsquivouPorReflexo);
	}

	TestEqual(TEXT("Magia nunca é desviada por reflexo"), Reflexos, 0);
	return true;
}

// A agressividade ESTREITA a variação; nunca aumenta o dano médio.
//
// É a diferença entre "confiável" e "mais forte". Um bônus de dano disfarçado
// de constância faria a personalidade ser só uma segunda musculatura.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAggressionNarrowsVarianceWithoutRaisingDamageTest,
	"BattleSim.Attributes.AggressionNarrowsVarianceWithoutRaisingDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAggressionNarrowsVarianceWithoutRaisingDamageTest::RunTest(const FString& Parameters)
{
	auto MedirDanos = [](int32 Variacao, int32& OutMinimo, int32& OutMaximo, int64& OutSoma)
	{
		OutMinimo = TNumericLimits<int32>::Max();
		OutMaximo = TNumericLimits<int32>::Min();
		OutSoma = 0;
		int32 Amostras = 0;

		for (uint64 Semente = 1; Semente <= 300; ++Semente)
		{
			FBattleState Estado = MakeReflexDuel(/*Esquiva=*/0, Variacao, Semente);
			const FBattleResolveResult Resultado =
				FBattleResolver::ResolveTurn(Estado,
		FBattleResolver::DuelCommits(Estado, AtaqueSimples(), AguardarTresVezes()));

			for (const FBattleEvent& Evento : Resultado.Trace)
			{
				if (Evento.Type != EBattleEventType::AtaqueAcertou)
				{
					continue;
				}
				OutMinimo = FMath::Min(OutMinimo, Evento.Value);
				OutMaximo = FMath::Max(OutMaximo, Evento.Value);
				OutSoma += Evento.Value;
				++Amostras;
			}
		}

		return Amostras;
	};

	int32 MinimoCauteloso = 0, MaximoCauteloso = 0;
	int64 SomaCauteloso = 0;
	const int32 AmostrasCauteloso = MedirDanos(
		BattleArenaConstants::DamageVarianceBasePercent,
		MinimoCauteloso, MaximoCauteloso, SomaCauteloso);

	int32 MinimoAgressivo = 0, MaximoAgressivo = 0;
	int64 SomaAgressivo = 0;
	const int32 AmostrasAgressivo = MedirDanos(
		BattleArenaConstants::DamageVarianceFloorPercent,
		MinimoAgressivo, MaximoAgressivo, SomaAgressivo);

	TestTrue(TEXT("Houve acertos nos dois grupos"),
		AmostrasCauteloso > 0 && AmostrasAgressivo > 0);

	TestTrue(
		*FString::Printf(TEXT("A faixa do agressivo é MENOR (%d..%d contra %d..%d)"),
			MinimoAgressivo, MaximoAgressivo, MinimoCauteloso, MaximoCauteloso),
		(MaximoAgressivo - MinimoAgressivo) < (MaximoCauteloso - MinimoCauteloso));

	// A média fica perto: a variação é simétrica, então estreitá-la não
	// desloca o centro. Tolerância de 10% para o sorteio, não para um viés.
	const int64 MediaCauteloso = SomaCauteloso / AmostrasCauteloso;
	const int64 MediaAgressivo = SomaAgressivo / AmostrasAgressivo;
	TestTrue(
		*FString::Printf(TEXT("A média não sobe com agressividade (%lld contra %lld)"),
			MediaAgressivo, MediaCauteloso),
		FMath::Abs(MediaAgressivo - MediaCauteloso) <= MediaCauteloso / 10);

	return true;
}

// O acaso NÃO quebra o determinismo: mesma semente, mesmo resultado.
//
// É a amarra do DP-atr-06, e a mais cara de descobrir tarde — numa partida em
// rede, os dois lados resolveriam diferente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRandomnessStaysDeterministicTest,
	"BattleSim.Attributes.RandomnessStaysDeterministic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRandomnessStaysDeterministicTest::RunTest(const FString& Parameters)
{
	FBattleState Primeira = MakeReflexDuel(/*Esquiva=*/15, /*Variacao=*/8, /*Semente=*/987654321);
	FBattleState Segunda = MakeReflexDuel(/*Esquiva=*/15, /*Variacao=*/8, /*Semente=*/987654321);

	const FBattleResolveResult ResultadoA =
		FBattleResolver::ResolveTurn(Primeira,
		FBattleResolver::DuelCommits(Primeira, AtaqueSimples(), AguardarTresVezes()));
	const FBattleResolveResult ResultadoB =
		FBattleResolver::ResolveTurn(Segunda,
		FBattleResolver::DuelCommits(Segunda, AtaqueSimples(), AguardarTresVezes()));

	TestEqual(TEXT("Mesma semente, mesma assinatura de estado"),
		ResultadoA.NextState.ComputeHash(), ResultadoB.NextState.ComputeHash());
	TestEqual(TEXT("Mesma semente, mesmo número de eventos"),
		ResultadoA.Trace.Num(), ResultadoB.Trace.Num());

	// Semente diferente PRECISA divergir, senão o teste acima passaria com o
	// acaso desligado e ninguém notaria.
	FBattleState Outra = MakeReflexDuel(/*Esquiva=*/15, /*Variacao=*/8, /*Semente=*/123456789);
	const FBattleResolveResult ResultadoC =
		FBattleResolver::ResolveTurn(Outra,
		FBattleResolver::DuelCommits(Outra, AtaqueSimples(), AguardarTresVezes()));

	TestNotEqual(TEXT("Semente diferente muda o resultado"),
		ResultadoC.NextState.ComputeHash(), ResultadoA.NextState.ComputeHash());

	return true;
}

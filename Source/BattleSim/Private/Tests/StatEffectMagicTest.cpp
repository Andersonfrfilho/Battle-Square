// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakeEffectDuel(uint8 StatDoGolpe, int32 PercentualDoGolpe)
	{
		FBattleState Estado;

		FPetState Lancador;
		Lancador.PetId = 1;
		Lancador.Side = 0;
		Lancador.Column = 0;
		Lancador.Row = 1;
		Lancador.Health = 400;
		Lancador.MaxHealth = 400;
		Lancador.Attack = 100;
		Lancador.Defense = 50;
		Lancador.Speed = 50;
		// Golpe 0 carrega o efeito; golpe 1 é um ataque limpo, para medir.
		Lancador.MovePowers[0] = 100;
		Lancador.MoveEffectStats[0] = StatDoGolpe;
		Lancador.MoveEffectPercents[0] = PercentualDoGolpe;
		Lancador.MovePowers[1] = 100;

		FPetState Alvo;
		Alvo.PetId = 2;
		Alvo.Side = 1;
		Alvo.Column = 1;
		Alvo.Row = 1;
		Alvo.Health = 400;
		Alvo.MaxHealth = 400;
		Alvo.Attack = 10;
		Alvo.Defense = 50;
		Alvo.Speed = 50;

		Estado.Pets.Add(Lancador);
		Estado.Pets.Add(Alvo);
		Estado.Random.State = 4242;
		return Estado;
	}

	FTurnCommit MagiaDepoisAtaque(uint8 IndiceDaMagia)
	{
		FTurnCommit Commit;
		Commit.Actions[0] = MakeMoveAction(EActionType::Magia, IndiceDaMagia);
		Commit.Actions[1] = MakeMoveAction(EActionType::Atacar, /*MoveIndex=*/1);
		Commit.Actions[2] = MakeMoveAction(EActionType::Atacar, /*MoveIndex=*/1);
		return Commit;
	}

	FTurnCommit TresVezesAguardar()
	{
		FTurnCommit Commit;
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			Commit.Actions[Slot].Type = EActionType::Aguardar;
		}
		return Commit;
	}

	int32 ContarEventosDoTipo(const TArray<FBattleEvent>& Trace, EBattleEventType Tipo)
	{
		int32 Total = 0;
		for (const FBattleEvent& Evento : Trace)
		{
			if (Evento.Type == Tipo) { ++Total; }
		}
		return Total;
	}
}

// Subir o próprio ataque MUDA o dano que sai depois.
//
// É a razão de existir da escola psíquica: ela muda a luta em vez de
// encerrá-la, e sem esta ligação o bônus seria um número guardado que não
// aparece em lugar nenhum — o defeito que este projeto já chamou de L-041.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuffRaisesLaterDamageTest,
	"BattleSim.StatEffect.BuffRaisesLaterDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBuffRaisesLaterDamageTest::RunTest(const FString& Parameters)
{
	FBattleState ComBonus = MakeEffectDuel(
		static_cast<uint8>(EBattleStat::Ataque), /*Percentual=*/40);
	FBattleState SemBonus = MakeEffectDuel(
		static_cast<uint8>(EBattleStat::Nenhum), /*Percentual=*/0);

	const FBattleResolveResult ComEfeito =
		FBattleResolver::ResolveTurn(ComBonus, MagiaDepoisAtaque(0), TresVezesAguardar());
	const FBattleResolveResult SemEfeito =
		FBattleResolver::ResolveTurn(SemBonus, MagiaDepoisAtaque(0), TresVezesAguardar());

	TestTrue(TEXT("Com o bônus, o alvo termina com MENOS vida"),
		ComEfeito.NextState.Pets[1].Health < SemEfeito.NextState.Pets[1].Health);

	TestEqual(TEXT("E o efeito foi narrado"),
		ContarEventosDoTipo(ComEfeito.Trace, EBattleEventType::AtributoAlterado), 1);

	return true;
}

// O SINAL decide o alvo: positivo em si, negativo no oponente.
//
// Sem isto seria preciso um campo a mais para dizer quem sofre — e um campo
// que só tem duas respostas, das quais uma nunca é usada com o outro sinal.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSignDecidesWhoIsAffectedTest,
	"BattleSim.StatEffect.SignDecidesWhoIsAffected",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSignDecidesWhoIsAffectedTest::RunTest(const FString& Parameters)
{
	FBattleState Positivo = MakeEffectDuel(static_cast<uint8>(EBattleStat::Ataque), 40);
	const FBattleResolveResult SubiuEmSi =
		FBattleResolver::ResolveTurn(Positivo, MagiaDepoisAtaque(0), TresVezesAguardar());

	TestEqual(TEXT("Positivo afeta QUEM LANÇOU"),
		static_cast<int32>(SubiuEmSi.Trace[
			SubiuEmSi.Trace.IndexOfByPredicate([](const FBattleEvent& E)
			{ return E.Type == EBattleEventType::AtributoAlterado; })].TargetId), 1);

	FBattleState Negativo = MakeEffectDuel(static_cast<uint8>(EBattleStat::Defesa), -40);
	const FBattleResolveResult DesceuNoOutro =
		FBattleResolver::ResolveTurn(Negativo, MagiaDepoisAtaque(0), TresVezesAguardar());

	TestEqual(TEXT("Negativo afeta O OPONENTE"),
		static_cast<int32>(DesceuNoOutro.Trace[
			DesceuNoOutro.Trace.IndexOfByPredicate([](const FBattleEvent& E)
			{ return E.Type == EBattleEventType::AtributoAlterado; })].TargetId), 2);

	// E derrubar a defesa do outro precisa AUMENTAR o dano que ele leva.
	FBattleState Limpo = MakeEffectDuel(static_cast<uint8>(EBattleStat::Nenhum), 0);
	const FBattleResolveResult SemNada =
		FBattleResolver::ResolveTurn(Limpo, MagiaDepoisAtaque(0), TresVezesAguardar());

	TestTrue(TEXT("Defesa derrubada faz o alvo perder mais vida"),
		DesceuNoOutro.NextState.Pets[1].Health < SemNada.NextState.Pets[1].Health);

	return true;
}

// UM efeito de cada vez, e o novo SUBSTITUI o antigo.
//
// Empilhar seria dominante: três magias no mesmo turno dobrariam o dano, e a
// escola psíquica venceria por repetição em vez de por escolha.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNewEffectReplacesTheOldTest,
	"BattleSim.StatEffect.NewEffectReplacesTheOld",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNewEffectReplacesTheOldTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = MakeEffectDuel(static_cast<uint8>(EBattleStat::Ataque), 40);

	// Três magias seguidas, a mesma. Se empilhassem, o percentual final seria
	// 120; substituindo, continua 40.
	FTurnCommit TresMagias;
	for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
	{
		TresMagias.Actions[Slot] = MakeMoveAction(EActionType::Magia, /*MoveIndex=*/0);
	}

	const FBattleResolveResult Resultado =
		FBattleResolver::ResolveTurn(Estado, TresMagias, TresVezesAguardar());

	TestEqual(TEXT("O percentual ativo continua o de UMA magia"),
		Resultado.NextState.Pets[0].ActiveEffectPercent, 40);

	return true;
}

// O efeito EXPIRA, e o fim é narrado como o começo foi.
//
// Bônus que some calado faz o jogador achar que o dano ficou errado — e este
// projeto já gastou rodadas com regra funcionando e parecendo quebrada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEffectExpiresAndSaysSoTest,
	"BattleSim.StatEffect.ExpiresAndSaysSo",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEffectExpiresAndSaysSoTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = MakeEffectDuel(static_cast<uint8>(EBattleStat::Ataque), 40);

	const FBattleResolveResult Primeiro =
		FBattleResolver::ResolveTurn(Estado, MagiaDepoisAtaque(0), TresVezesAguardar());

	// Lançado no slot 0, dura três slots: expira ao fim do turno, e não antes.
	TestEqual(TEXT("O efeito acabou ao fim do turno"),
		static_cast<int32>(Primeiro.NextState.Pets[0].ActiveEffectSlotsRemaining), 0);
	TestEqual(TEXT("E o fim foi narrado"),
		ContarEventosDoTipo(Primeiro.Trace, EBattleEventType::AtributoVoltouAoNormal), 1);
	TestEqual(TEXT("O atributo voltou ao valor limpo"),
		Primeiro.NextState.Pets[0].GetEffectiveAttack(),
		Primeiro.NextState.Pets[0].Attack);

	return true;
}

// O TETO é amarra do jogo, e o núcleo recorta mesmo que a montagem já tenha
// recortado: um estado vindo da rede não pode passar por cima dela.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEffectIsClampedInTheCoreTest,
	"BattleSim.StatEffect.ClampedInTheCore",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEffectIsClampedInTheCoreTest::RunTest(const FString& Parameters)
{
	FBattleState Absurdo = MakeEffectDuel(static_cast<uint8>(EBattleStat::Ataque), 5000);
	const FBattleResolveResult Resultado =
		FBattleResolver::ResolveTurn(Absurdo, MagiaDepoisAtaque(0), TresVezesAguardar());

	// O valor é lido no EVENTO, e não no estado final: o efeito expira ao fim
	// do turno, então o estado final tem zero — e zero passaria por "recortado"
	// sem provar nada. O evento é também o que o jogador lê.
	const int32 Indice = Resultado.Trace.IndexOfByPredicate([](const FBattleEvent& E)
	{
		return E.Type == EBattleEventType::AtributoAlterado;
	});
	TestTrue(TEXT("O efeito foi aplicado"), Indice != INDEX_NONE);
	TestEqual(TEXT("Recortado no teto"),
		Indice != INDEX_NONE ? Resultado.Trace[Indice].Value : 0,
		BattleStatEffectMaxPercent);

	// E do outro lado: o atributo NUNCA chega a zero, senão o pet deixa de ser
	// adversário e perder assim não ensina nada a quem perdeu.
	FBattleState Aniquilador = MakeEffectDuel(static_cast<uint8>(EBattleStat::Defesa), -5000);
	const FBattleResolveResult Fundo =
		FBattleResolver::ResolveTurn(Aniquilador, MagiaDepoisAtaque(0), TresVezesAguardar());

	TestTrue(TEXT("A defesa do alvo continua ao menos 1"),
		Fundo.NextState.Pets[1].GetEffectiveDefense() >= 1);

	return true;
}

// SÓ A MAGIA carrega efeito. Ataque físico que mexesse em atributo apagaria a
// diferença entre as duas ações — e é ela que dá sentido à escola psíquica.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhysicalAttackCarriesNoEffectTest,
	"BattleSim.StatEffect.PhysicalAttackCarriesNoEffect",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPhysicalAttackCarriesNoEffectTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = MakeEffectDuel(static_cast<uint8>(EBattleStat::Ataque), 40);

	// O MESMO golpe, com o mesmo efeito cadastrado, usado como ATAQUE.
	FTurnCommit SoAtaques;
	for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
	{
		SoAtaques.Actions[Slot] = MakeMoveAction(EActionType::Atacar, /*MoveIndex=*/0);
	}

	const FBattleResolveResult Resultado =
		FBattleResolver::ResolveTurn(Estado, SoAtaques, TresVezesAguardar());

	TestEqual(TEXT("Nenhum efeito de atributo foi aplicado"),
		ContarEventosDoTipo(Resultado.Trace, EBattleEventType::AtributoAlterado), 0);
	TestEqual(TEXT("E ninguém ficou com efeito ativo"),
		static_cast<int32>(Resultado.NextState.Pets[0].ActiveEffectSlotsRemaining), 0);

	return true;
}

// O efeito ENTRA NO HASH. Sem isso, dois estados que resolvem diferente
// teriam a mesma assinatura — e numa partida em rede os dois lados
// divergiriam sem nada acusar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEffectEntersStateHashTest,
	"BattleSim.StatEffect.EntersStateHash",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEffectEntersStateHashTest::RunTest(const FString& Parameters)
{
	FBattleState Limpo = MakeEffectDuel(static_cast<uint8>(EBattleStat::Nenhum), 0);

	FBattleState ComEfeito = Limpo;
	ComEfeito.Pets[0].ActiveEffectStat = static_cast<uint8>(EBattleStat::Ataque);
	ComEfeito.Pets[0].ActiveEffectPercent = 40;
	ComEfeito.Pets[0].ActiveEffectSlotsRemaining = 3;

	TestNotEqual(TEXT("Efeito ativo muda a assinatura"),
		Limpo.ComputeHash(), ComEfeito.ComputeHash());

	// E o EFEITO DO GOLPE também: dois pets com os mesmos atributos e golpes
	// de efeito diferentes são dois pets diferentes.
	FBattleState OutroGolpe = Limpo;
	OutroGolpe.Pets[0].MoveEffectStats[0] = static_cast<uint8>(EBattleStat::Defesa);
	OutroGolpe.Pets[0].MoveEffectPercents[0] = -30;

	TestNotEqual(TEXT("Efeito cadastrado no golpe muda a assinatura"),
		Limpo.ComputeHash(), OutroGolpe.ComputeHash());

	return true;
}

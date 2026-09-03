// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Balance/TypeEffectivenessTable.h"
#include "World/SchoolTeachings.h"

/**
 * O QUADRO DE LIÇÕES — decisão 64.
 *
 * O que se afirma é a INFORMAÇÃO: o golpe trancado sai com o requisito e com o
 * dedo apontando o campo do pátio. Uma lição que só diz "trancado" ensina
 * tanto quanto o cadeado da batalha ensinava — nada.
 */

namespace QuadroDeLicoesTeste
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	FLoadedPetMove GolpeParaOQuadroDeLicoes(const TCHAR* Nome,
		const TCHAR* Exige, int32 Valor)
	{
		FLoadedPetMove Golpe;
		Golpe.Name = Nome;
		Golpe.RequiresAttribute = Exige;
		Golpe.RequiresValue = Valor;
		return Golpe;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSchoolLessonNamesWhatIsMissingTest,
	"BattleSquare.World.Escola.ALicaoDizOQueFaltaEOnde",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSchoolLessonNamesWhatIsMissingTest::RunTest(const FString&)
{
	FLoadedPetRecord Registro;
	Registro.Moves.Add(QuadroDeLicoesTeste::GolpeParaOQuadroDeLicoes(
		TEXT("Investida"), TEXT("none"), 0));
	Registro.Moves.Add(QuadroDeLicoesTeste::GolpeParaOQuadroDeLicoes(
		TEXT("Explosao"), TEXT("musculature"), 12));
	Registro.Moves.Add(QuadroDeLicoesTeste::GolpeParaOQuadroDeLicoes(
		TEXT("Rasante"), TEXT("flight"), 8));

	// Um aluno com musculatura de sobra e voo de menos: uma lição de cada
	// tipo, no mesmo quadro.
	FOwnedPetInstance Aluno;
	Aluno.Musculature = 20;

	const TArray<FString> Linhas = SchoolTeachings::BuildLessonLines(Registro, Aluno);
	TestEqual(TEXT("uma linha por golpe"), Linhas.Num(), 3);

	// Golpe sem requisito e golpe com requisito CUMPRIDO se alcançam.
	TestTrue(TEXT("o golpe livre esta alcancado"), Linhas[0].Contains(TEXT("✓")));
	TestTrue(TEXT("o golpe de musculatura tambem — 20 de 12"),
		Linhas[1].Contains(TEXT("✓")));

	// O TRANCADO diz o que falta E onde treinar — as duas metades. Só o
	// cadeado seria a tela de batalha de novo, com outro fundo.
	TestTrue(TEXT("o de voo esta trancado"), Linhas[2].Contains(TEXT("🔒")));
	TestTrue(TEXT("a licao diz QUANTO falta"), Linhas[2].Contains(TEXT("8")));
	TestTrue(TEXT("e diz ONDE treinar"), Linhas[2].Contains(TEXT("pátio")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSchoolStrategyComesFromTheTableTest,
	"BattleSquare.World.Escola.AEstrategiaVemDaTabela",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSchoolStrategyComesFromTheTableTest::RunTest(const FString&)
{
	// A lição de tipos sai da MESMA tabela da batalha — montada aqui em
	// memória, como os testes da própria tabela fazem. Uma lição escrita à
	// mão ensinaria um número e a batalha cobraria outro.
	FTypeEffectivenessTable Tabela;
	Tabela.SetPercent(TEXT("Fisica/Agua"), TEXT("Fisica/Fogo"), 150);
	Tabela.SetPercent(TEXT("Fisica/Fogo"), TEXT("Fisica/Agua"), 50);
	Tabela.SetPercent(TEXT("Fisica/Raio"), TEXT("Fisica/Agua"), 150);

	const TArray<FString> Tipos = {
		TEXT("Fisica/Agua"), TEXT("Fisica/Fogo"),
		TEXT("Fisica/Raio"), TEXT("Fisica/Terra"),
	};

	const TArray<FString> Linhas = SchoolTeachings::BuildStrategyLines(
		TEXT("Fisica/Agua"), Tabela, Tipos);

	// As três lições, cada uma na sua linha: vantagem, desvantagem, e o
	// aviso — que só existe para golpe FORTE (150+), porque aviso de tudo é
	// aviso de nada.
	bool bBomContraFogo = false;
	bool bRuimContraFogo = false;
	bool bCuidadoComRaio = false;
	for (const FString& Linha : Linhas)
	{
		bBomContraFogo |= Linha.Contains(TEXT("bom contra")) && Linha.Contains(TEXT("Fogo"));
		bRuimContraFogo |= Linha.Contains(TEXT("ruim contra")) && Linha.Contains(TEXT("Fogo"));
		bCuidadoComRaio |= Linha.Contains(TEXT("apanha")) && Linha.Contains(TEXT("Raio"));
	}

	TestTrue(TEXT("agua e boa contra fogo — 150 na tabela"), bBomContraFogo);
	TestTrue(TEXT("e o aviso aponta o raio — 150 contra agua"), bCuidadoComRaio);
	TestFalse(TEXT("fogo nao aparece como ruim: 50 e vantagem DELE, nao nossa"),
		bRuimContraFogo);

	// O NEUTRO fica de fora: Terra não tem par cadastrado com Água, e lição
	// sobre o neutro é ruído.
	for (const FString& Linha : Linhas)
	{
		TestFalse(TEXT("o par neutro nao vira licao"), Linha.Contains(TEXT("Terra")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSchoolTerrainLessonsExistTest,
	"BattleSquare.World.Escola.OTerrenoTemLicao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSchoolTerrainLessonsExistTest::RunTest(const FString&)
{
	// Cada frase do terreno tem um teste de BattleSim por trás, e este teste
	// afirma que as lições EXISTEM e citam as regras certas — voar escapa do
	// chão (`LeavingTheGround.AvoidsCellDamage`), submergir exige água funda.
	const TArray<FString> Linhas = SchoolTeachings::BuildTerrainLines();
	TestTrue(TEXT("ha licao de terreno"), Linhas.Num() > 0);

	bool bDanoEVoar = false;
	bool bSubmergir = false;
	for (const FString& Linha : Linhas)
	{
		bDanoEVoar |= Linha.Contains(TEXT("DANO")) && Linha.Contains(TEXT("voar"));
		bSubmergir |= Linha.Contains(TEXT("submergir")) && Linha.Contains(TEXT("FUNDA"));
	}

	TestTrue(TEXT("a licao do dano diz que voar escapa"), bDanoEVoar);
	TestTrue(TEXT("a licao da agua diz que submergir exige fundura"), bSubmergir);

	return true;
}

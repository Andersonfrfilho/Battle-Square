// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
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

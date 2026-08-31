// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetTypeIdentity.h"
#include "Balance/TypeEffectivenessTable.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// T1: par presente retorna o percentual cadastrado; par ausente e
// "mesmo tipo contra si mesmo" (sem entrada explícita) retornam 100.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTypeEffectivenessTableGetPercentTest,
	"BattleSquare.Balance.TypeEffectivenessTable.GetPercent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTypeEffectivenessTableGetPercentTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Table;
	Table.SetPercent(TEXT("Fogo"), TEXT("Planta"), 150);
	Table.SetPercent(TEXT("Fogo"), TEXT("Agua"), 50);

	TestEqual(TEXT("Par cadastrado super efetivo"), Table.GetPercent(TEXT("Fogo"), TEXT("Planta")), 150);
	TestEqual(TEXT("Par cadastrado resistido"), Table.GetPercent(TEXT("Fogo"), TEXT("Agua")), 50);
	TestEqual(TEXT("Par ausente é neutro"), Table.GetPercent(TEXT("Fogo"), TEXT("Eletrico")), 100);
	TestEqual(TEXT("Mesmo tipo contra si mesmo, sem entrada explícita, é neutro"), Table.GetPercent(TEXT("Fogo"), TEXT("Fogo")), 100);

	return true;
}

namespace
{
	FString WriteFixtureFile(const FString& FileName, const FString& Contents)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TypeEffectivenessFixture"), FileName);
		FFileHelper::SaveStringToFile(Contents, *Path);
		return Path;
	}
}

// T2: arquivo válido carrega todos os pares; ausente e malformado
// retornam false sem crash.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTypeEffectivenessTableLoadFromJsonTest,
	"BattleSquare.Balance.TypeEffectivenessTable.LoadFromJson",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTypeEffectivenessTableLoadFromJsonTest::RunTest(const FString& Parameters)
{
	// Caso 1: arquivo válido.
	const FString ValidJson = TEXT(R"({
		"types": ["Fogo", "Agua", "Planta"],
		"effectiveness": [
			{ "attacker": "Fogo", "defender": "Planta", "percent": 150 },
			{ "attacker": "Fogo", "defender": "Agua", "percent": 50 },
			{ "attacker": "Agua", "defender": "Fogo", "percent": 150 }
		]
	})");
	const FString ValidPath = WriteFixtureFile(TEXT("valid.json"), ValidJson);

	FTypeEffectivenessTable Table;
	TestTrue(TEXT("Arquivo válido carrega com sucesso"), FTypeEffectivenessTable::LoadFromJson(ValidPath, Table));
	TestEqual(TEXT("Par Fogo->Planta carregado corretamente"), Table.GetPercent(TEXT("Fogo"), TEXT("Planta")), 150);
	TestEqual(TEXT("Par Fogo->Agua carregado corretamente"), Table.GetPercent(TEXT("Fogo"), TEXT("Agua")), 50);
	TestEqual(TEXT("Par Agua->Fogo carregado corretamente"), Table.GetPercent(TEXT("Agua"), TEXT("Fogo")), 150);
	TestEqual(TEXT("Par sem entrada continua neutro"), Table.GetPercent(TEXT("Planta"), TEXT("Agua")), 100);

	// Caso 2: arquivo ausente.
	FTypeEffectivenessTable MissingTable;
	const FString MissingPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TypeEffectivenessFixture"), TEXT("nao-existe.json"));
	TestFalse(TEXT("Arquivo ausente retorna false"), FTypeEffectivenessTable::LoadFromJson(MissingPath, MissingTable));

	// Caso 3: JSON malformado (sintaxe inválida).
	const FString MalformedPath = WriteFixtureFile(TEXT("malformed.json"), TEXT("{ isto nao e json valido"));
	FTypeEffectivenessTable MalformedTable;
	TestFalse(TEXT("JSON malformado retorna false, sem crash"), FTypeEffectivenessTable::LoadFromJson(MalformedPath, MalformedTable));

	// Caso 4: tipo novo reconhecido sem mudança de código (ESCALA-06) —
	// prova viva: um tipo que não existia no Caso 1 aparece aqui só
	// porque o JSON mudou.
	const FString NewTypeJson = TEXT(R"({
		"types": ["Fogo", "Eletrico"],
		"effectiveness": [
			{ "attacker": "Eletrico", "defender": "Fogo", "percent": 100 },
			{ "attacker": "Fogo", "defender": "Eletrico", "percent": 150 }
		]
	})");
	const FString NewTypePath = WriteFixtureFile(TEXT("new-type.json"), NewTypeJson);
	FTypeEffectivenessTable NewTypeTable;
	TestTrue(TEXT("Arquivo com tipo novo carrega com sucesso"), FTypeEffectivenessTable::LoadFromJson(NewTypePath, NewTypeTable));
	TestEqual(TEXT("Tipo novo (Eletrico) reconhecido sem rebuild"), NewTypeTable.GetPercent(TEXT("Fogo"), TEXT("Eletrico")), 150);

	return true;
}

// A tabela de DOIS EIXOS se compõe, e é o composto que decide o dano.
//
// O teste lê o arquivo REAL. Um teste sobre uma tabela inventada aqui provaria
// que o código multiplica, e não que o jogo está equilibrado — e é o jogo que
// se joga.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShippedTypeChartComposesTwoAxesTest,
	"BattleSquare.Balance.TypeEffectiveness.ShippedChartComposesTwoAxes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FShippedTypeChartComposesTwoAxesTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Tabela;
	const FString Caminho = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("TypeEffectiveness.json"));
	if (!FTypeEffectivenessTable::LoadFromJson(Caminho, Tabela))
	{
		AddError(TEXT("Config/TypeEffectiveness.json não carregou"));
		return false;
	}

	// Mesmo elemento, escolas diferentes: só o eixo da escola pesa.
	TestEqual(TEXT("Natural bate Física no eixo da escola"),
		Tabela.GetPercent(TEXT("Natural/Fogo"), TEXT("Fisica/Fogo")), 120);

	// Mesma escola, elementos diferentes: só o eixo do elemento pesa.
	TestEqual(TEXT("Água apaga Fogo no eixo do elemento"),
		Tabela.GetPercent(TEXT("Natural/Agua"), TEXT("Natural/Fogo")), 150);

	// OS DOIS de uma vez, que é o ponto da composição.
	TestEqual(TEXT("Vantagem dupla compõe: 150 × 120 ÷ 100"),
		Tabela.GetPercent(TEXT("Natural/Agua"), TEXT("Fisica/Fogo")), 180);
	TestEqual(TEXT("Desvantagem dupla também: 50 × 80 ÷ 100"),
		Tabela.GetPercent(TEXT("Fisica/Fogo"), TEXT("Natural/Agua")), 40);

	// Vantagem num eixo e desvantagem no outro quase se anulam — e é isso que
	// impede que um tipo seja bom contra tudo.
	TestEqual(TEXT("Eixos opostos se cancelam quase por completo"),
		Tabela.GetPercent(TEXT("Psiquica/Fogo"), TEXT("Fisica/Agua")), 40);

	// O MÁGICO deixou de ser especial, e este é o número que prova.
	//
	// Ele tinha 150% contra os três tipos naturais. Agora é psíquico de fogo
	// como qualquer outro, e contra um natural de fogo tem 120 — o eixo suave.
	TestEqual(TEXT("Psíquico contra natural do MESMO elemento é só 120"),
		Tabela.GetPercent(TEXT("Psiquica/Fogo"), TEXT("Natural/Fogo")), 120);

	// Nomes ANTIGOS continuam valendo: eles já foram assinados.
	TestEqual(TEXT("'Magico' contra 'Fogo' resolve pelo par traduzido"),
		Tabela.GetPercent(TEXT("Magico"), TEXT("Fogo")), 120);
	TestEqual(TEXT("'Agua' contra 'Fogo' continua sendo 150"),
		Tabela.GetPercent(TEXT("Agua"), TEXT("Fogo")), 150);

	// A LUZ é a resposta ao fantasma, e é o número mais alto da tabela: 200,
	// acima do 150 do ciclo natural. "Bem efetivo" precisava ser mais que a
	// vantagem comum, senão a luz seria só mais um elemento que vence outro.
	TestEqual(TEXT("Luz contra Fantasma é 200"),
		Tabela.GetPercent(TEXT("Natural/Luz"), TEXT("Natural/Fantasma")), 200);
	TestEqual(TEXT("E o fantasma quase não arranha a luz"),
		Tabela.GetPercent(TEXT("Natural/Fantasma"), TEXT("Natural/Luz")), 50);

	// O fantasma NÃO é forte contra o ciclo natural. A força dele está no que
	// ele FAZ — atravessa, não toca o chão, o físico não o acerta — e somar
	// vantagem de tabela a isso faria um tipo bom contra tudo.
	TestEqual(TEXT("Fantasma contra Fogo é neutro"),
		Tabela.GetPercent(TEXT("Natural/Fantasma"), TEXT("Natural/Fogo")), 100);

	// NENHUM tipo é forte contra todos. Tipo sem fraqueza é tipo que todo
	// mundo escolhe, e aí a tabela inteira deixa de decidir alguma coisa.
	TArray<FString> Todos;
	for (const FString& Escola : FPetTypeIdentity::AllSchools())
	{
		for (const FString& Elemento : FPetTypeIdentity::AllElements())
		{
			Todos.Add(FString::Printf(TEXT("%s/%s"), *Escola, *Elemento));
		}
	}
	// A conta sai dos EIXOS, e não de um número escrito aqui: era doze, virou
	// vinte e quatro com a escola espiritual e os elementos fantasma e luz, e
	// um literal aqui só diria que alguém esqueceu de editar o teste.
	TestEqual(TEXT("Todo par de escola e elemento é um tipo"), Todos.Num(),
		FPetTypeIdentity::AllSchools().Num() * FPetTypeIdentity::AllElements().Num());
	TestTrue(TEXT("E há tipos de sobra para a tabela decidir algo"), Todos.Num() >= 12);

	for (const FString& Atacante : Todos)
	{
		bool bTemFraqueza = false;
		for (const FString& Defensor : Todos)
		{
			if (Tabela.GetPercent(Defensor, Atacante) > 100)
			{
				bTemFraqueza = true;
				break;
			}
		}
		TestTrue(*FString::Printf(TEXT("%s tem ao menos um tipo que o supera"), *Atacante),
			bTemFraqueza);
	}

	return true;
}

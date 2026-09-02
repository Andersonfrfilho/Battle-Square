// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetBiologyCatalog.h"
#include "Balance/PetTypeCatalog.h"
#include "Balance/PetTypeIdentity.h"
#include "Data/BattleDataTranslator.h"
#include "Data/PetDataLoader.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO (L-042).
namespace BiologiaDoPetTeste
{
	FPetBiologyCatalog MontarCatalogo()
	{
		FPetBiologyCatalog Catalogo;

		FPetBiologyTrait Escamosa;
		Escamosa.Name = TEXT("escamosa");
		Escamosa.FluidResists.Add(TEXT("agua doce"), 20);
		Catalogo.Skins.Add(Escamosa);

		FPetBiologyTrait Peluda;
		Peluda.Name = TEXT("peluda");
		Peluda.FluidResists.Add(TEXT("agua doce"), -20);
		Peluda.FootingPerMille = -50;
		Catalogo.Skins.Add(Peluda);

		FPetBiologyTrait Corpulento;
		Corpulento.Name = TEXT("corpulento");
		Corpulento.FootingPerMille = 200;
		Catalogo.Builds.Add(Corpulento);

		FPetBiologyTrait Branquia;
		Branquia.Name = TEXT("branquia");
		Branquia.FluidResists.Add(TEXT("agua doce"), 30);
		Branquia.bBreathesUnderwater = true;
		Catalogo.Breathings.Add(Branquia);

		FPetBiologyTrait Patas;
		Patas.Name = TEXT("patas");
		Catalogo.Limbs.Add(Patas);

		return Catalogo;
	}

	/** RAII: o override tem de sair mesmo se o teste falhar no meio. */
	struct FCatalogoInstalado
	{
		explicit FCatalogoInstalado(const FPetBiologyCatalog& Catalogo)
		{
			FPetBiologyCatalog::OverrideForTesting(&Catalogo);
		}
		~FCatalogoInstalado()
		{
			FPetBiologyCatalog::OverrideForTesting(nullptr);
		}
	};
}

// ---------------------------------------------------------------------------
// ESTE É O ACEITE DA TAREFA.
//
// Dois pets do MESMO elemento com biologias diferentes resistem diferente. Sem
// isto, a anatomia é o elemento com outro nome — e o pedido do usuário ("depende
// da anatomia dele e da biologia da pele dele") não teria sido atendido.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetBiologySameElementDiffersTest,
	"BattleSquare.PetBiology.SameElementDiffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetBiologySameElementDiffersTest::RunTest(const FString& Parameters)
{
	const FPetBiologyCatalog Catalogo = BiologiaDoPetTeste::MontarCatalogo();
	const BiologiaDoPetTeste::FCatalogoInstalado Instalado(Catalogo);

	FPetBiology Escamoso;
	Escamoso.Skin = TEXT("escamosa");

	FPetBiology Peludo;
	Peludo.Skin = TEXT("peluda");

	const int32 DoEscamoso =
		FPetBiologyCatalog::Get().ResistFor(Escamoso, TEXT("agua doce"));
	const int32 DoPeludo =
		FPetBiologyCatalog::Get().ResistFor(Peludo, TEXT("agua doce"));

	AddInfo(FString::Printf(TEXT("escamoso %d, peludo %d"), DoEscamoso, DoPeludo));

	TestTrue(TEXT("duas peles resistem DIFERENTE a mesma agua"),
		DoEscamoso != DoPeludo);

	// E a diferença tem SINAL: uma resiste, a outra é fraca. Só "diferente"
	// passaria com 20 e 21, que para quem joga é a mesma coisa.
	TestTrue(TEXT("a escamosa resiste"), DoEscamoso > 0);
	TestTrue(TEXT("e a peluda e FRACA — biologia tem custo"), DoPeludo < 0);

	return true;
}

// OS EIXOS SOMAM. Quatro chamadas espalhadas por quem precisa fariam a terceira
// cópia esquecer um eixo, e o pet resistiria diferente conforme quem perguntou.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetBiologyAxesAddUpTest,
	"BattleSquare.PetBiology.AxesAddUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetBiologyAxesAddUpTest::RunTest(const FString& Parameters)
{
	const FPetBiologyCatalog Catalogo = BiologiaDoPetTeste::MontarCatalogo();
	const BiologiaDoPetTeste::FCatalogoInstalado Instalado(Catalogo);

	FPetBiology Completa;
	Completa.Skin = TEXT("escamosa");
	Completa.Breathing = TEXT("branquia");

	TestEqual(TEXT("pele 20 mais branquia 30 da 50"),
		FPetBiologyCatalog::Get().ResistFor(Completa, TEXT("agua doce")), 50);

	FPetBiology Firmeza;
	Firmeza.Skin = TEXT("peluda");
	Firmeza.Build = TEXT("corpulento");

	TestEqual(TEXT("pelo -50 mais corpulento 200 da 150"),
		FPetBiologyCatalog::Get().FootingFor(Firmeza), 150);

	return true;
}

// CONTRAPESO — BIOLOGIA VAZIA SOMA ZERO.
//
// Este é o que protege todo pet já assinado: o cadastro deles não tem o campo,
// e a regra nova não pode mudar a resistência de ninguém sem uma linha do
// cadastro ter mudado. O dono só descobriria perdendo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetBiologyEmptyChangesNothingTest,
	"BattleSquare.PetBiology.EmptyChangesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetBiologyEmptyChangesNothingTest::RunTest(const FString& Parameters)
{
	const FPetBiologyCatalog Catalogo = BiologiaDoPetTeste::MontarCatalogo();
	const BiologiaDoPetTeste::FCatalogoInstalado Instalado(Catalogo);

	const FPetBiology Nenhuma;
	TestTrue(TEXT("biologia vazia se reconhece como vazia"), Nenhuma.IsEmpty());
	TestEqual(TEXT("e nao resiste a nada"),
		FPetBiologyCatalog::Get().ResistFor(Nenhuma, TEXT("agua doce")), 0);
	TestEqual(TEXT("nem se firma"),
		FPetBiologyCatalog::Get().FootingFor(Nenhuma), 0);

	// EIXO DESCONHECIDO também soma zero, e não derruba: um erro de digitação
	// no cadastro não pode tirar o pet da partida.
	FPetBiology Errada;
	Errada.Skin = TEXT("pele que ninguem cadastrou");
	TestEqual(TEXT("eixo desconhecido soma zero"),
		FPetBiologyCatalog::Get().ResistFor(Errada, TEXT("agua doce")), 0);

	// E um eixo informado NÃO inventa os outros: um cadastro que só diz a pele
	// não ganha um porte por omissão.
	FPetBiology SoPele;
	SoPele.Skin = TEXT("peluda");
	TestEqual(TEXT("so a pele, so a firmeza dela"),
		FPetBiologyCatalog::Get().FootingFor(SoPele), -50);

	return true;
}

// A RESPIRAÇÃO É DE UM EIXO SÓ, e basta um dizer sim.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetBiologyBreathingTest,
	"BattleSquare.PetBiology.BreathingUnderwater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetBiologyBreathingTest::RunTest(const FString& Parameters)
{
	const FPetBiologyCatalog Catalogo = BiologiaDoPetTeste::MontarCatalogo();
	const BiologiaDoPetTeste::FCatalogoInstalado Instalado(Catalogo);

	FPetBiology ComBranquia;
	ComBranquia.Breathing = TEXT("branquia");
	TestTrue(TEXT("quem tem branquia respira embaixo d'agua"),
		FPetBiologyCatalog::Get().BreathesUnderwater(ComBranquia));

	// CONTRAPESO: sem o eixo, não respira. Sem este, uma implementação que
	// devolvesse sempre `true` passaria no de cima.
	const FPetBiology Nenhuma;
	TestTrue(TEXT("sem o eixo, nao respira"),
		!FPetBiologyCatalog::Get().BreathesUnderwater(Nenhuma));

	return true;
}

// O CATÁLOGO DE VERDADE CARREGA, e os nomes de fluido dele CASAM com o registro.
//
// Uma segunda grafia seria uma resistência que nunca casa e nunca acusa: o JSON
// diria "agua salgada", o registro diria outra coisa, e o pet resistiria a nada
// com a bateria toda verde.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetBiologyRealCatalogLoadsTest,
	"BattleSquare.PetBiology.RealCatalogLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetBiologyRealCatalogLoadsTest::RunTest(const FString& Parameters)
{
	const FPetBiologyCatalog& Real = FPetBiologyCatalog::Get();

	TestTrue(TEXT("ha peles cadastradas"), Real.Skins.Num() > 0);
	TestTrue(TEXT("ha portes cadastrados"), Real.Builds.Num() > 0);
	TestTrue(TEXT("ha respiracoes cadastradas"), Real.Breathings.Num() > 0);
	TestTrue(TEXT("ha apoios cadastrados"), Real.Limbs.Num() > 0);

	const TArray<FPetBiologyTrait>* Eixos[4] =
		{ &Real.Skins, &Real.Builds, &Real.Breathings, &Real.Limbs };

	int32 Casaram = 0;
	for (const TArray<FPetBiologyTrait>* Eixo : Eixos)
	{
		for (const FPetBiologyTrait& Traco : *Eixo)
		{
			for (const auto& Par : Traco.FluidResists)
			{
				bool bAchou = false;
				for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
				{
					const FString Nome = FluidRegistry::TraitsOf(
						static_cast<EFluidKind>(Qual)).DebugName;
					if (Nome.Equals(Par.Key, ESearchCase::IgnoreCase))
					{
						bAchou = true;
						++Casaram;
						break;
					}
				}

				TestTrue(*FString::Printf(
					TEXT("'%s' em '%s' e um fluido do registro"),
					*Par.Key, *Traco.Name), bAchou);
			}
		}
	}

	// E ALGUMA casou: se o JSON não tivesse resistência nenhuma, o laço acima
	// não afirmaria nada e passaria calado.
	TestTrue(TEXT("ao menos uma resistencia foi conferida"), Casaram > 0);

	return true;
}

// ---------------------------------------------------------------------------
// O CAMINHO DE PRODUÇÃO, e não só o catálogo.
//
// Regra sem chamador em produção é regra que não existe (invariante 11). Os
// testes acima provam a SOMA; este prova que ela chega ao pet que entra na
// batalha, atravessando o tradutor — que é o único lugar por onde ela chega.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetBiologyReachesTheBattleTest,
	"BattleSquare.PetBiology.ReachesTheBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetBiologyReachesTheBattleTest::RunTest(const FString& Parameters)
{
	const FPetBiologyCatalog Catalogo = BiologiaDoPetTeste::MontarCatalogo();
	const BiologiaDoPetTeste::FCatalogoInstalado Instalado(Catalogo);

	// O MESMO cadastro nos dois, mudando SÓ a biologia. Dois registros
	// diferentes não provariam nada: a diferença poderia vir do tipo, do
	// ataque, de qualquer coisa.
	FLoadedPetRecord Base;
	Base.Id = TEXT("um");
	Base.Name = TEXT("Um");
	// ESCOLA/ELEMENTO, nesta ordem — ver a nota no teste da bota.
	Base.Type = TEXT("Fisica/Agua");
	Base.Attack = 50; Base.Defense = 50; Base.Speed = 50; Base.MaxHealth = 100;

	FLoadedPetRecord Corpulento = Base;
	Corpulento.Biology.Build = TEXT("corpulento");

	FPetState SemBiologia;
	FPetState ComBiologia;
	FPetPresentationInfo Fora;

	FBattleDataTranslator::TranslatePet(Base, 1, 0, 0, 1, SemBiologia, Fora);
	FBattleDataTranslator::TranslatePet(Corpulento, 2, 1, 2, 1, ComBiologia, Fora);

	AddInfo(FString::Printf(TEXT("firmeza sem biologia %d, com corpulento %d"),
		SemBiologia.FootingPerMille, ComBiologia.FootingPerMille));

	TestTrue(TEXT("o corpulento se firma MAIS contra a corrente"),
		ComBiologia.FootingPerMille > SemBiologia.FootingPerMille);

	// CONTRAPESO: sem biologia, a firmeza é EXATAMENTE a do elemento. Este é o
	// que protege todo pet já assinado — a regra nova não pode mudar ninguém
	// sem uma linha do cadastro ter mudado.
	const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(Base.Type);
	if (const FPetElementDefinition* Elemento =
		FPetTypeCatalog::Get().FindElement(Identidade.Element))
	{
		TestEqual(TEXT("sem biologia, so o elemento decide a firmeza"),
			SemBiologia.FootingPerMille, Elemento->FootingPerMille);
	}
	else
	{
		// A mensagem DIZ O QUE MEDIU. "Sumiu do catálogo" me custou três
		// rodadas: ela nomeia o sintoma e nenhuma das três coisas que podem
		// causá-lo — o tipo escrito errado, o Parse não reconhecendo, ou o
		// catálogo vazio. Cada uma pede um conserto diferente.
		FString Quais;
		for (const FPetElementDefinition& Candidato : FPetTypeCatalog::Get().GetElements())
		{
			Quais += Candidato.Name + TEXT(" ");
		}

		AddError(FString::Printf(
			TEXT("tipo '%s' -> escola '%s', elemento '%s'; o catalogo tem %d: %s"),
			*Base.Type, *Identidade.School, *Identidade.Element,
			FPetTypeCatalog::Get().GetElements().Num(), *Quais));
	}

	// E A BIOLOGIA ACOMPANHA O PET até a coleção: sem isso, ela decidiria a
	// batalha em que ele foi capturado e sumiria depois.
	TestEqual(TEXT("a apresentacao leva o eixo pelo NOME"),
		Fora.Biology.Build, FString(TEXT("corpulento")));

	return true;
}

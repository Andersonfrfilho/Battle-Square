// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetMorphology.h"
#include "Battle/PetAppearance.h"
#include "Misc/AutomationTest.h"

namespace
{
	/**
	 * Os tipos que o jogo tem hoje, mais um desconhecido.
	 *
	 * Varrer só o tipo neutro deixaria passar exatamente o defeito que o teto
	 * do adorno existe para impedir: a folha do tipo Planta é achatada e
	 * comprida, e é ela que estoura primeiro.
	 */
	const TCHAR* MorfologiaTiposDeTeste[] =
	{
		TEXT("Dog"), TEXT("Fogo"), TEXT("Agua"), TEXT("Planta"), TEXT("TipoQueNaoExiste")
	};

	const EPetGrowthStage MorfologiaFasesDeTeste[] =
	{
		EPetGrowthStage::Filhote, EPetGrowthStage::Adulto, EPetGrowthStage::Evoluido
	};

	FString MorfologiaSementeDeTeste(int32 Indice)
	{
		// Formato de UUID, que é o que o CatalogId realmente é: semente curta
		// e sequencial esconderia dependência do comprimento da string.
		return FString::Printf(TEXT("%08x-4f04-4fb1-8588-e572a2aa%04x"), Indice * 2654435761u, Indice);
	}

	/**
	 * Onde a superfície do tronco está, na direção em que a cabeça foi posta.
	 *
	 * O teste refaz esta conta em vez de perguntar ao gerador de propósito: se
	 * ele errar a geometria, um acessor dele confirmaria o próprio erro.
	 */
	float MorfologiaSuperficieNaDirecaoDaCabeca(const FPetMorphology& Corpo)
	{
		const float Frente = Corpo.HeadForwardUnits;
		const float Sobe = Corpo.HeadCenterUnits - Corpo.BodyCenterUnits;
		const float Distancia = FMath::Sqrt(FMath::Square(Frente) + FMath::Square(Sobe));
		if (Distancia <= KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}

		const float Cosseno = Frente / Distancia;
		const float Seno = Sobe / Distancia;
		return 1.0f / FMath::Sqrt(
			FMath::Square(Cosseno / Corpo.BodySemiAxisXUnits())
			+ FMath::Square(Seno / Corpo.BodySemiAxisZUnits()));
	}

	float MorfologiaDistanciaAteACabeca(const FPetMorphology& Corpo)
	{
		return FMath::Sqrt(
			FMath::Square(Corpo.HeadForwardUnits)
			+ FMath::Square(Corpo.HeadCenterUnits - Corpo.BodyCenterUnits));
	}
}

/**
 * A varredura do "faz sentido".
 *
 * Cada linha aqui é uma frase que alguém diria olhando o bicho na tela —
 * "a barriga arrasta", "a pata está no ar", "a cabeça soltou", "o adorno virou
 * chapéu" — virada em conta. São milhares de sementes porque o gerador só
 * falha em algumas: testar um id seria testar a sorte de ter pego o ruim.
 *
 * Limite sem varredura é limite que a primeira semente nova atravessa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetMorphologyMakesSenseAcrossSeedsTest,
	"BattleSquare.Pets.MorphologyMakesSenseAcrossSeeds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetMorphologyMakesSenseAcrossSeedsTest::RunTest(const FString& Parameters)
{
	constexpr int32 QuantidadeDeSementes = 1500;
	constexpr float MargemDaBarrigaEsperada = 4.0f;

	for (int32 Indice = 0; Indice < QuantidadeDeSementes; ++Indice)
	{
		const FString Semente = MorfologiaSementeDeTeste(Indice);
		const TCHAR* Tipo = MorfologiaTiposDeTeste[Indice % UE_ARRAY_COUNT(MorfologiaTiposDeTeste)];
		const FPetAppearance Aparencia = FPetAppearance::ForType(Tipo);

		for (const EPetGrowthStage Fase : MorfologiaFasesDeTeste)
		{
			const FPetMorphology Corpo = FPetMorphology::FromSeed(Semente, Aparencia, Fase);
			const FString Onde = FString::Printf(
				TEXT("semente %d (%s), fase %d"), Indice, Tipo, static_cast<int32>(Fase));

			const float SemiX = Corpo.BodySemiAxisXUnits();
			const float SemiY = Corpo.BodySemiAxisYUnits();
			const float SemiZ = Corpo.BodySemiAxisZUnits();

			if (!(SemiX > 1.0f && SemiY > 1.0f && SemiZ > 1.0f))
			{
				AddError(FString::Printf(TEXT("tronco degenerado em %s"), *Onde));
				return false;
			}

			// A barriga não raspa o chão. É o limite que o corpo comprido e
			// baixo estoura primeiro, e o mais fácil de ver na tela.
			if (Corpo.BodyLowestPointUnits() < MargemDaBarrigaEsperada - KINDA_SMALL_NUMBER)
			{
				AddError(FString::Printf(TEXT("a barriga arrasta no chão em %s (%.2f)"),
					*Onde, Corpo.BodyLowestPointUnits()));
				return false;
			}

			// A pata encosta no corpo: fora da elipse, a conta da superfície
			// não tem raiz real e a perna iria parar no centro do tronco.
			const float Normalizado =
				FMath::Square(Corpo.LegSpreadXUnits / SemiX)
				+ FMath::Square(Corpo.LegSpreadYUnits / SemiY);
			if (Normalizado >= 0.95f)
			{
				AddError(FString::Printf(TEXT("a pata saiu da elipse do corpo em %s (%.3f)"),
					*Onde, Normalizado));
				return false;
			}

			if (Corpo.LegHeightUnits() <= 0.0f || Corpo.LegClearanceUnits <= 0.0f)
			{
				AddError(FString::Printf(TEXT("perna sem comprimento em %s"), *Onde));
				return false;
			}

			// A cabeça encosta no tronco: no máximo 0,9 do próprio raio para
			// fora da superfície, senão ela flutua.
			const float Superficie = MorfologiaSuperficieNaDirecaoDaCabeca(Corpo);
			const float Distancia = MorfologiaDistanciaAteACabeca(Corpo);
			if (Distancia - Superficie > Corpo.HeadRadiusUnits() * 0.9f)
			{
				AddError(FString::Printf(TEXT("a cabeça soltou do corpo em %s (%.2f além de %.2f)"),
					*Onde, Distancia - Superficie, Corpo.HeadRadiusUnits()));
				return false;
			}

			// E ela não é mais larga que o tronco que a carrega.
			if (Corpo.HeadRadiusUnits() > FMath::Min(SemiY, SemiZ) * 1.45f)
			{
				AddError(FString::Printf(TEXT("cabeça maior que o tronco em %s"), *Onde));
				return false;
			}

			// O focinho sai da cabeça, não do vazio nem de dentro dela.
			const float RaioDaCabeca = Corpo.HeadRadiusUnits();
			if (Corpo.SnoutLocation.X > RaioDaCabeca * 1.05f
				|| Corpo.SnoutLocation.X < RaioDaCabeca * 0.5f)
			{
				AddError(FString::Printf(TEXT("focinho fora da cabeça em %s"), *Onde));
				return false;
			}
			if (Corpo.SnoutScale * 100.0f * 0.5f >= RaioDaCabeca)
			{
				AddError(FString::Printf(TEXT("focinho maior que a cabeça em %s"), *Onde));
				return false;
			}

			// O adorno nunca vira chapéu.
			if (Corpo.CrestScale.GetAbsMax() * 100.0f > RaioDaCabeca * 2.2f + KINDA_SMALL_NUMBER)
			{
				AddError(FString::Printf(TEXT("adorno maior que a cabeça em %s"), *Onde));
				return false;
			}

			// E a barra de vida fica SEMPRE por cima de tudo: é o que uma
			// altura fixa não consegue com bichos de alturas diferentes.
			const float Coroa = Corpo.CrownUnits();
			if (Coroa <= Corpo.BodyCenterUnits + SemiZ
				|| Coroa <= Corpo.HeadCenterUnits + RaioDaCabeca)
			{
				AddError(FString::Printf(TEXT("a barra de vida entra no bicho em %s"), *Onde));
				return false;
			}
		}
	}

	return true;
}

/**
 * O mesmo id devolve o mesmo bicho, sempre.
 *
 * Sem isto o pet mudaria de forma entre uma partida e outra — e entre a
 * máquina de quem joga e a de quem assiste.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetMorphologyIsDeterministicTest,
	"BattleSquare.Pets.MorphologyIsDeterministic",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetMorphologyIsDeterministicTest::RunTest(const FString& Parameters)
{
	const FPetAppearance Aparencia = FPetAppearance::ForType(TEXT("Dog"));

	for (int32 Indice = 0; Indice < 64; ++Indice)
	{
		const FString Semente = MorfologiaSementeDeTeste(Indice);
		const FPetMorphology Primeiro = FPetMorphology::FromSeed(Semente, Aparencia);
		const FPetMorphology Segundo = FPetMorphology::FromSeed(Semente, Aparencia);

		TestEqual(TEXT("tronco igual"), Segundo.BodyScale, Primeiro.BodyScale);
		TestEqual(TEXT("cabeça igual"), Segundo.HeadScale, Primeiro.HeadScale);
		TestEqual(TEXT("perna igual"), Segundo.LegClearanceUnits, Primeiro.LegClearanceUnits);
		TestEqual(TEXT("cauda igual"), Segundo.TailScale, Primeiro.TailScale);
		TestEqual(TEXT("adorno igual"), Segundo.CrestScale, Primeiro.CrestScale);
		TestTrue(TEXT("tom igual"), Segundo.AccentColor.Equals(Primeiro.AccentColor, 0.0f));
	}

	return true;
}

/**
 * Ids diferentes dão bichos VISIVELMENTE diferentes.
 *
 * Um gerador que varia na terceira casa decimal passa em todo teste de
 * invariante e devolve seis pets idênticos na tela — que é exatamente o
 * problema que ele foi escrito para resolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetMorphologyDistinguishesSeedsTest,
	"BattleSquare.Pets.MorphologyDistinguishesSeeds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetMorphologyDistinguishesSeedsTest::RunTest(const FString& Parameters)
{
	const FPetAppearance Aparencia = FPetAppearance::ForType(TEXT("Dog"));

	float ComprimentoMinimo = TNumericLimits<float>::Max();
	float ComprimentoMaximo = TNumericLimits<float>::Lowest();
	float PernaMinima = TNumericLimits<float>::Max();
	float PernaMaxima = TNumericLimits<float>::Lowest();
	TSet<int32> Silhuetas;

	for (int32 Indice = 0; Indice < 64; ++Indice)
	{
		const FPetMorphology Corpo =
			FPetMorphology::FromSeed(MorfologiaSementeDeTeste(Indice), Aparencia);

		ComprimentoMinimo = FMath::Min(ComprimentoMinimo, Corpo.BodyScale.X);
		ComprimentoMaximo = FMath::Max(ComprimentoMaximo, Corpo.BodyScale.X);
		PernaMinima = FMath::Min(PernaMinima, Corpo.LegClearanceUnits);
		PernaMaxima = FMath::Max(PernaMaxima, Corpo.LegClearanceUnits);

		// Silhueta grosseira: comprimento e altura da perna arredondados ao
		// que o olho separa de longe. Duas iguais aqui são dois bichos que
		// ninguém distingue no tabuleiro.
		Silhuetas.Add(FMath::RoundToInt(Corpo.BodyScale.X * 40.0f) * 1000
			+ FMath::RoundToInt(Corpo.LegClearanceUnits * 0.8f));
	}

	TestTrue(TEXT("o comprimento do corpo varia de verdade"),
		ComprimentoMaximo - ComprimentoMinimo > 0.25f);
	TestTrue(TEXT("a altura da perna varia de verdade"),
		PernaMaxima - PernaMinima > 8.0f);
	TestTrue(TEXT("64 ids não dão meia dúzia de silhuetas"), Silhuetas.Num() >= 40);

	return true;
}

/**
 * O que o gerador NÃO tem direito de mexer.
 *
 * O tom varia dentro da família do tipo, e a forma do adorno é a do tipo. Um
 * pet de Água que vira esverdeado, ou uma folha que vira chifre ao ser
 * apertada, deixam de dizer o que a tela precisa dizer — decoração comeu
 * informação.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetMorphologyKeepsTypeIdentityTest,
	"BattleSquare.Pets.MorphologyKeepsTypeIdentity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetMorphologyKeepsTypeIdentityTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Tipo : MorfologiaTiposDeTeste)
	{
		const FPetAppearance Aparencia = FPetAppearance::ForType(Tipo);
		const FVector FormaDoTipo = Aparencia.CrestScale / Aparencia.CrestScale.GetAbsMax();

		for (int32 Indice = 0; Indice < 200; ++Indice)
		{
			const FString Semente = MorfologiaSementeDeTeste(Indice);

			for (const EPetGrowthStage Fase : MorfologiaFasesDeTeste)
			{
				const FPetMorphology Corpo = FPetMorphology::FromSeed(Semente, Aparencia, Fase);

				// A forma do adorno é a proporção entre os eixos: apertar os
				// três juntos preserva; apertar só um transforma a peça.
				const FVector Forma = Corpo.CrestScale / Corpo.CrestScale.GetAbsMax();
				if (!Forma.Equals(FormaDoTipo, 0.001f))
				{
					AddError(FString::Printf(
						TEXT("o adorno de %s mudou de forma (semente %d)"), Tipo, Indice));
					return false;
				}

				// A ORDEM entre os canais é o que faz o azul ler como azul.
				const FLinearColor Base = Aparencia.AccentColor;
				const FLinearColor Tom = Corpo.AccentColor;
				if (FMath::Sign(Tom.R - Tom.G) != FMath::Sign(Base.R - Base.G)
					|| FMath::Sign(Tom.G - Tom.B) != FMath::Sign(Base.G - Base.B)
					|| FMath::Sign(Tom.R - Tom.B) != FMath::Sign(Base.R - Base.B))
				{
					AddError(FString::Printf(
						TEXT("o tom de %s saiu da família de cor (semente %d)"), Tipo, Indice));
					return false;
				}

				TestEqual(TEXT("o adorno continua opaco"), Corpo.AccentColor.A, 1.0f);
			}
		}
	}

	return true;
}

/**
 * Crescer é o MESMO bicho com outra idade, não outro bicho.
 *
 * Um segundo gerador para a evolução concordaria com o primeiro até a
 * primeira edição (L-032, L-033), e o desacordo aqui apareceria como o pet
 * virando outra criatura ao evoluir — na frente de quem acabou de criar
 * carinho por ele.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetMorphologyGrowsWithoutBecomingAnotherPetTest,
	"BattleSquare.Pets.MorphologyGrowsWithoutBecomingAnotherPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetMorphologyGrowsWithoutBecomingAnotherPetTest::RunTest(const FString& Parameters)
{
	for (int32 Indice = 0; Indice < 400; ++Indice)
	{
		const FString Semente = MorfologiaSementeDeTeste(Indice);
		const TCHAR* Tipo = MorfologiaTiposDeTeste[Indice % UE_ARRAY_COUNT(MorfologiaTiposDeTeste)];
		const FPetAppearance Aparencia = FPetAppearance::ForType(Tipo);

		const FPetMorphology Filhote =
			FPetMorphology::FromSeed(Semente, Aparencia, EPetGrowthStage::Filhote);
		const FPetMorphology Adulto =
			FPetMorphology::FromSeed(Semente, Aparencia, EPetGrowthStage::Adulto);
		const FPetMorphology Evoluido =
			FPetMorphology::FromSeed(Semente, Aparencia, EPetGrowthStage::Evoluido);

		const FString Onde = FString::Printf(TEXT("semente %d (%s)"), Indice, Tipo);

		// Neotenia: a cabeça encolhe EM RELAÇÃO ao corpo com a idade. É o
		// sinal que o olho lê como "novo" sem precisar de legenda — e é o que
		// impede o filhote de ser só o adulto reduzido.
		const float RazaoFilhote = Filhote.HeadRadiusUnits() / Filhote.BodySemiAxisYUnits();
		const float RazaoAdulto = Adulto.HeadRadiusUnits() / Adulto.BodySemiAxisYUnits();
		const float RazaoEvoluido = Evoluido.HeadRadiusUnits() / Evoluido.BodySemiAxisYUnits();
		if (!(RazaoFilhote > RazaoAdulto && RazaoAdulto > RazaoEvoluido))
		{
			AddError(FString::Printf(
				TEXT("a cabeça não encolhe com a idade em %s (%.3f, %.3f, %.3f)"),
				*Onde, RazaoFilhote, RazaoAdulto, RazaoEvoluido));
			return false;
		}

		// E o bicho nunca DIMINUI ao envelhecer.
		if (Filhote.LegClearanceUnits > Adulto.LegClearanceUnits
			|| Adulto.LegClearanceUnits > Evoluido.LegClearanceUnits)
		{
			AddError(FString::Printf(TEXT("a perna encolheu com a idade em %s"), *Onde));
			return false;
		}
		if (Filhote.CrownUnits() > Adulto.CrownUnits()
			|| Adulto.CrownUnits() > Evoluido.CrownUnits())
		{
			AddError(FString::Printf(TEXT("o bicho ficou mais baixo com a idade em %s"), *Onde));
			return false;
		}
		if (Filhote.CrestScale.GetAbsMax() > Adulto.CrestScale.GetAbsMax()
			|| Adulto.CrestScale.GetAbsMax() > Evoluido.CrestScale.GetAbsMax())
		{
			AddError(FString::Printf(TEXT("o adorno encolheu com a idade em %s"), *Onde));
			return false;
		}

		// O focinho encurta na infância: a outra metade da neotenia.
		const float FocinhoFilhote = Filhote.SnoutScale / Filhote.HeadScale;
		const float FocinhoEvoluido = Evoluido.SnoutScale / Evoluido.HeadScale;
		if (!(FocinhoFilhote < FocinhoEvoluido))
		{
			AddError(FString::Printf(TEXT("o focinho não cresce com a idade em %s"), *Onde));
			return false;
		}

		// E continua sendo o MESMO pet: mesma forma de adorno, mesmo tombo.
		const FVector FormaFilhote = Filhote.CrestScale / Filhote.CrestScale.GetAbsMax();
		const FVector FormaEvoluida = Evoluido.CrestScale / Evoluido.CrestScale.GetAbsMax();
		if (!FormaFilhote.Equals(FormaEvoluida, 0.001f))
		{
			AddError(FString::Printf(TEXT("o adorno mudou de forma ao crescer em %s"), *Onde));
			return false;
		}
		TestEqual(TEXT("o tombo do adorno não muda com a idade"),
			Evoluido.CrestRotation.Roll, Filhote.CrestRotation.Roll);

		// A cor escurece com a idade, dentro da mesma família.
		if (!(Evoluido.AccentColor.GetLuminance() < Filhote.AccentColor.GetLuminance()))
		{
			AddError(FString::Printf(TEXT("o filhote não é mais pálido que o evoluído em %s"), *Onde));
			return false;
		}
	}

	return true;
}

/**
 * Pet sem id ainda é um pet — e ele PRECISA aparecer na tela.
 *
 * Devolver corpo vazio para semente vazia daria um bicho invisível, que é o
 * defeito que este projeto já cometeu três vezes.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetMorphologyWithoutSeedIsStillABodyTest,
	"BattleSquare.Pets.MorphologyWithoutSeedIsStillABody",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetMorphologyWithoutSeedIsStillABodyTest::RunTest(const FString& Parameters)
{
	const FPetAppearance Aparencia = FPetAppearance::ForType(TEXT("Dog"));
	const FPetMorphology Corpo = FPetMorphology::FromSeed(FString(), Aparencia);

	TestTrue(TEXT("o tronco existe"), Corpo.BodySemiAxisXUnits() > 1.0f);
	TestTrue(TEXT("a cabeça existe"), Corpo.HeadRadiusUnits() > 1.0f);
	TestTrue(TEXT("a perna existe"), Corpo.LegHeightUnits() > 1.0f);
	TestTrue(TEXT("a barra fica acima do bicho"),
		Corpo.CrownUnits() > Corpo.HeadCenterUnits + Corpo.HeadRadiusUnits());
	TestEqual(TEXT("o adorno é o do tipo"), Corpo.CrestScale, Aparencia.CrestScale);

	return true;
}

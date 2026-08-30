// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ScenaryPalette.h"

#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace PaletaDoCenario
{
	/**
	 * O material da engine que aceita o parâmetro `Color`.
	 *
	 * O mesmo que já pinta pets, donos e inimigos do mundo. Reusar o caminho
	 * comprovado vale mais que um material próprio: este aparece na tela.
	 */
	const TCHAR* MaterialColorivel =
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	/** O nome do parâmetro de cor daquele material. */
	const TCHAR* ParametroDeCor = TEXT("Color");

	// A escada de verdes. O que separa um degrau do outro não é só o matiz: é
	// o BRILHO. Verde claro contra verde escuro se distingue de longe, na tela
	// pequena e para quem enxerga cor de um jeito diferente; dois verdes de
	// mesmo brilho em matizes vizinhos viram a mesma mancha, que foi
	// exatamente o defeito relatado.

	/** Chão da mata: o mais escuro de todos, para tudo se destacar CONTRA ele. */
	const FLinearColor VerdeDoChao(0.055f, 0.098f, 0.043f);
	/** Capim rasteiro: o mais claro e o mais amarelado — quase limão. */
	const FLinearColor VerdeDoCapim(0.404f, 0.612f, 0.129f);
	/** Arbusto: escuro e azulado, o oposto do capim na mesma família. */
	const FLinearColor VerdeDoArbusto(0.086f, 0.298f, 0.169f);
	/** Copa da árvore da mata: médio, entre o capim e o arbusto. */
	const FLinearColor VerdeDaMata(0.180f, 0.443f, 0.137f);
	/** Copa do dossel: quase preto-esverdeado, fecha o fundo. */
	const FLinearColor VerdeDoDossel(0.043f, 0.169f, 0.106f);
	/** Musgo sobre pedra: puxa para o amarelo, para não virar a pedra. */
	const FLinearColor VerdeDoMusgo(0.239f, 0.353f, 0.106f);

	// Madeira e terra — a faixa quente, que dá o contraste que faltava.

	/** Casca de árvore. */
	const FLinearColor MarromDaCasca(0.231f, 0.129f, 0.063f);
	/** Casca de pinheiro, mais escura. */
	const FLinearColor MarromEscuro(0.129f, 0.071f, 0.035f);
	/** Miolo do tronco cortado — o claro que faz o toco ser visto. */
	const FLinearColor MadeiraExposta(0.545f, 0.361f, 0.176f);
	/** Terra. */
	const FLinearColor MarromDaTerra(0.239f, 0.157f, 0.086f);

	/**
	 * Pedra: cinza FRIO e dessaturado.
	 *
	 * Nenhum verde e nenhum marrom, de propósito — a pedra é a única coisa do
	 * cenário que não é nem folha nem madeira, e é assim que ela se anuncia.
	 */
	const FLinearColor CinzaDaPedra(0.318f, 0.325f, 0.341f);

	/**
	 * Rocha da serra: o mesmo cinza puxado para o escuro e para o azul.
	 *
	 * A serra está a quilômetros, e a perspectiva aérea escurece e azula tudo
	 * que está longe. Pintá-la com o cinza da pedra de perto a traria para o
	 * primeiro plano — o quadro perderia profundidade justamente onde ela
	 * deveria aparecer.
	 */
	const FLinearColor CinzaDaSerra(0.196f, 0.216f, 0.259f);

	/** Gelo do cume: branco frio, nunca puro — branco puro estoura na tela. */
	const FLinearColor BrancoDoGelo(0.878f, 0.914f, 0.949f);

	/**
	 * A encosta em que se anda: cinza de perto, um passo mais escuro que a
	 * pedra solta, para a trilha ter contra o que se destacar.
	 */
	const FLinearColor CinzaDaEncosta(0.263f, 0.271f, 0.286f);

	/**
	 * Terra batida da trilha.
	 *
	 * A diferença que importa aqui é de LUMINÂNCIA, não de matiz: contra a
	 * encosta em 0,27 esta terra está perto de 0,44, e é isso que faz o
	 * caminho aparecer de longe. Trocar por um marrom escuro "mais natural"
	 * some com a trilha e devolve a montanha à condição de parede.
	 */
	const FLinearColor TerraDaTrilha(0.514f, 0.388f, 0.243f);

	/** Rocha da caverna: o mais escuro da paleta, porque é dentro. */
	const FLinearColor CinzaDaCaverna(0.114f, 0.118f, 0.129f);

	/**
	 * Chão da caverna.
	 *
	 * Claro o bastante para o corredor se separar da parede — num labirinto
	 * sem essa separação, quem entra não enxerga a próxima curva e o labirinto
	 * deixa de ser percorrível para virar tela preta.
	 */
	const FLinearColor ChaoDaCaverna(0.353f, 0.310f, 0.259f);

	// Acentos: pontos pequenos, saturados, que só funcionam porque o resto
	// do quadro é dessaturado.

	const FLinearColor VermelhoDaFlor(0.741f, 0.106f, 0.118f);
	const FLinearColor AmareloDaFlor(0.867f, 0.667f, 0.098f);
	const FLinearColor CremeDoCogumelo(0.831f, 0.788f, 0.671f);

	/** Os slots de madeira do pacote Kenney, medidos nos `.uasset`. */
	bool EhSlotDeMadeira(FName Slot)
	{
		return Slot == FName(TEXT("woodBark"))
			|| Slot == FName(TEXT("woodBarkDark"));
	}

	/** O slot do miolo exposto de tronco e toco. */
	bool EhSlotDeMioloDeMadeira(FName Slot)
	{
		return Slot == FName(TEXT("woodInner"));
	}

	/** O slot de terra, que na pedra é a própria rocha. */
	bool EhSlotDeTerra(FName Slot)
	{
		return Slot == FName(TEXT("dirt"));
	}

	/** Os slots de folhagem. */
	bool EhSlotDeFolha(FName Slot)
	{
		return Slot == FName(TEXT("leafsGreen"))
			|| Slot == FName(TEXT("leafsDark"));
	}
}

FLinearColor ScenaryPalette::GroundColor()
{
	return PaletaDoCenario::VerdeDoChao;
}

FLinearColor ScenaryPalette::ColorFor(EScenaryRole Role, FName MaterialSlot)
{
	using namespace PaletaDoCenario;

	// A madeira é madeira em qualquer papel: um tronco de árvore do dossel e
	// um tronco caído no chão são a mesma matéria, e pintá-los de cores
	// diferentes só confundiria quem tenta ler o que é o quê.
	if (EhSlotDeMioloDeMadeira(MaterialSlot)) { return MadeiraExposta; }
	if (MaterialSlot == FName(TEXT("woodBarkDark"))) { return MarromEscuro; }
	if (EhSlotDeMadeira(MaterialSlot)) { return MarromDaCasca; }

	// Flor e cogumelo trazem o próprio acento no nome do slot.
	if (MaterialSlot == FName(TEXT("colorRed"))) { return VermelhoDaFlor; }
	if (MaterialSlot == FName(TEXT("colorYellow"))) { return AmareloDaFlor; }

	switch (Role)
	{
	case EScenaryRole::GroundCover:
		return VerdeDoCapim;

	case EScenaryRole::Undergrowth:
		return VerdeDoArbusto;

	case EScenaryRole::Accent:
		// O caule da flor é verde de capim; o chapéu do cogumelo, que chega
		// no slot genérico do pacote, é creme.
		return EhSlotDeFolha(MaterialSlot) || MaterialSlot == FName(TEXT("grass"))
			? VerdeDoCapim : CremeDoCogumelo;

	case EScenaryRole::DeadWood:
		return MarromDaCasca;

	case EScenaryRole::Rock:
		// A pedra do pacote chega com `dirt` (a rocha) e `grass` (o musgo em
		// cima). São as duas metades do MESMO asset, e é por elas que a pedra
		// deixa de ser um bloco bege: cinza embaixo, musgo em cima.
		return EhSlotDeTerra(MaterialSlot) ? CinzaDaPedra : VerdeDoMusgo;

	case EScenaryRole::ForestTree:
		return VerdeDaMata;

	case EScenaryRole::CanopyTree:
		return VerdeDoDossel;

	case EScenaryRole::MountainRock:
		return CinzaDaSerra;

	case EScenaryRole::MountainSnow:
		return BrancoDoGelo;

	case EScenaryRole::ClimbableRock:
		return CinzaDaEncosta;

	case EScenaryRole::MountainTrail:
		return TerraDaTrilha;

	case EScenaryRole::CaveRock:
		return CinzaDaCaverna;

	case EScenaryRole::CaveFloor:
		return ChaoDaCaverna;
	}

	return VerdeDaMata;
}

UMaterialInterface* ScenaryPalette::ColorableBaseMaterial()
{
	return LoadObject<UMaterialInterface>(
		nullptr, PaletaDoCenario::MaterialColorivel);
}

int32 ScenaryPalette::PaintComponent(UPrimitiveComponent* Component, EScenaryRole Role)
{
	if (!Component) { return 0; }

	UMaterialInterface* Base = ColorableBaseMaterial();
	if (!Base) { return 0; }

	const TArray<FName> Slots = Component->GetMaterialSlotNames();
	int32 Pintados = 0;

	for (int32 Indice = 0; Indice < Slots.Num(); ++Indice)
	{
		UMaterialInstanceDynamic* Tinta =
			Component->CreateDynamicMaterialInstance(Indice, Base);
		if (!Tinta) { continue; }

		Tinta->SetVectorParameterValue(
			PaletaDoCenario::ParametroDeCor, ColorFor(Role, Slots[Indice]));
		++Pintados;
	}

	return Pintados;
}

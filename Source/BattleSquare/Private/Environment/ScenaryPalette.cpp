// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ScenaryPalette.h"
#include "Misc/ConfigCacheIni.h"

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

	// Os biomas de fora da mata. Cada um se anuncia pela TEMPERATURA da cor
	// antes de se anunciar pelo que tem plantado: quem chega ao deserto vê
	// ocre antes de reparar que não há árvore.

	/** Areia do deserto: ocre, quente, e mais CLARA que qualquer verde. */
	const FLinearColor OcreDaAreia(0.702f, 0.549f, 0.310f);

	/**
	 * Pedra do deserto.
	 *
	 * O mesmo cinza da mata puxado para o quente, e um degrau mais claro. Não
	 * pode ser o cinza frio: contra a areia ocre ele lê como sujeira, e a
	 * pedra some — que é o oposto do que uma pedra no deserto faz.
	 */
	const FLinearColor BegeDaPedraSeca(0.478f, 0.408f, 0.322f);

	/**
	 * O gelo do chão da geleira: branco AZUL.
	 *
	 * Mais escuro que a touca do cume de propósito. Gelo de piso com o branco
	 * do cume estoura a tela inteira, e sem nada mais escuro por perto quem
	 * anda perde a noção de onde o chão acaba.
	 */
	const FLinearColor AzulDoGelo(0.706f, 0.792f, 0.859f);

	/**
	 * Basalto do vulcão: o mais escuro que existe fora da caverna — e ainda
	 * assim NÃO preto.
	 *
	 * Preto de verdade encosta na luminância do chão da mata, e a fronteira
	 * entre a floresta e o vulcão viraria uma mancha escura só. Escuro o
	 * bastante para a lava ter contra o que brilhar, claro o bastante para se
	 * ler como outro lugar.
	 */
	const FLinearColor PretoDoBasalto(0.176f, 0.145f, 0.141f);

	/**
	 * A lava: laranja quente, quase branco no verde.
	 *
	 * O material da engine não emite luz — a cor é tudo o que há. Por isso ela
	 * é empurrada ao alto da escala de brilho em vez de ao vermelho profundo
	 * que a lava tem de perto: contra o basalto em 0,16, o vermelho escuro
	 * simplesmente não apareceria, e uma lava que não aparece não é lava.
	 */
	const FLinearColor LaranjaDaLava(0.973f, 0.478f, 0.106f);

	/** A fumaça: cinza puxando para o quente, porque nasce do fogo. */
	const FLinearColor CinzaDaFumaca(0.545f, 0.522f, 0.510f);

	/** A poça da caverna: azul quase preto, do fundo que não recebe luz. */
	const FLinearColor AzulDaPocaEscura(0.043f, 0.153f, 0.196f);

	/**
	 * A areia SECA da praia: fria e um degrau mais escura que a duna do
	 * deserto.
	 *
	 * A diferença não é enfeite: é ela que separa a beira do deserto quando os
	 * dois se encostam, que acontece em toda volta da ilha.
	 */
	const FLinearColor PalidoDaPraia(0.694f, 0.655f, 0.549f);

	/**
	 * A areia MOLHADA, na beira: a mesma areia com um terço da claridade.
	 *
	 * Um terço, e não um retoque: a faixa tem menos de meio pedaço de largura
	 * e é vista de longe, de cima e de lado. Diferença sutil a essa distância
	 * é diferença que não existe — e a faixa serve justamente para se ver de
	 * longe onde a água começa.
	 */
	const FLinearColor ParadoDaAreiaMolhada(0.427f, 0.412f, 0.361f);

	/** A espuma: o branco mais alto da paleta, com um sopro de azul. */
	const FLinearColor BrancoDaEspuma(0.925f, 0.949f, 0.949f);

	/**
	 * A água doce: turquesa clara, a única água clara da ilha.
	 *
	 * O mar da borda é (0,06 / 0,16 / 0,26) e o charco é verde parado; esta
	 * fica acima dos dois em brilho e puxa para o ciano. Contra a grama, que é
	 * verde médio, ela precisa desse degrau de claridade — água doce escura
	 * sobre mato lê como sombra de árvore, não como rio.
	 */
	const FLinearColor TurquesaDoRio(0.235f, 0.514f, 0.545f);

	/**
	 * O verde da aurora, e o roxo do alto dela.
	 *
	 * Os dois são os valores mais SATURADOS da paleta inteira, de propósito: sem
	 * canal de emissão, o único jeito de a cortina se sustentar contra o céu de
	 * noite é a cor pura. Qualquer cinza misturado aqui a apaga.
	 */
	const FLinearColor VerdeDaAurora(0.302f, 0.945f, 0.545f);
	const FLinearColor RoxoDaAurora(0.545f, 0.361f, 0.882f);

	/**
	 * A lama do brejo: escura, parda e quase sem saturação.
	 *
	 * Fica em 0,10 de luminância contra os 0,22 do chão de mata. É uma
	 * distância grande de propósito — a divisa entre os dois é uma faixa
	 * larga em volta da ilha inteira, e divisa que só se nota de perto não
	 * cumpre o papel de dizer onde se está.
	 */
	const FLinearColor PardoDaLama(0.153f, 0.129f, 0.094f);

	/** A poça parada: verde escuro, o oposto do azul do mar. */
	const FLinearColor VerdeDaPoca(0.184f, 0.243f, 0.161f);

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

	case EScenaryRole::DesertSand:
		return OcreDaAreia;

	case EScenaryRole::DesertRock:
		return BegeDaPedraSeca;

	case EScenaryRole::GlacierIce:
		return AzulDoGelo;

	case EScenaryRole::VolcanicRock:
		return PretoDoBasalto;

	case EScenaryRole::LavaGlow:
		return LaranjaDaLava;

	case EScenaryRole::BeachSand:
		return PalidoDaPraia;

	case EScenaryRole::SwampMud:
		return PardoDaLama;

	case EScenaryRole::SwampWater:
		return VerdeDaPoca;

	case EScenaryRole::AshPlume:
		return CinzaDaFumaca;

	case EScenaryRole::CaveWater:
		return AzulDaPocaEscura;

	case EScenaryRole::WetSand:
		return ParadoDaAreiaMolhada;

	case EScenaryRole::WaterFoam:
		return BrancoDaEspuma;

	case EScenaryRole::FreshWater:
		return TurquesaDoRio;

	case EScenaryRole::AuroraVeil:
		return VerdeDaAurora;

	case EScenaryRole::AuroraCrown:
		return RoxoDaAurora;

	case EScenaryRole::Count:
		break;
	}

	return VerdeDaMata;
}

const TCHAR* ScenaryPalette::RoleConfigKey(EScenaryRole Role)
{
	// Chave estavel por papel — muda o rotulo do papel a vontade, a chave de
	// config do asset nao se mexe. So os papeis que um pacote de arte costuma
	// cobrir; o resto usa a primitiva.
	switch (Role)
	{
	case EScenaryRole::ForestTree:  return TEXT("Mesh_ForestTree");
	case EScenaryRole::CanopyTree:  return TEXT("Mesh_CanopyTree");
	case EScenaryRole::Rock:        return TEXT("Mesh_Rock");
	case EScenaryRole::DeadWood:    return TEXT("Mesh_DeadWood");
	case EScenaryRole::Undergrowth: return TEXT("Mesh_Undergrowth");
	case EScenaryRole::Accent:      return TEXT("Mesh_Accent");
	case EScenaryRole::GroundCover: return TEXT("Mesh_GroundCover");
	default:                        return TEXT("");
	}
}

FString ScenaryPalette::MeshPathForRole(EScenaryRole Role, EScenaryPrimitive Fallback)
{
	const TCHAR* Chave = RoleConfigKey(Role);
	if (Chave && *Chave)
	{
		FString Override;
		if (GConfig->GetString(TEXT("/Script/BattleSquare.Art"), Chave, Override, GGameIni)
			&& !Override.IsEmpty())
		{
			return Override;
		}
	}
	// Sem override: a primitiva de sempre. Verde sem pacote nenhum.
	return PrimitiveMeshPath(Fallback);
}

const TCHAR* ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive Primitive)
{
	// A fonte unica dos caminhos de primitiva (MV2/MV3). /Engine/BasicShapes/*
	// ate alguem decidir o pacote (invariante 20); trocar o pacote e trocar aqui.
	switch (Primitive)
	{
	case EScenaryPrimitive::Cube:     return TEXT("/Engine/BasicShapes/Cube.Cube");
	case EScenaryPrimitive::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	case EScenaryPrimitive::Sphere:   return TEXT("/Engine/BasicShapes/Sphere.Sphere");
	case EScenaryPrimitive::Cone:     return TEXT("/Engine/BasicShapes/Cone.Cone");
	}
	return TEXT("/Engine/BasicShapes/Cube.Cube");
}

const TCHAR* ScenaryPalette::ColorableBaseMaterialPath()
{
	return PaletaDoCenario::MaterialColorivel;
}

UMaterialInterface* ScenaryPalette::ColorableBaseMaterial()
{
	return LoadObject<UMaterialInterface>(
		nullptr, PaletaDoCenario::MaterialColorivel);
}

int32 ScenaryPalette::PaintComponent(UPrimitiveComponent* Component, EScenaryRole Role)
{
	return TintComponent(Component, Role, 1.0f);
}

int32 ScenaryPalette::TintComponent(
	UPrimitiveComponent* Component, EScenaryRole Role, float Brightness)
{
	if (!Component) { return 0; }

	UMaterialInterface* Base = ColorableBaseMaterial();
	if (!Base) { return 0; }

	const TArray<FName> Slots = Component->GetMaterialSlotNames();
	const float Brilho = FMath::Max(0.0f, Brightness);
	int32 Pintados = 0;

	for (int32 Indice = 0; Indice < Slots.Num(); ++Indice)
	{
		UMaterialInstanceDynamic* Tinta =
			Cast<UMaterialInstanceDynamic>(Component->GetMaterial(Indice));
		if (!Tinta)
		{
			Tinta = Component->CreateDynamicMaterialInstance(Indice, Base);
		}

		if (!Tinta) { continue; }

		Tinta->SetVectorParameterValue(
			PaletaDoCenario::ParametroDeCor, ColorFor(Role, Slots[Indice]) * Brilho);
		++Pintados;
	}

	return Pintados;
}

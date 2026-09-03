// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/Pantheon.h"

/**
 * O que ocupa o chão entre as trilhas.
 *
 * A ilha tinha assentamento, água e marco — e entre eles, mata igual em toda
 * parte. Mata uniforme não é floresta: é textura. Quem anda por ela não sabe
 * se andou cem metros ou mil.
 */
UENUM()
enum class EGroundUse : uint8
{
	/** Nada de especial: a mata comum. */
	Nenhum,

	/**
	 * BOSQUE: mata fechada, mais densa que o resto.
	 *
	 * É o que dá referência ao caminhar — sair do aberto e entrar no escuro é
	 * a menor unidade de "cheguei em outro lugar" que existe sem placa.
	 */
	Bosque,

	/**
	 * CLAREIRA FECHADA: o vazio cercado de mata por todos os lados.
	 *
	 * O contrário do bosque, e vale pelo mesmo motivo. Ela é FECHADA de
	 * propósito — nenhuma trilha entra. Achá-la é acidente, e é isso que faz
	 * dela um lugar em vez de um pátio.
	 */
	ClareiraFechada,

	/**
	 * FAZENDA: o chão trabalhado, perto de uma vila e perto da água.
	 *
	 * Ela é a razão pela qual a vila come, e é o primeiro sinal de que alguém
	 * mora ali — quem chega vê roçado antes de telhado.
	 */
	Fazenda,

	/**
	 * CRIADOURO: a fazenda de pets, LONGE da vila.
	 *
	 * A distância é a razão de ser dela, não um detalhe: criar pet perto de
	 * gente é criar pet manso, e quem cria para valer procura sossego e espaço.
	 * Um criadouro na esquina da praça seria só mais um prédio.
	 *
	 * E ela dá ao mapa uma coisa que faltava: um destino que não é vila, não é
	 * marco natural e não é fronteira. Um lugar onde mora alguém sozinho, e que
	 * só existe porque alguém foi até lá.
	 */
	Criadouro,

	/**
	 * LOJA de beira de estrada, e ela mora no CRUZAMENTO.
	 *
	 * A posição não é enfeite, é a razão dela existir: comércio nasce onde dois
	 * caminhos se encontram, porque é onde passa gente das duas direções. Loja
	 * no meio de uma trilha é loja com metade dos fregueses.
	 */
	Loja,

	/**
	 * ACAMPAMENTO: parada de quem está no meio do caminho.
	 *
	 * Fica LONGE das vilas e PERTO da trilha — as duas coisas ao mesmo tempo,
	 * e é a combinação que o define. Perto da vila ninguém acampa: dorme-se na
	 * vila. Longe da trilha ninguém acha.
	 */
	Acampamento,

	/**
	 * POMAR CUIDADO: a fruta que alguém plantou, ao lado da fazenda.
	 *
	 * Pomar leva anos para dar, e por isso ele só existe onde alguém ficou. Ele
	 * fica colado na lavoura porque é a mesma pessoa que cuida dos dois.
	 */
	Pomar,

	/**
	 * POMAR SELVAGEM: a fruta que ninguém plantou.
	 *
	 * Ele é o oposto do outro e vale pelo contraste: fica longe de tudo, e
	 * achá-lo é sorte. É o que faz valer a pena sair da trilha.
	 */
	PomarSelvagem,

	/**
	 * DECK: o atracadouro com barcos, na margem.
	 *
	 * Ele existe porque a água virou CAMINHO neste mundo — há balsa, há barco
	 * grande e pequeno, e há rio que liga tudo. Sem um lugar para atracar, o
	 * barco não tem de onde sair, e a navegabilidade fica sendo uma tabela que
	 * ninguém usa.
	 *
	 * Fica onde a água aceita barco grande, e de preferência perto de trilha:
	 * deck que ninguém alcança por terra é deck que só serve a quem já está
	 * navegando.
	 */
	Deck,

	/**
	 * POÇO ARTESIANO: a aposta de quem cavou.
	 *
	 * Ele é o outro jeito de a vila beber, ao lado do aqueduto — e o barato:
	 * aqueduto é obra de cidade, poço é obra de quem tem uma pá.
	 *
	 * E ele pode dar SECO. Isso não é crueldade nem sorteio no vácuo: a chance
	 * sai de quão fundo está o lençol, e o lençol acompanha a água de
	 * superfície e a altura do chão. Poço no baixio perto do rio quase sempre
	 * dá; poço no alto e longe quase nunca.
	 *
	 * O jogador pode aprender isso olhando o mapa, e é o que separa uma aposta
	 * de um dado.
	 */
	Poco,

	/**
	 * TEMPLO: a casa de um deus, no lugar que ele governa.
	 *
	 * A posição não é decoração, é a identidade dele: o do monte fica no monte,
	 * o da água na cachoeira, o do fogo na beira da rocha queimada. Quem vê um
	 * templo sabe de quem ele é pelo lugar — e é assim que um panteão se ensina
	 * sem texto.
	 */
	Templo,

	/**
	 * TEMPLO EM RUÍNAS: a casa de um deus que ninguém visita mais.
	 *
	 * Ele é o oposto do outro em tudo o que importa: fica ESCONDIDO — dentro de
	 * bosque fechado, em clareira que trilha nenhuma toca, ou em caverna — e
	 * achá-lo é acidente.
	 *
	 * E é ele que dá ao mundo uma coisa que nenhuma outra peça dá: passado.
	 * Um lugar que já foi importante e deixou de ser conta uma história que
	 * ninguém precisou escrever.
	 */
	Ruina,

	/**
	 * CEMITÉRIO da vila: infraestrutura, e a mais silenciosa que existe.
	 *
	 * Fica PERTO do assentamento e na direção OPOSTA à da água. Não é
	 * superstição: ninguém enterra rio acima de onde bebe, e essa é uma regra
	 * sanitária real, velha como as vilas. Aqui ela vira uma amarra que o
	 * gerador consegue verificar.
	 *
	 * O que ele diz a quem chega é curto e não precisa de texto: quem mora
	 * aqui já morreu aqui. Uma vila sem cemitério é um acampamento.
	 */
	Cemiterio,

	/**
	 * CEMITÉRIO ESQUECIDO: alguém foi enterrado onde não há mais vila nenhuma.
	 *
	 * É o único que carrega história em vez de função, e por isso ele nasce
	 * perto das RUÍNAS: um templo caído com um cemitério ao lado conta que ali
	 * houve gente, e conta melhor do que qualquer placa contaria.
	 *
	 * O cemitério da vila e este não são o mesmo lugar em tamanhos diferentes
	 * — um é serviço, o outro é vestígio.
	 */
	CemiterioEsquecido,

	/**
	 * MERCADO-NEGRO: o lugar onde se vende o que não se declara.
	 *
	 * É LUGAR, não estado (K3), e é a diferença que importa: um estado
	 * viajaria com o vendedor e não custaria nada achar. Sendo lugar, ele tem
	 * posição, e chegar até ela é o preço de vender um pet roubado.
	 *
	 * Fica ao LADO da trilha e LONGE da vila, as duas coisas ao mesmo tempo.
	 * Sem freguês não há mercado; com vizinho não há negro.
	 *
	 * E ele é ESCONDIDO: a carta o CONTA e não o aponta. Um mercado-negro que
	 * o gabarito anuncia é um mercado-negro com placa.
	 */
	MercadoNegro
};

struct BATTLESQUARE_API FGroundUsePatch
{
	EGroundUse Use = EGroundUse::Nenhum;
	FVector2D CenterUnits = FVector2D::ZeroVector;
	float HalfExtentUnits = 0.0f;

	/**
	 * O poço deu água. Só faz sentido para `Poco`, e é falso no resto.
	 *
	 * Fica no traçado e não no ator porque o mapa precisa saber: um poço seco
	 * desenhado igual a um cheio é uma promessa que a carta não cumpre.
	 */
	bool bYieldsWater = false;

	/** De que deus, quando é templo ou ruína. */
	EDeity Deity = EDeity::MaeNatureza;
};

/**
 * O uso do solo — PURO, como todo o resto do traçado deste mundo.
 *
 * As posições saem do que JÁ EXISTE: fazenda nasce ao lado de uma vila e
 * virada para a água; clareira nasce longe de tudo; bosque nasce entre os
 * marcos. Nenhuma delas é uma coordenada escrita à mão, e é isso que faz o
 * traçado continuar certo quando a ilha mudar de tamanho.
 */
namespace LandUseLayout
{
	BATTLESQUARE_API TArray<FGroundUsePatch> Plan();

	/**
	 * O AFASTAMENTO MÍNIMO entre dois mercados-negros.
	 *
	 * "Bem espalhados" é medição, e a medida mora aqui — num lugar só, porque
	 * um limiar no gerador e outro no teste concordam até a primeira edição, e
	 * a partir dali o teste passa a afirmar uma regra que o mundo não segue.
	 *
	 * Derivado do RAIO DA ILHA, nunca escrito em unidades: número absoluto
	 * escolhido quando só existia um tamanho é a armadilha mais cara deste
	 * projeto.
	 */
	BATTLESQUARE_API float BlackMarketSpreadUnits();

	/** O que este ponto é. */
	BATTLESQUARE_API EGroundUse UseAt(const FVector2D& PositionUnits);

	/**
	 * Aqui não se planta árvore.
	 *
	 * Vale para fazenda e para clareira fechada — as duas são vazios, por
	 * motivos opostos. O bosque é o contrário: ali se planta MAIS.
	 */
	BATTLESQUARE_API bool BlocksPlanting(const FVector2D& PositionUnits);

	/**
	 * Quanto a densidade da mata muda aqui, como multiplicador.
	 *
	 * Um no chão comum, mais no bosque, zero onde não se planta. É um número
	 * só para o gerador da mata multiplicar, em vez de ele aprender as regras
	 * de uso do solo — que não são dele.
	 */
	BATTLESQUARE_API float PlantingDensityAt(const FVector2D& PositionUnits);
}

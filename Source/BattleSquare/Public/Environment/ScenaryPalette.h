// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UPrimitiveComponent;

/**
 * Papel de uma espécie na LEITURA do cenário.
 *
 * Não é o que a planta é, é o que ela precisa dizer na tela. Duas espécies
 * podem chegar do pacote com a mesma malha e o mesmo material e ainda assim
 * precisar de cores diferentes — foi exatamente o caso aqui: o arbusto e o
 * capim rasteiro têm ambos o slot `grass`, e enquanto a cor vinha do pacote
 * eram o mesmo verde. Quem olha não distingue arbusto de capim, e o cenário
 * inteiro lê como uma mancha só.
 */
/**
 * A PRIMITIVA da engine de onde uma malha vem (a-malha-vem-de-fora, MV2/MV3).
 *
 * Escopo global com base explicita (a armadilha do enum incompleto ja mordeu
 * este projeto). Os quatro solidos que os ~20 atores usam; o CAMINHO de cada um
 * mora num lugar so (ScenaryPalette::PrimitiveMeshPath), ao lado da cor — nunca
 * mais espalhado por 35 literais (invariante 18, a fonte unica do visual).
 */
enum class EScenaryPrimitive : uint8
{
	Cube,
	Cylinder,
	Sphere,
	Cone,
};

enum class EScenaryRole : uint8
{
	/** Capim e folha rasteira, logo atrás da grade. */
	GroundCover,
	/** Arbusto e planta de volume baixo. */
	Undergrowth,
	/** Flor e cogumelo — o acento de cor. */
	Accent,
	/** Tronco caído e toco: madeira sem folha. */
	DeadWood,
	/** Pedra. */
	Rock,
	/** Árvore da mata, ainda inteira no quadro. */
	ForestTree,
	/** Árvore do dossel, que fecha o fundo. */
	CanopyTree,

	/**
	 * Rocha da serra do horizonte.
	 *
	 * Separada de `Rock` porque a distância é informação: a pedra a dois
	 * metros da grade é objeto, e a mesma cor a sete quilômetros faria a serra
	 * parecer perto. Ela é mais escura e mais azul — é assim que o olho lê
	 * "longe" antes de qualquer neblina.
	 */
	MountainRock,

	/** O gelo do cume. */
	MountainSnow,

	/**
	 * A encosta da montanha em que se PODE andar — a que fica na ilha.
	 *
	 * Separada de `MountainRock` pelo mesmo motivo que aquela se separou de
	 * `Rock`: a serra do horizonte é fundo, e esta é chão. Fundo e chão com a
	 * mesma cor fazem a montanha perto parecer recorte de cartaz.
	 */
	ClimbableRock,

	/**
	 * A trilha que sobe a encosta.
	 *
	 * É a única cor deste arquivo cuja função é ser ACHADA. Terra batida
	 * clara contra rocha escura: quem chega na base precisa ver por onde se
	 * sobe antes de encostar na montanha, senão a montanha lê como parede.
	 */
	MountainTrail,

	/** A rocha de dentro da caverna — mais escura que tudo, porque é dentro. */
	CaveRock,

	/** O chão da caverna, claro o bastante para o corredor se ler. */
	CaveFloor,

	/**
	 * A areia do DESERTO: ocre quente, seca.
	 *
	 * Separada da areia da praia porque a diferença entre as duas é o que
	 * conta a travessia. Uma cor só para "areia" faria o deserto e a beira
	 * lerem como o mesmo lugar, e quem anda do meio da ilha até o mar não
	 * veria que chegou.
	 */
	DesertSand,

	/** Pedra do deserto: o cinza da mata puxado para o quente e para o claro. */
	DesertRock,

	/**
	 * O gelo do CHÃO da geleira.
	 *
	 * Não é `MountainSnow`: aquele é a touca do cume, vista de longe e contra
	 * o céu; este é piso, visto de perto e contra os pés. Pintar os dois com o
	 * mesmo branco faria a geleira parecer um cume deitado.
	 */
	GlacierIce,

	/** Rocha do vulcão: basalto, o mais escuro que existe fora da caverna. */
	VolcanicRock,

	/**
	 * A lava: o único papel da paleta que EMITE em vez de refletir.
	 *
	 * Ela não é "vermelho de vulcão": é o laranja mais claro e mais saturado
	 * que existe aqui, e é assim de propósito. O basalto ao redor está em 0,16
	 * de luminância; sem esta distância de brilho a cratera vira uma mancha
	 * escura dentro de outra mancha escura, e o vulcão perde justamente o que
	 * o faz ser um vulcão e não um morro preto.
	 */
	LavaGlow,

	/** A areia MOLHADA da praia: fria, e mais escura que a duna do deserto. */
	BeachSand,

	/**
	 * A lama do pântano: o chão mais escuro que existe ao ar livre.
	 *
	 * Ela precisa ser mais escura que a mata, e não só mais parda: o pântano
	 * é a mata que não drenou, e os dois se encostam numa faixa larga em volta
	 * de toda a ilha. Se o chão dos dois tiver a mesma luminância, o jogador
	 * atravessa a divisa sem perceber que atravessou — que é exatamente o que
	 * acontece hoje, porque o brejo herda a tabela da floresta.
	 */
	SwampMud,

	/**
	 * A poça parada.
	 *
	 * Verde, não azul: água parada com matéria orgânica dentro não reflete o
	 * céu como o mar reflete. O azul aqui diria "mar raso", e mar raso é o que
	 * o pântano justamente NÃO é.
	 */
	SwampWater,

	/**
	 * A fumaça do vulcão.
	 *
	 * Cinza claro, e mais claro que o basalto de propósito: fumaça da cor da
	 * pedra some contra o cone, e uma coluna que só se vê contra o céu deixa
	 * de contar o que devia — que aquele monte ali é o vulcão.
	 */
	AshPlume,

	/**
	 * A poça de água parada no fundo da caverna.
	 *
	 * Mais escura que o mar: ali dentro não bate sol, e água clara num buraco de
	 * pedra lê como piso pintado de azul em vez de água.
	 */
	CaveWater,

	/**
	 * A faixa de areia que a onda acabou de lavar.
	 *
	 * Escura, e escura por MOLHADA — é a única coisa numa praia que diz de que
	 * lado fica o mar. Sem ela, a areia vai até a borda do pedaço com a mesma
	 * cor e o jogador descobre onde a água começa caindo dentro dela.
	 */
	WetSand,

	/**
	 * A espuma da água branca — a do mar E a da cachoeira.
	 *
	 * Quase branca, e a coisa mais clara da paleta inteira: espuma cinzenta é
	 * pedra achatada. Na praia ela mora do lado de FORA da areia molhada,
	 * porque é ali que a onda quebra; no rio, no degrau em que a água despenca.
	 *
	 * Chamou-se `SeaFoam` por um dia, quando só a praia a usava. O nome durou
	 * até a primeira cachoeira: espuma é espuma, e dois papéis com a mesma cor
	 * são duas cores no dia em que alguém editar uma delas.
	 */
	WaterFoam,

	/**
	 * A água DOCE: rio, lago e queda.
	 *
	 * Clara e esverdeada, e é assim de propósito — o mar da borda é azul fundo
	 * e dessaturado, a poça da caverna é quase preta, e o charco é verde
	 * parado. Se a água do rio nascesse parecida com qualquer uma das três,
	 * quem olha o mapa perderia justamente a informação que ela carrega: esta
	 * dá para beber, atravessar e seguir até o monte de onde ela desce.
	 */
	FreshWater,

	/**
	 * A cortina BAIXA da aurora — o verde que quase todo mundo já viu em foto.
	 *
	 * Emissivo é o que ela deveria ser, e não é: a paleta pinta cor, não brilho
	 * (`PaintComponent` monta um `BasicShapeMaterial`, que não tem canal de
	 * emissão). Então o verde entra aqui CLARO ao ponto de se sustentar contra
	 * o céu de noite, e quem constrói a cortina escurece o mesmo verde conforme
	 * a aurora enfraquece — nunca troca a matiz. Duas matizes para a mesma
	 * aurora seriam duas auroras no dia em que alguém editasse uma.
	 */
	AuroraVeil,

	/**
	 * O ALTO da mesma cortina, arroxeado.
	 *
	 * A aurora real muda de cor com a altura porque muda o gás que acende; aqui
	 * ela muda de cor para a cortina ter TOPO. Um retângulo de verde chapado
	 * não tem direção nenhuma, e é justamente a direção que diz que aquilo
	 * pende do céu em vez de ser um muro plantado no gelo.
	 */
	AuroraCrown,

	/**
	 * Quantos papéis existem. Não é papel nenhum.
	 *
	 * Existe para o teste poder percorrer a lista inteira e cobrar `case` de
	 * cada um. Sem ela, papel novo sem `case` cai no retorno final de
	 * `ColorFor` e sai VERDE DE MATA — passa em todo teste de lógica, e o
	 * defeito só aparece na tela, que é o modo de falhar que este projeto já
	 * pagou três vezes.
	 */
	Count
};

/**
 * A paleta do cenário — fonte de verdade ÚNICA da cor de tudo que é mata.
 *
 * Ela existe porque a cor que vinha nos assets não separava nada. O chão da
 * mata era vestido com `leafsGreen`, o MESMO material das folhas das árvores:
 * chão e copa eram literalmente a mesma cor, e o quadro inteiro lia como um
 * verde-água sem relevo. Trocar um material do pacote por outro só moveria o
 * problema de lugar — os dez materiais do kit são variações de uma faixa
 * estreita, e nenhum par deles se destaca do outro a três metros da tela.
 *
 * Por isso a cor passa a ser decisão NOSSA, e mora num lugar só: duas tabelas
 * de cor concordam até a primeira edição (L-032/L-033).
 */
namespace ScenaryPalette
{
	/**
	 * A cor de UM slot de material, dado o papel de quem o usa.
	 *
	 * Duas entradas, e não uma, porque nenhuma das duas basta sozinha. Só o
	 * slot não distingue o arbusto do capim (ambos chegam como `grass`); só o
	 * papel não distingue o tronco da copa na MESMA árvore (`woodBark` e
	 * `leafsGreen` convivem num asset). O par é que decide — e é o par que
	 * faz a pedra com musgo ter pedra cinza e musgo verde.
	 */
	BATTLESQUARE_API FLinearColor ColorFor(EScenaryRole Role, FName MaterialSlot);

	/**
	 * O verde do chão da mata.
	 *
	 * Fora de `ColorFor` de propósito: o chão não é espécie nenhuma, e é
	 * contra ELE que todo o resto precisa se destacar.
	 */
	BATTLESQUARE_API FLinearColor GroundColor();

	/**
	 * O material base que aceita cor — o mesmo que já pinta pets e donos.
	 *
	 * Nulo se o conteúdo da engine não carregar, e quem chama trata: cenário
	 * sem cor é feio, cenário que derruba o jogo é defeito.
	 */
	/**
	 * O CAMINHO da malha de uma primitiva — a fonte unica (MV2/MV3).
	 *
	 * Aponta para /Engine/BasicShapes (invariante 20: o dado decide de ONDE
	 * vem a malha, nao o estilo; a feature fecha verde ainda em primitiva da
	 * engine). Trocar o pacote e trocar ISTO, num lugar so.
	 */
	BATTLESQUARE_API const TCHAR* PrimitiveMeshPath(EScenaryPrimitive Primitive);

	BATTLESQUARE_API UMaterialInterface* ColorableBaseMaterial();

	/**
	 * Pinta TODOS os slots de um componente com a paleta do papel dado, e
	 * devolve quantos pintou.
	 *
	 * Devolver a conta é o que torna a pintura verificável: componente sem
	 * malha atribuída pinta zero, passa em todo teste de lógica e não existe
	 * na tela — o padrão que já custou três defeitos neste projeto.
	 */
	BATTLESQUARE_API int32 PaintComponent(UPrimitiveComponent* Component, EScenaryRole Role);

	/**
	 * A mesma pintura, com um FATOR de brilho de 0 a 1.
	 *
	 * Existe para a aurora, que acende e apaga com a noite: a matiz continua
	 * saindo daqui e só a QUANTIDADE dela vem de fora. Quem precisa modular
	 * escolheria um verde próprio, e aí seriam duas auroras de cores diferentes
	 * no dia em que alguém editasse a da paleta (L-032).
	 *
	 * Reaproveita a tinta que o componente já tem quando ela existe. Criar uma
	 * instância dinâmica nova a cada quadro trocaria o material sessenta vezes
	 * por segundo para pintar quase sempre a mesma cor.
	 */
	BATTLESQUARE_API int32 TintComponent(
		UPrimitiveComponent* Component, EScenaryRole Role, float Brightness);
}

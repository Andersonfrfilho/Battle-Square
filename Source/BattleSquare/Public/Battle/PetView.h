// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/PetAppearance.h"
#include "Data/BattleDataTranslator.h"
#include "PetView.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

// T8 (tasks.md, PRES-11/12/13): lado lógico do pet na apresentação — reage
// a eventos do trace (via UBattleTracePlayer, T7), nunca calcula dano ou
// alcance. O que a visão sabe é só o que o evento já trouxe pronto —
// ver design.md, BTL-22.
UCLASS()
class BATTLESQUARE_API APetView : public AActor
{
	GENERATED_BODY()

public:
	APetView();

	// Posiciona o pet e define vida cheia a partir do estado inicial do
	// núcleo + info de apresentação (nome/tag, nunca entra em FPetState).
	// Não exposto ao Blueprint: FPetState/FPetPresentationInfo não são
	// BlueprintType (mesmo motivo de FTurnCommit em BuildCommit) — quem
	// consome isto é C++ (ABattleArena, T10).
	void SetInitialState(const FPetState& InitialState, const FPetPresentationInfo& Presentation);

	// Único ponto de reação a eventos do trace. Nunca recalcula: lê o que
	// o evento já trouxe pronto (Value, ToCell).
	void ApplyEvent(const FBattleEvent& Event);

	UFUNCTION(BlueprintPure)
	float GetHealthRatio() const { return HealthRatio; }

	UFUNCTION(BlueprintPure)
	bool IsDefeated() const { return bDefeated; }

	UFUNCTION(BlueprintPure)
	uint8 GetColumn() const { return Column; }

	UFUNCTION(BlueprintPure)
	uint8 GetRow() const { return Row; }

	UFUNCTION(BlueprintPure)
	uint8 GetPetId() const { return PetId; }

	/** O tipo que veio da apresentação. É ele que escolhe adorno e cor de acento. */
	const FString& GetPetType() const { return PetType; }

	/**
	 * Raiz separada da malha DE PROPÓSITO. Quando o BodyMesh era a própria
	 * raiz, o deslocamento vertical que o tira de dentro do piso era a
	 * transformação do ator — e o primeiro SetActorLocation o apagava, com o
	 * pet afundando meio corpo a partir do primeiro movimento.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<USceneComponent> BodyRoot;

	/**
	 * Tudo o que é BICHO pendura aqui, e só isto gira para encarar o
	 * adversário. A barra de vida fica de fora, na raiz: ela precisa
	 * continuar de frente para a câmera para permanecer legível.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<USceneComponent> BodyPivot;

	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/**
	 * A cabeça inclina sozinha, sem levar o corpo junto. Olhar para cima é
	 * levantar a cabeça; virar o bicho inteiro para o céu o deitaria de
	 * costas.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<USceneComponent> HeadPivot;

	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	// A esfera não tinha frente: girá-la não se via, e por isso a marca do
	// olhar orbitava o corpo como um cubo solto. Com cabeça, a frente existe
	// — a marca virou o FOCINHO, que já é onde se olha para saber para onde
	// o bicho está virado.
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> GazeMarker;

	/** Adorno do TIPO: chama, barbatana, folha ou orelha (FPetAppearance). */
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> CrestLeft;

	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> CrestRight;

	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> TailMesh;

	/**
	 * Quatro patas. Sem elas o corpo é uma bola pousada, e o pet parece
	 * flutuar mesmo assentado na casa certa — foi o que a esfera fazia.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;

	// Barra de vida: fundo escuro sempre inteiro + preenchimento que encolhe.
	// Sem o fundo não se sabe quanto FALTA, só quanto sobrou — e é a diferença
	// entre "estou mal" e "estou quase morto".
	/**
	 * O personagem de verdade, quando existe um.
	 *
	 * A silhueta de primitivas continua montada por baixo e é ela que aparece
	 * enquanto não houver malha atribuída aqui. É emenda com queda combinada,
	 * não substituição: asset que não carrega deixaria o pet invisível, e pet
	 * invisível é o defeito que este projeto já viu três vezes.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<USkeletalMeshComponent> CharacterMesh;

	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> HealthBarBackground;

	UPROPERTY(VisibleAnywhere, Category = "Apresentação")
	TObjectPtr<UStaticMeshComponent> HealthBarFill;

	/** Cor por lado: quem é meu, quem é do outro. */
	UPROPERTY(EditDefaultsOnly, Category = "Apresentação")
	FLinearColor LocalSideColor = FLinearColor(0.15f, 0.45f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category = "Apresentação")
	FLinearColor OpponentSideColor = FLinearColor(0.95f, 0.25f, 0.20f);

	/** Aplica cor do lado, adorno do tipo e estado de derrota. */
	void RefreshBodyAppearance();

	/** Verdadeiro quando há personagem vestido — e é então que a silhueta sai de cena. */
	bool HasCharacterMesh() const;

	/**
	 * Onde mora o personagem de um tipo, POR CONVENÇÃO de nome.
	 *
	 * O catálogo de pets é dado assinado que pode ganhar tipo novo sem
	 * recompilar (ver FPetAppearance::ForType); uma tabela de caminhos em
	 * código obrigaria a recompilar para cada bicho novo. Com convenção, basta
	 * o asset entrar com o nome certo.
	 *
	 * O tipo vem de fora, então o nome é peneirado antes de virar caminho:
	 * só letras e dígitos passam. Devolve vazio para o que não passar, e vazio
	 * significa "fica na silhueta" — nunca um caminho montado com o que veio.
	 */
	static FString CharacterMeshPathForType(const FString& PetType);

	/**
	 * Vira para uma posição do MUNDO, não para uma casa da grade.
	 *
	 * A casa muda de uma vez; a posição desliza. Olhando para a casa, o pet
	 * viraria a cabeça de repente e depois esperaria o outro chegar — o olhar
	 * só acompanha o movimento se seguir onde o outro ESTÁ.
	 */
	void LookAtLocation(const FVector& TargetLocation);

	/**
	 * Desliza até uma posição em vez de aparecer nela.
	 *
	 * Teleporte não conta a história: quem só vê o antes e o depois não sabe
	 * se o pet ANDOU ou foi empurrado, nem em que ordem as coisas
	 * aconteceram. Isso é independente da malha — quando o modelo 3D chegar,
	 * ele desliza igual, e a animação de passo entra por cima disto.
	 */
	void GlideTo(const FVector& Destination);

	/** Chamado pela arena a cada quadro para avançar o deslize. */
	void AdvanceGlide(float DeltaSeconds);

	/** DP-ia-04: o olhar segue o que o adversário FEZ, não só onde ele está. */
	void LookUp();
	void LookDown();
	void LoseSightOfTarget();

	uint8 GetSide() const { return Side; }

	/**
	 * Z da face de BAIXO do corpo na posição de uma pata.
	 *
	 * O corpo é um elipsoide: a barriga sobe conforme se afasta do centro, e
	 * a pata não fica no centro. Por isso este valor não é "o fundo do corpo".
	 */
	static float BodyUnderSurfaceAtLegUnits();

	/**
	 * Altura da pata — DERIVADA da barriga, não escolhida.
	 *
	 * Com 20uu fixos as patas paravam ~7uu abaixo da barriga, e na tela o
	 * corpo pairava sobre quatro tocos soltos. Número fixo não acompanha
	 * mudança de escala do corpo; este acompanha.
	 */
	static float LegHeightUnits();

	/** Ponto mais baixo do corpo — no centro, onde a barriga desce mais. */
	static float BodyLowestPointUnits();

	/** Raio da cabeça. É nele que o adorno assenta. */
	static float HeadRadiusUnits();

	/** Quanto o adorno entra na cabeça, para não sobrar costura entre os dois. */
	static float CrestEmbedUnits();

	/** -1 é o adorno esquerdo, +1 o direito. O direito espelha o Roll. */
	static FRotator CrestRotationForSide(const FRotator& CrestRotation, float LateralSign);

	/**
	 * Onde o adorno assenta, com a BASE encostando na superfície da cabeça.
	 *
	 * Com um Z fixo o cone de 26uu afundava e sobravam 9uu para fora: na tela
	 * isso não é orelha, é uma mancha clara na testa — e o adorno é justamente
	 * quem diz o TIPO do pet, então o tipo deixava de ser dito.
	 */
	static FVector CrestRelativeLocation(const FPetAppearance& Appearance, float LateralSign);

private:
	void BuildBody();
	void BuildHead();
	void BuildLegs();
	void BuildCharacter();
	void BuildHealthBar();
	void RefreshHealthBar();
	void RefreshCrests();

	/** Carrega o personagem do tipo, se houver um. Sem asset, não mexe em nada. */
	void RefreshCharacterMesh();

	/**
	 * Quem aparece: personagem OU silhueta, nunca os dois, nenhum se derrotado.
	 *
	 * Uma função só decide isso — duas concordariam até a primeira edição
	 * (L-032, L-033), e o desacordo aqui é o bicho dentro do bicho.
	 */
	void RefreshVisibility();

	/** A malha da engine que desenha cada forma de adorno. */
	UStaticMesh* CrestMeshFor(EPetCrestShape Shape) const;

	static constexpr float CubeSizeUnits = 100.0f;
	static constexpr float BarWidthScale = 0.9f;
	static constexpr float BarHeightScale = 0.09f;
	static constexpr float BarHeightUnits = 105.0f;
	static constexpr float BarDepthScale = 0.04f;

	// Menor que o fundo, para ele virar moldura e as bordas NÃO coincidirem.
	static constexpr float BarFillHeightScale = 0.06f;

	// Maior que a espessura do cubo (BarDepthScale * CubeSizeUnits = 4uu):
	// separação menor que isso deixa faces no mesmo plano, e é o que piscava.
	static constexpr float BarFrontOffsetUnits = 6.0f;

	// Um pouco mais curto que o passo da reprodução (0,45s), para o pet chegar
	// antes do evento seguinte em vez de arrastar por cima dele.
	static constexpr float GlideSeconds = 0.30f;

	// A silhueta cabe numa casa de 150uu com folga: o bicho ocupa a casa sem
	// encostar na vizinha, e a laje continua sendo lida como casa.
	static constexpr float BodyCenterUnits = 44.0f;
	static constexpr float HeadForwardUnits = 30.0f;
	static constexpr float HeadCenterUnits = 58.0f;
	static constexpr float HeadPitchDegrees = 30.0f;

	UPROPERTY()
	uint8 PetId = 0;

	UPROPERTY()
	uint8 Side = 0;

	UPROPERTY()
	FString PetType;

	// Guardadas no CDO para a silhueta poder trocar de adorno em tempo de
	// execução: o tipo só é conhecido em SetInitialState, e ConstructorHelpers
	// só funciona no construtor.
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeAsset;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderAsset;

	FVector GlideStart = FVector::ZeroVector;
	FVector GlideTarget = FVector::ZeroVector;
	float GlideElapsed = 0.0f;
	bool bIsGliding = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HealthBarFillMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HealthBarBackgroundMaterial;

	UPROPERTY()
	uint8 Column = 0;

	UPROPERTY()
	uint8 Row = 0;

	UPROPERTY()
	int32 MaxHealth = 0;

	// Derivado de MaxHealth/Health do evento — nunca de um novo cálculo de
	// dano. 1.0 = vida cheia, 0.0 = sem vida.
	UPROPERTY()
	float HealthRatio = 1.0f;

	UPROPERTY()
	bool bDefeated = false;
};

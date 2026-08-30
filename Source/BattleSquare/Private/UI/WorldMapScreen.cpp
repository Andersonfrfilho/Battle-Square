// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/WorldMapScreen.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "WorldMap"

namespace
{
	FWorldMapSnapshot GSnapshot;
	bool GFullMapOpen = false;

	const FLinearColor CorDaAgua(0.06f, 0.16f, 0.26f, 0.95f);
	const FLinearColor CorDaTerra(0.16f, 0.20f, 0.15f, 0.95f);
	const FLinearColor CorDaBorda(0.45f, 0.52f, 0.46f, 1.0f);
	const FLinearColor CorDoJogador(0.95f, 0.95f, 0.90f, 1.0f);

	/** Quanto do mundo cabe no minimapa. Curto: ele é para o passo seguinte. */
	constexpr float AlcanceDoMinimapaUnidades = 3500.0f;

	/** Um quadradinho, porque o Slate desenha caixa e não círculo sem asset. */
	void DesenharPonto(FSlateWindowElementList& Elementos, int32 Camada,
		const FGeometry& Geometria, const FVector2D& CentroLocal, float Lado,
		const FLinearColor& Cor)
	{
		FSlateDrawElement::MakeBox(
			Elementos, Camada,
			Geometria.ToPaintGeometry(
				FVector2f(Lado, Lado),
				FSlateLayoutTransform(FVector2f(
					static_cast<float>(CentroLocal.X) - Lado * 0.5f,
					static_cast<float>(CentroLocal.Y) - Lado * 0.5f))),
			FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
			ESlateDrawEffect::None, Cor);
	}

	/**
	 * O desenho de um mapa. O MODO e o ALCANCE vêm de fora — é o mesmo widget
	 * servindo o minimapa e o mapa completo, e a diferença entre os dois é
	 * dado, não código duplicado.
	 */
	class SMapaDoMundo : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMapaDoMundo) {}
			SLATE_ARGUMENT(FWorldMapProjection::EMode, Modo)
			SLATE_ARGUMENT(float, AlcanceUnidades)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Modo = InArgs._Modo;
			AlcanceUnidades = InArgs._AlcanceUnidades;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(200.0f, 200.0f);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometria,
			const FSlateRect& CullingRect, FSlateWindowElementList& Elementos,
			int32 Camada, const FWidgetStyle& Estilo, bool bParentEnabled) const override
		{
			const FVector2D Tamanho = Geometria.GetLocalSize();
			const FVector2D Centro = Tamanho * 0.5f;
			const float RaioEmPixels = FMath::Min(Tamanho.X, Tamanho.Y) * 0.5f;

			// Água primeiro, terra por cima: o mapa é uma ilha, e a ordem
			// desenha isso sem precisar recortar nada.
			DesenharPonto(Elementos, Camada, Geometria, Centro,
				RaioEmPixels * 2.0f, CorDaAgua);

			const float RaioDaTerraEmPixels =
				(GSnapshot.ShoreRadiusUnits / AlcanceUnidades) * RaioEmPixels;
			DesenharPonto(Elementos, Camada + 1, Geometria, Centro,
				FMath::Min(RaioDaTerraEmPixels * 2.0f, RaioEmPixels * 2.0f), CorDaTerra);

			int32 CamadaAtual = Camada + 2;

			for (const FWorldMapMarkerInfo& Marcador : GSnapshot.Markers)
			{
				// O QUE NÃO FOI DESCOBERTO não aparece. A regra mora na
				// projeção, e não aqui, porque o minimapa e o mapa completo
				// precisam responder igual — duas cópias dariam um adversário
				// visível num e escondido no outro.
				if (!FWorldMapProjection::IsMarkerVisible(Marcador, GSnapshot))
				{
					continue;
				}

				const FVector2D Normalizado = FWorldMapProjection::ToMapSpace(
					Marcador.WorldXY, GSnapshot, Modo, AlcanceUnidades);

				// FORA do alcance não é desenhado na borda: um marcador
				// empurrado para a beirada diz "está aqui" sobre um lugar onde
				// ele não está, e quem anda até lá não acha nada.
				if (Normalizado.Size() > 1.0f)
				{
					continue;
				}

				const FVector2D EmPixels = Centro + Normalizado * RaioEmPixels;
				const float LadoEmPixels = FMath::Max(4.0f,
					(Marcador.WorldRadiusUnits / AlcanceUnidades) * RaioEmPixels * 2.0f);

				DesenharPonto(Elementos, CamadaAtual, Geometria, EmPixels,
					LadoEmPixels, Marcador.Color);
			}

			++CamadaAtual;

			// O JOGADOR por último, no centro, e sempre por cima: se um
			// adversário puder cobri-lo, o mapa perde a única referência que o
			// jogador tem de si mesmo.
			DesenharPonto(Elementos, CamadaAtual, Geometria, Centro, 9.0f, CorDoJogador);

			// A direção do olhar, como um segundo ponto à frente do primeiro.
			// É pobre perto de uma seta desenhada, e é o que dá para fazer sem
			// asset autorado — e um ponto adiante já responde "para onde eu
			// estou virado?", que é a pergunta.
			const float AnguloDaSeta = FWorldMapProjection::PlayerArrowAngleDegrees(GSnapshot, Modo);
			const float Radianos = FMath::DegreesToRadians(AnguloDaSeta);
			const FVector2D Frente(FMath::Sin(Radianos), -FMath::Cos(Radianos));

			DesenharPonto(Elementos, CamadaAtual + 1, Geometria,
				Centro + Frente * 11.0f, 5.0f, CorDoJogador);

			return CamadaAtual + 2;
		}

	private:
		FWorldMapProjection::EMode Modo = FWorldMapProjection::EMode::SeguindoOOlhar;
		float AlcanceUnidades = AlcanceDoMinimapaUnidades;
	};

	TSharedPtr<SWidget> GMinimapa;
	TSharedPtr<SWidget> GMapaCompleto;
}

void FWorldMapScreen::Show(UWorld* World)
{
	if (!GEngine || !GEngine->GameViewport || GMinimapa.IsValid())
	{
		return;
	}

	GMinimapa =
		SNew(SBox)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 16.0f, 16.0f, 0.0f))
		[
			SNew(SBox).WidthOverride(180.0f).HeightOverride(180.0f)
			[
				SNew(SMapaDoMundo)
				.Modo(FWorldMapProjection::EMode::SeguindoOOlhar)
				.AlcanceUnidades(AlcanceDoMinimapaUnidades)
			]
		];

	// ZOrder ABAIXO do carregamento (1000): o mapa não pode aparecer por cima
	// de uma tela que existe para cobrir o mundo.
	GEngine->GameViewport->AddViewportWidgetContent(GMinimapa.ToSharedRef(), /*ZOrder=*/50);
}

void FWorldMapScreen::Update(const FWorldMapSnapshot& Snapshot)
{
	GSnapshot = Snapshot;
}

void FWorldMapScreen::ToggleFullMap()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (GMapaCompleto.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GMapaCompleto.ToSharedRef());
		GMapaCompleto.Reset();
		GFullMapOpen = false;
		return;
	}

	// O completo mostra o MUNDO INTEIRO: alcance pelo raio da margem, com
	// folga, para a água aparecer como moldura e o jogador ver onde acaba.
	const float AlcanceCompleto = GSnapshot.ShoreRadiusUnits * 1.25f;

	GMapaCompleto =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(520.0f).HeightOverride(520.0f)
			[
				SNew(SMapaDoMundo)
				.Modo(FWorldMapProjection::EMode::NorteAcima)
				.AlcanceUnidades(AlcanceCompleto)
			]
		];

	GEngine->GameViewport->AddViewportWidgetContent(GMapaCompleto.ToSharedRef(), /*ZOrder=*/60);
	GFullMapOpen = true;
}

void FWorldMapScreen::Hide()
{
	if (GEngine && GEngine->GameViewport)
	{
		if (GMinimapa.IsValid())
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(GMinimapa.ToSharedRef());
		}
		if (GMapaCompleto.IsValid())
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(GMapaCompleto.ToSharedRef());
		}
	}
	GMinimapa.Reset();
	GMapaCompleto.Reset();
	GFullMapOpen = false;
}

bool FWorldMapScreen::IsFullMapOpen() { return GFullMapOpen; }
bool FWorldMapScreen::IsVisible() { return GMinimapa.IsValid(); }

#undef LOCTEXT_NAMESPACE

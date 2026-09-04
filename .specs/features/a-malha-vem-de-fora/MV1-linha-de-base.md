# MV1 — A linha de base MEDIDA (congelada em 04/09/2026)

> Congelada ANTES de mexer, como a MV1 exige. Cada `FObjectFinder` de
> malha/material do projeto, com arquivo e linha. A migracao (MV4-MV8) so
> pode afirmar 'a malha veio do dado' contra esta lista — sem ela, 'migrado'
> nao teria como ser medido.

**Total medido: 45 sitios de FObjectFinder de malha/material, em ~20 atores.**
(O resto de Content/ e MI_StreamingCell_* gerado por codigo; 6 assets autorados.)

## Todos os sitios (arquivo:linha)

```
Source/BattleSquare/Private/Battle/BattleArena.cpp:507:	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDaArena(TEXT("/Engine/BasicShapes/Cube.Cube"));
Source/BattleSquare/Private/Battle/BattleArena.cpp:577:	static ConstructorHelpers::FObjectFinder<UStaticMesh> PedraDoObstaculo(
Source/BattleSquare/Private/Battle/BattleArena.cpp:579:	static ConstructorHelpers::FObjectFinder<UStaticMesh> TroncoDoObstaculo(
Source/BattleSquare/Private/Battle/PetOwnerView.cpp:64:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
Source/BattleSquare/Private/Battle/PetOwnerView.cpp:65:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
Source/BattleSquare/Private/Battle/PetOwnerView.cpp:66:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
Source/BattleSquare/Private/Battle/PetOwnerView.cpp:67:	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialBasico(
Source/BattleSquare/Private/Battle/PetView.cpp:103:	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialBasico(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
Source/BattleSquare/Private/Battle/PetView.cpp:81:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
Source/BattleSquare/Private/Battle/PetView.cpp:82:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
Source/BattleSquare/Private/Battle/PetView.cpp:83:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
Source/BattleSquare/Private/Battle/PetView.cpp:84:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
Source/BattleSquare/Private/Environment/AuroraCurtain.cpp:47:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(AuroraDaGeleira::MalhaDaFita);
Source/BattleSquare/Private/Environment/CaveSystem.cpp:68:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(CavernaDaIlha::MalhaDaPedra);
Source/BattleSquare/Private/Environment/CaveSystem.cpp:93:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(CavernaDaIlha::MalhaDaPonta);
Source/BattleSquare/Private/Environment/ForestBackdrop.cpp:1035:		ConstructorHelpers::FObjectFinder<UStaticMesh> Malha(*Caminho);
Source/BattleSquare/Private/Environment/ForestBackdrop.cpp:885:	static ConstructorHelpers::FObjectFinder<UStaticMesh> CilindroDoChao(CilindroDaEngine);
Source/BattleSquare/Private/Environment/ForestBackdrop.cpp:894:	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDoChao(CuboDaEngine);
Source/BattleSquare/Private/Environment/ForestBackdrop.cpp:911:	static ConstructorHelpers::FObjectFinder<UStaticMesh> EsferaDoMonte(EsferaDaEngine);
Source/BattleSquare/Private/Environment/ForestBackdrop.cpp:935:	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDaOrla(CuboDaEngine);
Source/BattleSquare/Private/Environment/ForestBackdrop.cpp:950:	static ConstructorHelpers::FObjectFinder<UStaticMesh> TroncoDaBeira(
Source/BattleSquare/Private/Environment/ForestBackdrop.cpp:993:	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDoRio(CuboDaEngine);
Source/BattleSquare/Private/Environment/MountainRange.cpp:118:	ConstructorHelpers::FObjectFinder<UStaticMesh> Rocha(SerraDoHorizonte::MalhaDoCorpo);
Source/BattleSquare/Private/Environment/MountainRange.cpp:119:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(SerraDoHorizonte::MalhaDeReserva);
Source/BattleSquare/Private/Environment/Volcano.cpp:65:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(VulcaoDaIlha::MalhaDoDisco);
Source/BattleSquare/Private/Environment/Volcano.cpp:66:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(VulcaoDaIlha::MalhaDoBloco);
Source/BattleSquare/Private/Environment/Volcano.cpp:86:	ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(VulcaoDaIlha::MalhaDoBafo);
Source/BattleSquare/Private/Environment/WalkableMountain.cpp:82:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(MontanhaDaIlha::MalhaDoCorpo);
Source/BattleSquare/Private/Environment/WalkableMountain.cpp:83:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(MontanhaDaIlha::MalhaDoPatamar);
Source/BattleSquare/Private/Environment/WalkableMountain.cpp:84:	ConstructorHelpers::FObjectFinder<UStaticMesh> Pedra(MontanhaDaIlha::MalhaDoCostado);
Source/BattleSquare/Private/Environment/WorldBoundaryWater.cpp:39:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(
Source/BattleSquare/Private/Environment/WorldBoundaryWater.cpp:46:	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basico(
Source/BattleSquare/Private/World/FerryActor.cpp:54:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(Balsa::MalhaDoConves);
Source/BattleSquare/Private/World/GroundUseActor.cpp:145:	ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(UsoDoSolo::Cubo);
Source/BattleSquare/Private/World/MountView.cpp:34:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(
Source/BattleSquare/Private/World/Village.cpp:141:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(
Source/BattleSquare/Private/World/VillagerActor.cpp:66:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cilindro(
Source/BattleSquare/Private/World/VillagerActor.cpp:68:	static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(
Source/BattleSquare/Private/World/WorldEncounterActor.cpp:21:	static ConstructorHelpers::FObjectFinder<UStaticMesh> CorpoMesh(
Source/BattleSquare/Private/World/WorldEncounterActor.cpp:33:	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CorpoMaterial(
Source/BattleSquare/Private/World/WorldExplorerCharacter.cpp:53:	static ConstructorHelpers::FObjectFinder<UStaticMesh> CorpoMesh(
Source/BattleSquare/Private/World/WorldExplorerCharacter.cpp:61:	static ConstructorHelpers::FObjectFinder<UStaticMesh> MarcaMesh(
Source/BattleSquare/Private/World/WorldExplorerCharacter.cpp:73:	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CorpoMaterial(
Source/BattleSquare/Private/World/WorldTrainingField.cpp:89:	static ConstructorHelpers::FObjectFinder<UStaticMesh> MalhaDoMarco(
Source/BattleSquare/Private/World/WorldTrainingField.cpp:96:	static ConstructorHelpers::FObjectFinder<UStaticMesh> MalhaDaBorda(
```

## Por ator (o alvo de cada caixa de migracao)

- **MV4** APetView (PetView.cpp)
- **MV5** PetOwnerView, BattleArena
- **MV6** Village, GroundUseActor, FerryActor, WorldEncounterActor, WorldExplorerCharacter, VillagerActor, WorldTrainingField, MountView
- **MV7** ForestBackdrop, MountainRange, WalkableMountain, CaveSystem, Volcano, AuroraCurtain, WorldBoundaryWater
- **MV8** (material pelo dado) TrailMesh, AqueductMesh, RiverMesh, TerrainMesh, CrossingMesh

## Observacao medida

Varios atores JA leem o caminho da malha de uma CONSTANTE de namespace
(ex. `SerraDoHorizonte::MalhaDoCorpo`, `VulcaoDaIlha::MalhaDoDisco`,
`CavernaDaIlha::MalhaDaPedra`) em vez de literal solto — a MV2 unifica essas
constantes espalhadas numa fonte unica por PAPEL, e a MV3 faz `ScenaryPalette`
ser o ponto por onde malha e material entram, ao lado da cor que ja mora la.

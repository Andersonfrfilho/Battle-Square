// Copyright 2026 Anderson. All Rights Reserved.

#include "World/ResourceCatalog.h"

#define LOCTEXT_NAMESPACE "ResourceCatalog"

FText ResourceCatalog::NameOf(EWorldResource Resource)
{
	switch (Resource)
	{
	case EWorldResource::Madeira:  return LOCTEXT("Madeira", "madeira");
	case EWorldResource::Pedra:    return LOCTEXT("Pedra", "pedra");
	case EWorldResource::Fruta:    return LOCTEXT("Fruta", "fruta");
	case EWorldResource::Flor:     return LOCTEXT("Flor", "flor");
	case EWorldResource::Fibra:    return LOCTEXT("Fibra", "fibra");
	case EWorldResource::Cogumelo: return LOCTEXT("Cogumelo", "cogumelo");
	case EWorldResource::Minerio:  return LOCTEXT("Minerio", "minério");
	case EWorldResource::Cristal:  return LOCTEXT("Cristal", "cristal");
	case EWorldResource::Mel:      return LOCTEXT("Mel", "mel");
	case EWorldResource::Peixe:    return LOCTEXT("Peixe", "peixe");
	case EWorldResource::Argila:   return LOCTEXT("Argila", "argila");
	case EWorldResource::Sal:      return LOCTEXT("Sal", "sal");
	case EWorldResource::Agua:     return LOCTEXT("Agua", "água");
	case EWorldResource::Gelo:     return LOCTEXT("Gelo", "gelo");
	default:                       return LOCTEXT("Desconhecido", "recurso");
	}
}

EGatherTool ResourceCatalog::RequiredTool(EWorldResource Resource)
{
	// 68-b: a mão colhe o fácil (flor, fruta, cogumelo, fibra); o resto exige a
	// ferramenta certa. Fonte única do "o que precisa de quê".
	switch (Resource)
	{
	case EWorldResource::Madeira:                          return EGatherTool::Machado;
	case EWorldResource::Pedra:
	case EWorldResource::Minerio:
	case EWorldResource::Cristal:                          return EGatherTool::Picareta;
	case EWorldResource::Peixe:                            return EGatherTool::Vara;
	case EWorldResource::Agua:                             return EGatherTool::Balde;

	// Mão vazia dá conta: flor, fruta, cogumelo, fibra, mel, argila, sal, gelo.
	default:                                               return EGatherTool::Nenhuma;
	}
}

#undef LOCTEXT_NAMESPACE

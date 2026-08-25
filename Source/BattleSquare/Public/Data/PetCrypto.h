// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// AD-019: consome o OpenSSL 1.1.1t já empacotado na engine — nenhuma
// biblioteca terceira nova vendorizada. Cobre as duas necessidades de
// criptografia do backend de dados de pet: decifrar o espelho local
// (AES-256-GCM, AD-018) e verificar a assinatura de cada registro
// (Ed25519, PETDB-10).
//
// Contrato de formato — TEM que bater byte a byte com o lado TypeScript
// (mirror-crypto.ts, pet-signing.ts) ou a interoperabilidade quebra em
// silêncio:
//   - AES-256-GCM: [IV 12 bytes][AuthTag 16 bytes][ciphertext...]
//   - Ed25519: assina o JSON canônico {id,name,type,attack,defense,speed,maxHealth,updatedAt},
//     nessa ordem exata de chaves.
namespace BattlePetCrypto
{
	// Decifra um buffer no formato acima. Retorna false se a chave estiver
	// errada OU o arquivo tiver sido adulterado — GCM detecta os dois
	// casos da mesma forma (falha de autenticação).
	bool DecryptMirrorBuffer(
		TConstArrayView<uint8> EncryptedBuffer,
		TConstArrayView<uint8> Key32Bytes,
		TArray<uint8>& OutPlaintext);

	// Verifica uma assinatura Ed25519 sobre o JSON canônico do registro.
	// A chave pública é passada em formato PEM (SPKI) — a mesma exportada
	// pelo backend TypeScript via node:crypto.
	bool VerifyEd25519Signature(
		const FString& CanonicalJsonPayload,
		TConstArrayView<uint8> SignatureBytes,
		const FString& PublicKeyPem);
}

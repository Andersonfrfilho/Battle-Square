// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/PetDataLoader.h"
#include "Data/PetCrypto.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"

THIRD_PARTY_INCLUDES_START
#include "sqlite3.h"
THIRD_PARTY_INCLUDES_END

namespace
{
	// Espelha mirror-tempdir.ts (AD-018): RAM real só no Linux com
	// /dev/shm; qualquer outra plataforma AVISA que está usando disco
	// comum, em vez de fingir a mesma garantia. Mesma regra dos dois lados.
	FString ResolvePlainTextTempDirectory()
	{
#if PLATFORM_LINUX
		if (IFileManager::Get().DirectoryExists(TEXT("/dev/shm")))
		{
			return TEXT("/dev/shm");
		}
#endif
		static bool bHasWarned = false;
		if (!bHasWarned)
		{
			UE_LOG(LogTemp, Warning, TEXT(
				"PetDataLoader: nenhum diretorio RAM-backed detectado nesta plataforma. "
				"O temporario em texto plano do espelho vai usar o temp dir comum do SO, "
				"NAO memoria. Mitigacao do AD-018 reduzida em dev; producao deve rodar em "
				"Linux com /dev/shm disponivel."));
			bHasWarned = true;
		}
		return FPlatformProcess::UserTempDir();
	}

	// Serialização canônica — TEM que bater byte a byte com
	// pet-signing.ts / signature-verifier.ts (mesma ordem de campo).
	FString BuildCanonicalPayload(const FLoadedPetRecord& Record)
	{
		return FString::Printf(
			TEXT("{\"id\":\"%s\",\"name\":\"%s\",\"type\":\"%s\",\"attack\":%d,\"defense\":%d,\"speed\":%d,\"maxHealth\":%d,\"updatedAt\":\"%s\"}"),
			*Record.Id, *Record.Name, *Record.Type,
			Record.Attack, Record.Defense, Record.Speed, Record.MaxHealth,
			*Record.UpdatedAt);
	}

	// RAII: apaga o temporário em texto plano mesmo em caminho de erro —
	// equivalente ao trap do probe_isolation.sh e ao try/finally do
	// mirror.schema.ts.
	class FScopedPlainTextFile
	{
	public:
		explicit FScopedPlainTextFile(FString InPath) : Path(MoveTemp(InPath)) {}
		~FScopedPlainTextFile()
		{
			if (IFileManager::Get().FileExists(*Path))
			{
				IFileManager::Get().Delete(*Path, /*RequireExists=*/false, /*EvenReadOnly=*/true);
			}
		}
		const FString& GetPath() const { return Path; }

	private:
		FString Path;
	};
}

bool FPetDataLoader::LoadVerifiedPets(
	const FString& EncryptedMirrorPath,
	TConstArrayView<uint8> EncryptionKey32Bytes,
	const FString& Ed25519PublicKeyPem,
	TArray<FLoadedPetRecord>& OutPets,
	int32& OutRejectedCount)
{
	OutPets.Reset();
	OutRejectedCount = 0;

	// PETDB-06: espelho ausente é erro explícito.
	if (!IFileManager::Get().FileExists(*EncryptedMirrorPath))
	{
		UE_LOG(LogTemp, Error, TEXT("PetDataLoader: espelho local ausente em '%s' — não é seguro montar batalhas sem dados de pet"), *EncryptedMirrorPath);
		return false;
	}

	TArray<uint8> EncryptedBytes;
	if (!FFileHelper::LoadFileToArray(EncryptedBytes, *EncryptedMirrorPath))
	{
		UE_LOG(LogTemp, Error, TEXT("PetDataLoader: falha ao ler o arquivo do espelho"));
		return false;
	}

	TArray<uint8> PlainTextBytes;
	if (!BattlePetCrypto::DecryptMirrorBuffer(EncryptedBytes, EncryptionKey32Bytes, PlainTextBytes))
	{
		UE_LOG(LogTemp, Error, TEXT("PetDataLoader: falha ao decifrar o espelho — chave errada ou arquivo adulterado"));
		return false;
	}

	const FString TempDir = ResolvePlainTextTempDirectory();
	const FString TempPlainPath = FPaths::Combine(TempDir, FString::Printf(TEXT("pet-mirror-%s.sqlite"), *FGuid::NewGuid().ToString()));
	FScopedPlainTextFile ScopedTemp(TempPlainPath);

	if (!FFileHelper::SaveArrayToFile(PlainTextBytes, *TempPlainPath))
	{
		UE_LOG(LogTemp, Error, TEXT("PetDataLoader: falha ao escrever o temporário em texto plano"));
		return false;
	}

	sqlite3* Database = nullptr;
	if (sqlite3_open_v2(TCHAR_TO_UTF8(*TempPlainPath), &Database, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
	{
		UE_LOG(LogTemp, Error, TEXT("PetDataLoader: falha ao abrir o SQLite temporário: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(Database)));
		if (Database) { sqlite3_close(Database); }
		return false;
	}

	sqlite3_stmt* Statement = nullptr;
	const char* SelectSql = "SELECT id, name, type, attack, defense, speed, maxHealth, updatedAt, signature FROM pets";
	if (sqlite3_prepare_v2(Database, SelectSql, -1, &Statement, nullptr) != SQLITE_OK)
	{
		UE_LOG(LogTemp, Error, TEXT("PetDataLoader: falha ao preparar a query: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(Database)));
		sqlite3_close(Database);
		return false;
	}

	while (sqlite3_step(Statement) == SQLITE_ROW)
	{
		FLoadedPetRecord Record;
		Record.Id = UTF8_TO_TCHAR(reinterpret_cast<const char*>(sqlite3_column_text(Statement, 0)));
		Record.Name = UTF8_TO_TCHAR(reinterpret_cast<const char*>(sqlite3_column_text(Statement, 1)));
		Record.Type = UTF8_TO_TCHAR(reinterpret_cast<const char*>(sqlite3_column_text(Statement, 2)));
		Record.Attack = sqlite3_column_int(Statement, 3);
		Record.Defense = sqlite3_column_int(Statement, 4);
		Record.Speed = sqlite3_column_int(Statement, 5);
		Record.MaxHealth = sqlite3_column_int(Statement, 6);
		Record.UpdatedAt = UTF8_TO_TCHAR(reinterpret_cast<const char*>(sqlite3_column_text(Statement, 7)));
		const FString SignatureBase64 = UTF8_TO_TCHAR(reinterpret_cast<const char*>(sqlite3_column_text(Statement, 8)));

		TArray<uint8> SignatureBytes;
		const bool bSignatureDecoded = FBase64::Decode(SignatureBase64, SignatureBytes);
		const FString CanonicalPayload = BuildCanonicalPayload(Record);
		const bool bSignatureValid = bSignatureDecoded
			&& BattlePetCrypto::VerifyEd25519Signature(CanonicalPayload, SignatureBytes, Ed25519PublicKeyPem);

		if (!bSignatureValid)
		{
			UE_LOG(LogTemp, Warning, TEXT("PetDataLoader: assinatura inválida para o pet '%s' — descartado, POSSÍVEL VIOLAÇÃO"), *Record.Id);
			++OutRejectedCount;
			continue;
		}

		OutPets.Add(MoveTemp(Record));
	}

	sqlite3_finalize(Statement);
	sqlite3_close(Database);

	// PETDB-06: espelho vazio (nunca sincronizado com sucesso, ou tudo
	// rejeitado) também é erro explícito — nunca lista vazia silenciosa.
	if (OutPets.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("PetDataLoader: espelho não tem nenhum pet verificado — não é seguro montar batalhas"));
		return false;
	}

	return true;
}

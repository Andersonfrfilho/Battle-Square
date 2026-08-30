// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/PetDataLoader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
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

	/**
	 * Lê os golpes do JSON já verificado.
	 *
	 * Depois da assinatura conferir, e não antes: interpretar dado ainda não
	 * verificado é dar trabalho a um payload que pode ser lixo. O JSON cru
	 * continua guardado para remontar o payload — este parse é só para o jogo
	 * usar os nomes.
	 */
	void ParsePetMoves(const FString& Json, TArray<FLoadedPetMove>& OutMoves)
	{
		OutMoves.Reset();

		TArray<TSharedPtr<FJsonValue>> Valores;
		const TSharedRef<TJsonReader<>> Leitor = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Leitor, Valores))
		{
			return;
		}

		for (const TSharedPtr<FJsonValue>& Valor : Valores)
		{
			const TSharedPtr<FJsonObject>* Objeto = nullptr;
			if (!Valor.IsValid() || !Valor->TryGetObject(Objeto))
			{
				continue;
			}

			FLoadedPetMove Golpe;
			(*Objeto)->TryGetStringField(TEXT("name"), Golpe.Name);
			(*Objeto)->TryGetNumberField(TEXT("power"), Golpe.Power);

			// Ausente continua "none": golpe assinado antes do efeito existir
			// não muda a casa, e é assim que ele sempre se comportou.
			(*Objeto)->TryGetStringField(TEXT("terrainEffect"), Golpe.TerrainEffect);
			(*Objeto)->TryGetNumberField(TEXT("terrainDuration"), Golpe.TerrainDuration);

			// Chaves ausentes ficam com o padrão do struct: golpe assinado
			// antes do requisito existir não exige nada, e é assim que ele
			// sempre se comportou. O payload canônico não muda por causa
			// disto — o JSON dos golpes é repassado como veio do espelho.
			(*Objeto)->TryGetStringField(TEXT("requiresAttribute"), Golpe.RequiresAttribute);
			(*Objeto)->TryGetNumberField(TEXT("requiresValue"), Golpe.RequiresValue);
			(*Objeto)->TryGetStringField(TEXT("effectStat"), Golpe.EffectStat);
			(*Objeto)->TryGetNumberField(TEXT("effectPercent"), Golpe.EffectPercent);

			if (!Golpe.Name.IsEmpty())
			{
				OutMoves.Add(Golpe);
			}
		}
	}

	// Serialização canônica — TEM que bater byte a byte com
	// pet-signing.ts / signature-verifier.ts (mesma ordem de campo).
	FString BuildCanonicalPayload(const FLoadedPetRecord& Record, bool bIncludeMoves)
	{
		if (!bIncludeMoves)
		{
			// Formato ANTERIOR aos golpes, sem a chave `moves`.
			//
			// Pet assinado pelo backend antigo não tem esse campo no payload —
			// e acrescentá-lo, ainda que vazio, produz um texto diferente do
			// que foi assinado. A assinatura dele falharia, e o jogo ficaria
			// sem pet nenhum por causa de um campo que não existia quando ele
			// foi criado.
			return FString::Printf(
				TEXT("{\"id\":\"%s\",\"name\":\"%s\",\"type\":\"%s\",\"attack\":%d,\"defense\":%d,\"speed\":%d,\"maxHealth\":%d,\"updatedAt\":\"%s\"}"),
				*Record.Id, *Record.Name, *Record.Type,
				Record.Attack, Record.Defense, Record.Speed, Record.MaxHealth,
				*Record.UpdatedAt);
		}

		return FString::Printf(
			// Golpes no FIM, como em pet-signing.ts. O JSON dos golpes é
			// repassado COMO VEIO do espelho, sem reserializar: reconstruí-lo
			// campo a campo aqui arriscaria produzir espaçamento ou ordem
			// diferentes do que foi assinado, e a verificação falharia sem
			// ninguém entender por quê — que é exatamente o risco que o
			// comentário do backend descreve.
			TEXT("{\"id\":\"%s\",\"name\":\"%s\",\"type\":\"%s\",\"attack\":%d,\"defense\":%d,\"speed\":%d,\"maxHealth\":%d,\"updatedAt\":\"%s\",\"moves\":%s}"),
			*Record.Id, *Record.Name, *Record.Type,
			Record.Attack, Record.Defense, Record.Speed, Record.MaxHealth,
			*Record.UpdatedAt,
			// Vazio vira "[]", não string vazia: pet cadastrado antes dos
			// golpes foi assinado com moves:[] , e qualquer outra coisa aqui
			// invalidaria a assinatura DELE.
			Record.MovesCanonicalJson.IsEmpty() ? TEXT("[]") : *Record.MovesCanonicalJson);
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

	// Espelho ANTERIOR aos golpes não tem a coluna `moves`, e o worker cria a
	// tabela com CREATE TABLE IF NOT EXISTS — então um espelho já em uso não
	// ganha a coluna sozinho. Recusar aqui deixaria o jogo sem pet nenhum por
	// causa de um campo que ninguém tinha como ter.
	//
	// Aqueles pets foram assinados SEM golpes, e "[]" é exatamente o que a
	// assinatura deles espera — por isso o caminho antigo continua válido, não
	// é remendo.
	sqlite3_stmt* Statement = nullptr;
	bool bMirrorHasMovesColumn = true;

	const char* SelectWithMoves = "SELECT id, name, type, attack, defense, speed, maxHealth, updatedAt, moves, signature FROM pets";
	const char* SelectWithoutMoves = "SELECT id, name, type, attack, defense, speed, maxHealth, updatedAt, signature FROM pets";

	if (sqlite3_prepare_v2(Database, SelectWithMoves, -1, &Statement, nullptr) != SQLITE_OK)
	{
		bMirrorHasMovesColumn = false;
		UE_LOG(LogTemp, Display,
			TEXT("PetDataLoader: espelho sem a coluna 'moves' — lendo no formato anterior aos golpes"));

		if (sqlite3_prepare_v2(Database, SelectWithoutMoves, -1, &Statement, nullptr) != SQLITE_OK)
		{
			UE_LOG(LogTemp, Error, TEXT("PetDataLoader: falha ao preparar a query: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(Database)));
			sqlite3_close(Database);
			return false;
		}
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

		// Coluna ausente ou nula vira "[]", que é como aquele pet foi assinado.
		Record.MovesCanonicalJson = FString(TEXT("[]"));
		if (bMirrorHasMovesColumn)
		{
			if (const unsigned char* MovesText = sqlite3_column_text(Statement, 8))
			{
				Record.MovesCanonicalJson = UTF8_TO_TCHAR(reinterpret_cast<const char*>(MovesText));
			}
			ParsePetMoves(Record.MovesCanonicalJson, Record.Moves);
		}

		const int32 SignatureColumn = bMirrorHasMovesColumn ? 9 : 8;
		const FString SignatureBase64 = UTF8_TO_TCHAR(reinterpret_cast<const char*>(sqlite3_column_text(Statement, SignatureColumn)));

		TArray<uint8> SignatureBytes;
		const bool bSignatureDecoded = FBase64::Decode(SignatureBase64, SignatureBytes);
		// Aceita os DOIS formatos canônicos conhecidos: com golpes e sem.
		//
		// Isto NÃO enfraquece a verificação. Um registro adulterado continua
		// falhando nos dois, porque forjar assinatura exige a chave privada —
		// o que muda é que um pet assinado antes dos golpes existirem continua
		// válido, em vez de ser descartado como violação.
		//
		// A tolerância é de MIGRAÇÃO: quando todo pet tiver sido reassinado, o
		// caminho antigo pode sair.
		const bool bSignatureValid = bSignatureDecoded
			&& (BattlePetCrypto::VerifyEd25519Signature(
					BuildCanonicalPayload(Record, /*bIncludeMoves=*/true), SignatureBytes, Ed25519PublicKeyPem)
				|| BattlePetCrypto::VerifyEd25519Signature(
					BuildCanonicalPayload(Record, /*bIncludeMoves=*/false), SignatureBytes, Ed25519PublicKeyPem));

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

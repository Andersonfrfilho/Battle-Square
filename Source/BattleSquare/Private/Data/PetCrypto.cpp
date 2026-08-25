// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/PetCrypto.h"

// OpenSSL 1.1.1's ossl_typ.h declares "typedef struct ui_st UI;", que
// colide com o namespace UI da própria Unreal (ObjectMacros.h, já
// incluído transitivamente antes daqui). Não usamos o tipo UI do
// OpenSSL (é a abstração antiga de prompt/console dele) — renomear
// durante o include é seguro e resolve a colisão sem tocar em código
// da engine (mesmo padrão do pitfall "check" documentado pela skill
// unreal-thirdparty).
#define UI OpenSSL_UI_Unused
THIRD_PARTY_INCLUDES_START
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
THIRD_PARTY_INCLUDES_END
#undef UI

namespace
{
	constexpr int32 IvLengthBytes = 12;
	constexpr int32 AuthTagLengthBytes = 16;
}

bool BattlePetCrypto::DecryptMirrorBuffer(
	TConstArrayView<uint8> EncryptedBuffer,
	TConstArrayView<uint8> Key32Bytes,
	TArray<uint8>& OutPlaintext)
{
	OutPlaintext.Reset();

	if (Key32Bytes.Num() != 32)
	{
		UE_LOG(LogTemp, Error, TEXT("BattlePetCrypto: chave precisa ter exatamente 32 bytes, tinha %d"), Key32Bytes.Num());
		return false;
	}
	if (EncryptedBuffer.Num() < IvLengthBytes + AuthTagLengthBytes)
	{
		UE_LOG(LogTemp, Error, TEXT("BattlePetCrypto: buffer cifrado curto demais para conter IV+Tag"));
		return false;
	}

	const uint8* Iv = EncryptedBuffer.GetData();
	const uint8* AuthTag = EncryptedBuffer.GetData() + IvLengthBytes;
	const uint8* Ciphertext = EncryptedBuffer.GetData() + IvLengthBytes + AuthTagLengthBytes;
	const int32 CiphertextLength = EncryptedBuffer.Num() - IvLengthBytes - AuthTagLengthBytes;

	EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
	if (!Context)
	{
		return false;
	}

	bool bSuccess = false;
	OutPlaintext.SetNumUninitialized(CiphertextLength);
	int32 OutLength = 0;
	int32 FinalLength = 0;

	if (EVP_DecryptInit_ex(Context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1
		&& EVP_CIPHER_CTX_ctrl(Context, EVP_CTRL_GCM_SET_IVLEN, IvLengthBytes, nullptr) == 1
		&& EVP_DecryptInit_ex(Context, nullptr, nullptr, Key32Bytes.GetData(), Iv) == 1
		&& EVP_DecryptUpdate(Context, OutPlaintext.GetData(), &OutLength, Ciphertext, CiphertextLength) == 1
		&& EVP_CIPHER_CTX_ctrl(Context, EVP_CTRL_GCM_SET_TAG, AuthTagLengthBytes, const_cast<uint8*>(AuthTag)) == 1)
	{
		// Retorno <= 0 aqui é a detecção de adulteração/chave errada do
		// GCM — o mesmo mecanismo testado do lado TypeScript (AD-018).
		bSuccess = EVP_DecryptFinal_ex(Context, OutPlaintext.GetData() + OutLength, &FinalLength) == 1;
	}

	EVP_CIPHER_CTX_free(Context);

	if (!bSuccess)
	{
		OutPlaintext.Reset();
		return false;
	}

	OutPlaintext.SetNum(OutLength + FinalLength);
	return true;
}

bool BattlePetCrypto::VerifyEd25519Signature(
	const FString& CanonicalJsonPayload,
	TConstArrayView<uint8> SignatureBytes,
	const FString& PublicKeyPem)
{
	const FTCHARToUTF8 PemUtf8(*PublicKeyPem);
	BIO* KeyBio = BIO_new_mem_buf(PemUtf8.Get(), PemUtf8.Length());
	if (!KeyBio)
	{
		return false;
	}

	EVP_PKEY* PublicKey = PEM_read_bio_PUBKEY(KeyBio, nullptr, nullptr, nullptr);
	BIO_free(KeyBio);
	if (!PublicKey)
	{
		UE_LOG(LogTemp, Error, TEXT("BattlePetCrypto: chave pública PEM inválida"));
		return false;
	}

	EVP_MD_CTX* MdContext = EVP_MD_CTX_new();
	bool bSuccess = false;

	if (MdContext)
	{
		const FTCHARToUTF8 PayloadUtf8(*CanonicalJsonPayload);

		// Ed25519 no OpenSSL: MD deve ser nullptr (o algoritmo já define
		// o digest internamente), e a verificação é sempre one-shot —
		// EVP_DigestVerifyUpdate não é suportado para Ed25519.
		if (EVP_DigestVerifyInit(MdContext, nullptr, nullptr, nullptr, PublicKey) == 1)
		{
			bSuccess = EVP_DigestVerify(
				MdContext,
				SignatureBytes.GetData(),
				SignatureBytes.Num(),
				reinterpret_cast<const uint8*>(PayloadUtf8.Get()),
				PayloadUtf8.Length()) == 1;
		}
		EVP_MD_CTX_free(MdContext);
	}

	EVP_PKEY_free(PublicKey);
	return bSuccess;
}
